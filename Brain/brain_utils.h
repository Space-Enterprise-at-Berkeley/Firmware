/*
 * Brain_utils.h- A c++ program that uses I2C to establish communication between
 * the sensors and valves inside to the rocket with the ground station. Able to send
 * data to the ground station via RF. Can receive and process commands sent from
 * ground station. This contains additional functions and structs used in Brain_I2C.ino.
 * Created by Vainavi Viswanath, Aug 21, 2020.
 */
#include "solenoids.h"

String make_packet(struct sensorInfo sensor);
uint16_t Fletcher16(uint8_t *data, int count);

Solenoids sol;

/*
 * Data structure to allow the conversion of bytes to floats and vice versa.
 */
union floatArrToBytes {
  char buffer[24];
  float sensorReadings[6];
} farrbconvert;

/*
 * Data structure to store all information relevant to a specific sensor type.
 */
struct sensorInfo {
  String sensor_name;
  int board_address;
  byte sensor_id;
  int overall_id;
  int clock_freq;
  void  (*dataReadFunc)(float *data);
  //sensorInfo(String n, int b, byte s, int o, int f) : sensor_name(n), board_address(b), sensor_id(s), overall_id(o), clock_freq(f) {}
};

/*
 * Data structure to store all information relevant to a specific valve.
 */
struct valveInfo {
  String valve_name;
  int valve_id;
  int (*openValve)();
  int (*closeValve)();
  //valveInfo(String n, int v): valve_name(n), valve_id(v) {}
};

valveInfo valve_ids[7] = {
  valveInfo("LOX 2 Way", 20, &(Solenoids::armLOX), &(Solenoids::disarmLOX)); //example
};

int numValves = 1;


/*
 * Constructs packet in the following format:
 * {<sensor_ID>,<data1>,<data2>, ...,<dataN>|checksum}
 */
String make_packet(struct sensorInfo sensor) {
  String packet_content = (String)sensor.overall_id;
  packet_content += ",";
  for (int i=0; i<6; i++) {
    float reading = farrbconvert.sensorReadings[i];
    if (reading > 0) {
      packet_content += (String)reading;
      packet_content += ",";
    } else {
      packet_content.remove(packet_content.length()-1);
      int count = packet_content.length();
      char const *data = packet_content.c_str();
      uint16_t checksum = Fletcher16((uint8_t *) data, count);
      packet_content += "|";
      packet_content += (String)checksum;
      break;
    }
  }
  String packet = "{" + packet_content + "}";
  return packet;
}

/*
 * Decodes a packet sent from ground station in the following format:
 * {<valve_ID>,<open(1) or close(0)|checksum}
 * Populated the fields of the valve and returns the action to be taken
 */
int decode_received_packet(String packet, valveInfo *valve) {
  int ind1 = packet.indexOf(',');
  int valve_id = packet.substring(1,ind1).toInt();
  int ind2 = packet.indexOf('|');
  int action = packet.substring(ind1+1,ind2).toInt();
  uint16_t checksum = (uint16_t)packet.substring(ind2+1, packet.length()-1).toInt();
  char const *data = packet.substring(1,ind2).c_str();
  int count = packet.substring(1,ind2).length();
  uint16_t check = Fletcher16((uint8_t *) data, count);
  if (check == checksum) {
    chooseValveById(valve_id, valve);
    return action;
  } else {
    return -1;
  }
}

void chooseValveById(int id, valveInfo *valve) {
  for (int i = 0; i < numValves; i ++) {
    if (valve_ids[i].id == id) {
      valve = valve_ids[i];
      break;
    }
  }
}

/*
 * Calls the corresponding method for this valve with the appropriate
 * action in solenoids.h
 */
void take_action(valveInfo *valve, int action) {
  if (action) {
    valve->openValve();
  } else {
    valve->closeValve();
  }
}

/*
 * Calculates checksum for key values being sent to ground station:
 * sensor_ID and it's corresponding data points
 */
uint16_t Fletcher16(uint8_t *data, int count) {

  uint16_t sum1 = 0;
  uint16_t sum2 = 0;

  for (int index=0; index<count; index++) {
    if (data[index] > 0) {
      sum1 = (sum1 + data[index]) % 255;
      sum2 = (sum2 + sum1) % 255;
    }
  }
  return (sum2 << 8) | sum1;
}
