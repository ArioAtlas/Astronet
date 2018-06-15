#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

#include <stddef.h>
#include <stdint.h>

#include <Astronet_config.h>
#include <Astronet.h>


#define ASTRONET_BASE_NODE

// Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8
RF24 radio(7, 8);
Astronet network(radio);

void setup() {

  Serial.begin(115200);
  printf_begin();
  Serial.println(F("*** PRESS 'M' for MAIN MENU"));

  // Setup and configure rf radio
  radio.begin();
  radio.setAutoAck(1);                    // Ensure autoACK is enabled
  radio.enableAckPayload();               // Allow optional ack payloads
  radio.setRetries(__ASTRONET_RETRY_DELAY,__ASTRONET_RETRY_MAX_NUMBER);                 // Smallest time between retries, max no. of retries
  radio.setPayloadSize(__ASTRONET_PAYLOAD_SIZE);                // Here we are sending 1-byte payloads to test the call-response speed

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
    Serial.print(data.id, HEX);
    Serial.print(" ");
    Serial.print(data.cmd, HEX);
    Serial.print(" ");
    for (int i = 0; i < 6; i++) {
      Serial.print(data.data[i], HEX);
      Serial.print("|");
    }
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
      case 'R':
          radio.printDetails();                   // Dump the configuration of the rf unit for debugging
          break;

    }
  }

  delay(100);
}
