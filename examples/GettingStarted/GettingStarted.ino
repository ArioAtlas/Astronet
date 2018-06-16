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
  radio.setAutoAck(1);       // Ensure autoACK is enabled
  radio.enableAckPayload();  // Allow optional ack payloads
  radio.setRetries(__ASTRONET_RETRY_DELAY,__ASTRONET_RETRY_MAX_NUMBER);                 // Smallest time between retries, max no. of retries
  radio.setPayloadSize(__ASTRONET_PAYLOAD_SIZE);  // Here we are sending 1-byte payloads to test the call-response speed

  network.begin();
}

void loop(void) {
  network.refresh();

  while (network.available()) {
    Payload data;
    network.getData(data);
    Serial.print("Data received = ");
    Serial.print(data.from, HEX);
    Serial.print(" ");
    Serial.print(data.to, HEX);
    Serial.print(" ");
    Serial.print(data.scs, HEX);
    Serial.print(" ");
    Serial.print(data.id, HEX);
    Serial.print(" ");
    Serial.print(data.router, HEX);
    Serial.print(" ");
    for (int i = 0; i < 24; i++) {
      Serial.print(data.data[i], HEX);
      Serial.print("|");
    }
    Serial.print(" ");
    Serial.print(data.ndb, HEX);
    Serial.println("");
  }

  if ( Serial.available() )
  {
    char c = toupper(Serial.read());
    switch ( c )
    {
      case 'I':
          Serial.println(F("*** NODE INITIALIZE PROCESS -- PRESS 'M' TO MAIN MENU"));
          break;
      case 'D':
          radio.printDetails(); // Dump the configuration of the rf unit for debugging
          break;
      case 'T':
          {char tx_data[] = "Hi 0xFF, are you there?";
          network.send(0xFF,tx_data,24);}
          break;
      case 'P':
          {Serial.println(F("*** TRANSFERING 100 PATCH TO 0xFFFF"));
          for(int j=0;j<10;j++){
            char tx_data2[] = "Hi, are you there? [P0]";
            tx_data2[21] = j+48;
            network.send(0xFF,tx_data2,24);
          }}
          break;
      case 'B':
          {Serial.println(F("*** GOTO BASE MODE"));
          network.setNewAddress(0x00);}
          break;
       case 'R':
          {Serial.println(F("*** GOTO NODE MODE"));
          network.setNewAddress(0x21);}
          break;
    }
  }

  delay(100);
}
