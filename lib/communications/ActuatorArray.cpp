
#include "actuator.h"
#include <cstdlib>
#include <Arduino.h>


int8_t binary_search(uint8_t *array, uint8_t start, uint8_t end, uint8_t val);
int8_t sequential_search(uint8_t *array, uint8_t size, uint8_t val);

ActuatorArray::ActuatorArray(const uint8_t size) {
  _backingStore = (Actuator **) std::malloc(size * sizeof(Actuator *));
  _ids = (uint8_t *) std::malloc(size * sizeof(uint8_t));
  _size = size;
}

Actuator * ActuatorArray::get(uint8_t id) {
  Serial.println("Get Actuator, looking for id: " + String(id));
  Serial.flush();
  int8_t index = sequential_search(_ids, _size, id); //binary_search(_ids, 0, _size, id);
  if (index != -1){
    return _backingStore[index];
  } else {
    Serial.println("ERROR. ID NOT FOUND IN ACTUATOR ARRAY.");
    Serial.flush();
    delay(1000);
    exit(1);
  }
}

void ActuatorArray::insert(Actuator * toInsert) {
  if(currIdx >= _size){
    Serial.println("Actuator Array is Full. Error!");
    Serial.flush();
    exit(1);
  }

  _backingStore[currIdx] = toInsert;
  _ids[currIdx] = toInsert->ID();
  currIdx++;
}

int8_t sequential_search(uint8_t *array, uint8_t size, uint8_t val) {
  for (int i = 0; i < size; i++){
    if(array[i] == val){
      return i;
    }
  }
  return -1;
}

int8_t binary_search(uint8_t *array, uint8_t start, uint8_t end, uint8_t val) {
  Serial.println("binary search, start: " + String(start) + ", end: " + String(end));
  Serial.flush();
  if (array[start] == val){
    return start;
  } else if (start == end){
    return -1;
  } else {
    uint8_t new_size = (end - start + 1) / 2;
    uint8_t res1 = binary_search(array, start + new_size, end, val);
    uint8_t res2 = binary_search(array, start, end - new_size, val);
    if (res1 != -1){
      return res1;
    } else if (res2 != -1) {
      return res2;
    } else {
      return -1;
    }
  }
}