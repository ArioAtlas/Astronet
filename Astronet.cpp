


#include "Astronet_config.h"
#include "Astronet.h"
#include "RF24.h"
#include <stddef.h>
#include <stdint.h>
#include <EEPROM.h>


int Astronet::id = 1;
int Astronet::success=0;
int Astronet::failed=0;
int Astronet::overflow=0;
int Astronet::junk=0;
int Astronet::duplicate=0;
int Astronet::damaged=0;

Astronet::Astronet(RF24& _radio):radio(_radio){
  loadPin();
  loadAddress();
  initial();
  };

void Astronet::begin(){
  printf("\nListen at %x",address );
  radio.openReadingPipe(1,address);
  radio.startListening();
};

bool Astronet::updateAddress(uint8_t _address){
  address=_address;
};

void Astronet::refresh(){
    byte pipeNo;
    while( radio.available(&pipeNo)&&radio.isChipConnected()){
      printf("\n\nRoP->#%d",pipeNo);
      radio.read( &temp, __ASTRONET_PAYLOAD_SIZE );
      parsePaket((temp));
      if(temp.scs == (temp.from ^ temp.to)){
        if(temp.type<__ASTRONET_ACK_TYPE_DIVIDER){
          if(temp.to == address){
            printf("\nRX->handle");
            handleIncoming(temp);
            acknowlede(temp);
          }
          else{
            printf("\nRouter");
            temp.router = address;
            route(temp.to,temp);
          }
        }
      }
      else{
        junk++;
      }
   };
};

bool Astronet::write(uint8_t _to, Payload payload){
    unsigned long started = millis();
    unsigned long loop;
    bool timeout;
    int attempt = 1;
    Payload ply;
    while (attempt<=__ASTRONET_RETRY_MAX_NUMBER) {
      timeout = false;
      radio.openWritingPipe(_to);
      radio.stopListening();
      radio.write( &payload, __ASTRONET_PAYLOAD_SIZE );
      radio.startListening();
      loop = millis();
      while ( ! radio.available() && ! timeout ){
        if (millis() - loop > __ASTRONET_RETRY_DELAY ){
          timeout = true;
          attempt++;
        }
      }
      if (!timeout){
        radio.read( &ply, 32 );
        printf("\nSuccess %dms",(millis()-started));
        printf("[R:%d]",attempt);

        if(payload.type == (ply.type+__ASTRONET_ACK_TYPE_DIVIDER
            && payload.to == ply.from
            && payload.from == ply.to
            && payload.id == ply.id))
        break;
      }
    }
    if(attempt >__ASTRONET_RETRY_MAX_NUMBER){
      printf("\nFailed after %dms",(millis()-started));
      printf("[R:%d]",attempt);
      removeNeighbor(payload.to);
      return false;
    }
    else{
      printf("\nDLV in %dms",(millis()-started));
      printf("[R:%d]",attempt);
      addNeighbor(payload.to);
      return true;
    }
  };

bool Astronet::writeBlind(uint8_t _to, Payload payload){
    radio.openWritingPipe(_to);
    radio.stopListening();
    radio.write( &payload, __ASTRONET_PAYLOAD_SIZE );
    radio.startListening();
  };
/*!
 * \brief Send data throw mesh Network
 * \param to reciever node _address
 * \param *data pointer to data that you want to trasfer
 * \param size size of data packet (Max 24bit)
 * \details this function handle trafsering data to destination node of
 * mesh network
*/
bool Astronet::send(uint8_t to,void *data,uint8_t size){
  Payload packet;
  packet.type = 1;
  packet.to = to;
  packet.from = address;
  packet.scs = to ^ address;
  packet.id = id;
  packet.router = address;
  memcpy(packet.data,data,size);
  for(uint8_t i=size;i<24;i++){
    packet.data[i]=0x00;
  }
  packet.ndb = dataSetBits(packet);
  id= (id!=255)?id+1:1;

  return route(to,packet);
};

bool Astronet::getData(Payload &item){
 if(inbound_inedx > 0){
   item = inbound[0].packet;
   inbound_inedx--;
   if(inbound_inedx)
      memcpy(inbound,inbound+1,sizeof(Traffic)*inbound_inedx);
   return true;
 }
 return false;
};

void Astronet::addNeighbor(uint8_t address){
  if(address != __ASTRONET_BLIND_NODE_ADDRESS){
    uint8_t found = false;
    for(int i=0; i<neighbor_index; i++){
      if(neighbors[i]==address){
        found = true;
        break;
      }
    }

   if(!found && neighbor_index < __ASTRONET_MAX_NEIGHBORS_MEMORY){
     neighbors[neighbor_index] = address;
     neighbor_index++;
     return true;
   }
  }
};

void Astronet::removeNeighbor(uint8_t address){
  if(address != __ASTRONET_BLIND_NODE_ADDRESS){
    if(neighbor_index > 0){
      uint8_t index=neighbor_index;
      for(uint8_t i=0; i<neighbor_index; i++){
        if(neighbors[i] == address){
          index = i;
          break;
        }
      }

      if(index<neighbor_index){
        neighbor_index--;
        if(neighbor_index && neighbor_index!=index)
           memcpy(neighbors+index,neighbors+index+1,sizeof(uint8_t)*(neighbor_index-index));
      }
    }
  }
};

bool Astronet::available(){
  if(inbound_inedx)
    return true;
  return false;
};

bool Astronet::route(uint8_t _to,Payload &data){
  if(write(_to,data)){
    success++;
    return true;
  }
  else{
    bool send = false;
    for(int i=0; i<neighbor_index; i++){
      if(neighbors[i] != data.from
        && neighbors[i] != data.to
        && neighbors[i] != data.router){
        printf("\n#Neighbour Route *%x",neighbors[i]);
        data.router = address;
        send = send || write(neighbors[i],data);
      }
    }
    if(send)
      success++;
    else
      failed++;
    return send;
  }
};

// need update
uint8_t Astronet::retrieveNewAddress(Payload &packet){
    return packet.data[1]&&(0x0000FFFF);
};

bool Astronet::checkPin(Payload &packet){
  uint64_t secret = (uint64_t)packet.data[3]<<32 || (uint64_t)packet.data[4];
  if(secret==pin)
    return true;
  return false;
};

void Astronet::acknowlede(Payload packet){
  packet.type += __ASTRONET_ACK_TYPE_DIVIDER;
  packet.to = packet.from;
  packet.from = address;
  packet.router = address;
  writeBlind(packet.to,packet);
  printf("\nError in writing ack *%x-[%x] to #%x",packet.type,packet.id,packet.to );
};

void Astronet::loadPin(){
  EEPROM.get( __ASTRONET_EEPROM_PIN_ADDRESS_START, pin );
};

void Astronet::loadAddress(){
    EEPROM.get( __ASTRONET_EEPROM_NODE_ADDRESS_START, address );
}

void Astronet::initial(){
  chipReady = true;
  inbound_inedx = 0;
  neighbor_index = 0;
  history_inedx = 0;
}

void Astronet::setNewAddress(uint8_t _address){
    EEPROM.put(__ASTRONET_EEPROM_NODE_ADDRESS_START, _address);
    address = _address;
    begin(); // listen on new address
};

bool Astronet::addToInbound(Traffic rx){
     if(inbound_inedx < __ASTRONET_MAX_INBOUND_BUFFER){
       inbound[inbound_inedx] = rx;
       inbound_inedx++;
       return true;
     }
     return false;
};

void Astronet::handleIncoming(Payload packet){
    if(packet.ndb == dataSetBits(packet)){
      bool trash = false;
      for(int i=0; i<inbound_inedx;i++){
        if(inbound[i].packet == packet){
          trash=true;
          printf("\nDuplicate Data...");
          duplicate++;
          break;
        }
      }

      if(!trash){
        Traffic rx = {packet,false};
        if(!addToInbound(rx)){
          printf("\nIncoming buffer is full...");
          overflow++;
        }

        addNeighbor(packet.router);
      }
    }
    else{
      printf("\nNDB miss match...");
      damaged++;
    }

};

uint8_t Astronet::dataSetBits(Payload packet){
    uint8_t num = 0;
    uint32_t* i;
    for(uint8_t j=0;j<sizeof(packet.data);j+=4){
      i = (uint32_t*)&packet.data[j];
      *i = *i - ((*i >> 1) & 0x55555555);
      *i = (*i & 0x33333333) + ((*i >> 2) & 0x33333333);
      num+= (((*i + (*i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
    }
    return num;
};

void Astronet::parsePaket(Payload& paket){
  printf("\nRX=> TP:%x ",paket.type);
  printf("FROM:%x ",paket.from);
  printf("TO:%x ",paket.to);
  printf("SCS:%x ",paket.scs);
  printf("ID:%x ",paket.id);
  printf("ROUTER:%x ",paket.router);
  printf("DATA:[");
  for (int i = 0; i < 24; i++) {
    printf("%x,",paket.data[i]);
  }
  printf("] NDB:%x",paket.ndb);
  printf("\n");
};

void Astronet::readInfo(){
  printf("\n[ASTRONET INFO]\n");
  printf("Node:%x\n", address);
  printf("PIN:%x\n", pin);
  printf("InBound:%d\n", inbound_inedx);
  printf("History:%d\n", history_inedx);
  printf("Neighbour:%d\n", neighbor_index);
  printf("Success:%d\n", success);
  printf("Failed:%d\n", failed);
  printf("Oerflow:%d\n", overflow);
  printf("Junk:%d\n", junk);
  printf("Duplicate:%d\n", duplicate);
  printf("Damaged:%d\n", damaged);
};

void Astronet::neighboursList(){
  for(int i=0;i<neighbor_index;i++){
    printf("%d => %x\n",i,neighbors[i] );
  }
};

bool Payload::operator==(const Payload& second){
    if(type==second.type
      && from==second.from
      && to==second.to
      && scs==second.scs
      && id==second.id
      && router==second.router
      && ndb==second.ndb)
      return true;
    return false;
}

bool Payload::operator!=(const Payload& second){
    if(type==second.type
      && from==second.from
      && to==second.to
      && scs==second.scs
      && id==second.id
      && router==second.router
      && ndb==second.ndb)
      return true;
    return false;
}
