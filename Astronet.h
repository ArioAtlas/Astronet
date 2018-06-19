/*!
 * \author Omid Golshan
 * \version 1.0
 * \date 12/6/2018
 * \copyright GNU Public License
 * \mainpage RF24 Astronet
 * \section intro_sec Introduction
 * This library is generated to create Mesh-Network for RF24
 */


/**
 * @file Astronet.h
 *
 * Class declaration for Astronet
 */


#ifndef __ASTRONET_H__
#define __ASTRONET_H__

#include "Astronet_config.h"
#include <stddef.h>
#include <stdint.h>

typedef struct Payload {
  uint8_t type; //!< type of packet (N/A)
  uint8_t from; //!< source address (sender address)
  uint8_t to; //!< destination address
  uint8_t scs; //!< Sender check sum (from XOR to)
  uint16_t id; //!< sender packet identifier
  uint8_t router; //!< last sender address
  uint8_t data[24]; //!< 24bit data array
  uint8_t ndb; //!< number of SET bits in data array
  bool operator==(const Payload& second);
  bool operator!=(const Payload& second);
} Payload;

// typedef union Transit {
//   uint64_t serial;
//   uint16_t header[4];
// } Transit;

typedef struct Traffic {
  Payload packet;
  bool ack;
} Traffic;

class RF24;

class Astronet{
  private:
    static int id;
    bool chipReady;
    uint8_t address;
    Traffic inbound[__ASTRONET_MAX_INBOUND_BUFFER];
    uint8_t inbound_inedx;
    uint32_t history[__ASTRONET_MAX_HISTORY_SIZE];
    uint8_t history_inedx;
    uint8_t neighbors[__ASTRONET_MAX_NEIGHBORS_MEMORY];
    uint8_t neighbor_index;
    RF24& radio;
    uint64_t pin;
    static int success;
    static int failed;
    static int overflow;
    static int junk;
    static int duplicate;
    static int damaged;

    Payload temp;
  public:
    Astronet(RF24& _radio);

    void begin();

    bool updateAddress(uint8_t _address);

    void refresh();

    bool write(uint8_t _to, Payload payload);
    bool writeBlind(uint8_t _to, Payload payload);

    bool getData(Payload &item);

    void addNeighbor(uint8_t address);

    void removeNeighbor(uint8_t address);

    bool available();

    bool route(uint8_t _to,Payload &data);

    uint8_t retrieveNewAddress(Payload &packet);

    bool checkPin(Payload &packet);

    void acknowlede(Payload packet);

    void loadPin();

    void loadAddress();

    void initial();

    void setNewAddress(uint8_t _address);

    bool addToInbound(Traffic rx);

    void handleIncoming(Payload packet);

    uint8_t dataSetBits(Payload packet);

    bool send(uint8_t to,void *data,uint8_t size);

    void parsePaket(Payload& paket);

    void readInfo();

    void neighboursList();
};



#endif // __ASTRONET_H__
