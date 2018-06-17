#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

#define ASTRONET_BASE_NODE

#include <stddef.h>
#include <stdint.h>
#include <Astronet_config.h>
#include <Astronet.h>

// Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8
RF24 radio(7, 8);
Astronet network(radio);

void setup() {

  Serial.begin(115200);
  printf_begin();
  Serial.println(F("*** PRESS 'M' for MAIN MENU"));

  // Setup and configure rf radio
  radio.begin();
  radio.setChannel(112);
  radio.setAutoAck(1);       // Ensure autoACK is enabled
  radio.enableDynamicPayloads();
  radio.enableAckPayload();  // Allow optional ack payloads
  radio.setRetries(__ASTRONET_RETRY_DELAY,__ASTRONET_RETRY_MAX_NUMBER);                 // Smallest time between retries, max no. of retries
  radio.setPayloadSize(__ASTRONET_PAYLOAD_SIZE);  // Here we are sending 1-byte payloads to test the call-response speed
  radio.setDataRate(RF24_2MBPS);

  network.begin();
}

void loop(void) {
  network.refresh();

  while (network.available()) {
    Payload data;
    network.getData(data);
    network.parsePaket(data);
  }

  if ( Serial.available() )
  {
    char c = toupper(Serial.read());
    switch ( c )
    {
      case 'I':
          network.readInfo();
          break;
      case 'D':
          radio.printDetails(); // Dump the configuration of the rf unit for debugging
          break;
      case 'T':
          {char tx_data[] = "Hi 0xFF, are you there?";
          network.send(0x21,tx_data,24);}
          break;
      case 'P':
          {Serial.println(F("*** TRANSFERING 10 PATCH"));
          for(int j=0;j<100;j++){
            char tx_data2[] = "Hi, are you there? [P0]";
            tx_data2[21] = j+48;
            network.send(0x21,tx_data2,24);
            delay(25);
          }}
          break;
      case 'B':
          {Serial.println(F("*** GOTO BASE MODE"));
          network.setNewAddress(0x01);}
          break;
       case 'N':
          {Serial.println(F("*** GOTO NODE MODE"));
          network.setNewAddress(0x21);}
          break;
       case 'L':
          network.neighboursList();
          break;
    }
  }

}
