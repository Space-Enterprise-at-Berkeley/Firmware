/*
  Cryo_Thermocouple.cpp - A c++ library to interface with heavy duty cryo/high temp
  thermocouples using the MCP9600 chip.
  Created by Vamshi Balanaga, Feb 5, 2021.
*/



#include <Cryo_Thermocouple.h>

namespace Thermocouple {

    int Cryo::init(uint8_t numSensors, Adafruit_MCP9600 *cryo_boards, uint8_t * addrs, _themotype * types, TwoWire *theWire, float *latestReads) { // assume that numSensors is < max Size of packet. Add some error checking here
      _addrs = addrs;
      _latestReads = latestReads;
      _cryo_amp_boards = cryo_boards;

      _numSensors = numSensors;

      for (int i = 0; i < numSensors; i++) {

        if (!_cryo_amp_boards[i].begin(addrs[i], theWire)) {
          Serial.println("Error initializing cryo board at Addr 0x" + String(addrs[i], HEX));
          return -1;
        }

        _cryo_amp_boards[i].setADCresolution(MCP9600_ADCRESOLUTION_12);
        _cryo_amp_boards[i].setThermocoupleType(types[i]);
        _cryo_amp_boards[i].setFilterCoefficient(3);
        _cryo_amp_boards[i].enable(true);
      }

      for (int i = 0; i < _numSensors; i++) {
        Serial.println(i);
        Serial.println(_latestReads[i]);
        _latestReads[i] = -1;
        Serial.println(_latestReads[i]);
        _latestReads[i] = _cryo_amp_boards[i].readThermocouple();
        Serial.println(_latestReads[i]);
      }

      return 0;
    }

    void Cryo::readCryoTemps(float *data) {
      #ifdef DEBUG
      Serial.println("read Cryo temps");
      Serial.flush();
      #endif
      for (int i = 0; i < _numSensors; i++) {
        #ifdef DEBUG
        Serial.print(i);
        Serial.flush();
        #endif
        // data[i] = _cryo_amp_boards[i].readThermocouple();
        data[i] = _cryo_amp_boards[i].readAmbient();
        // data[i] = _cryo_amp_boards[i].readADC();
        #ifdef DEBUG
        Serial.print(data[i]);
        Serial.flush();
        #endif
        _latestReads[i] = data[i];
      }
      data[_numSensors] = -1;
    }

    void Cryo::readSpecificCryoTemp(uint8_t index, float *data) {
      data[0] = _latestReads[index];
      data[1] = -1;
    }

    int Cryo::freeAllResources() {
        free(_latestReads);
        return 0;
    }

  }
