#include <solenoids.h>
#include <Analog_Thermocouple.h>
#include <Cryo_Thermocouple.h>
#include <common_fw.h>
#include <ADS1219.h>
#include <ADS8167.h>
#include <Automation.h>
#include <INA219.h>
#include <GpioExpander.h>

#define FLIGHT_BRAIN_ADDR 0x00
#define DEBUG 1

std::string str_file_name = "E1_speed_test_results.txt";
const char * file_name = str_file_name.c_str();

const uint8_t numCryoTherms = 4;
// therm[2] = lox adapter tree pt, therm[3] = lox adapter tree gems
// ADDR = GND, VDD, 10k & 4.3K, 10K & 13K
int cryoThermAddrs[numCryoTherms] = {0x60, 0x67, 0x62, 0x64};
_themotype cryoTypes[numCryoTherms] = {MCP9600_TYPE_J, MCP9600_TYPE_T, MCP9600_TYPE_T, MCP9600_TYPE_K};

const uint8_t numADCSensors = 2;
int adcCSPins[numADCSensors] = {0, 1};
int adcDataReadyPins[numADCSensors] = {29, 28};
int adcAlertPins[numADCSensors] = {30, 31};
ADS8167 ** ads;

const uint8_t numAnalogThermocouples = 1;
int thermAdcIndices[numAnalogThermocouples] = {1};
int thermAdcChannels[numAnalogThermocouples] = {2};

const uint8_t numPressureTransducers = 7;
int ptAdcIndices[numPressureTransducers] = {0, 0, 0, 0, 1, 1, 1}; //not using 1-0 or 1-3
int ptAdcChannels[numPressureTransducers] = {0, 1, 2, 3, 2, 1, 3};
int ptTypes[numPressureTransducers] = {1, 1, 1, 1, 2, 1, 1};

const uint8_t numPowerSupplyMonitors = 2;       //12v  , 8v
uint8_t powSupMonAddrs[numPowerSupplyMonitors] = {0x44, 0x45};
INA219 powerSupplyMonitors[numPowerSupplyMonitors];

const uint8_t numGPIOExpanders = 1;
uint8_t gpioExpAddr[numGPIOExpanders] = {0x20};
int8_t gpioExpIntPin[numGPIOExpanders] = {-1};
GpioExpander heaterCtl(gpioExpAddr[0], gpioExpIntPin[0], &Wire);

const uint8_t numSensors = 6;
sensorInfo *sensors;

const int numValves = 11;
struct valveInfo *valves;

#define LOX_2_PIN 0
#define LOX_5_PIN 2
#define LOX_GEMS_PIN 4

#define PROP_2_PIN 1
#define PROP_5_PIN 3
#define PROP_GEMS_PIN 5

#define HIGH_SOL_PIN 6

#define LOX_ADAPTER_PT_HEATER_PIN 7
#define LOX_GEMS_HEATER_PIN 8

const float batteryMonitorShuntR = 0.002; // ohms
const float batteryMonitorMaxExpectedCurrent = 10; // amps

const float powerSupplyMonitorShuntR = 0.010; // ohms
const float powerSupplyMonitorMaxExpectedCurrent = 5; // amps


namespace config {
  void setup() {

    debug("Initializing ADCs", DEBUG);
    // initialize all ADCs
    ads = new ADS8167*[numADCSensors];
    for (int i = 0; i < numADCSensors; i++) {
      ads[i] = new ADS8167(&SPI1, adcCSPins[i], adcDataReadyPins[i], adcAlertPins[i]);
      ads[i]->init();
      ads[i]->setManualMode();
      ads[i]->setAllInputsSeparate();
      pinMode(adcDataReadyPins[i], INPUT_PULLUP);
    }

    debug("Initializing Power Supply monitors", DEBUG);
    for (int i = 0; i < numPowerSupplyMonitors; i++) {
        powerSupplyMonitors[i].begin(&Wire, powSupMonAddrs[i]);
        powerSupplyMonitors[i].configure(INA219_RANGE_16V, INA219_GAIN_40MV, INA219_BUS_RES_12BIT, INA219_SHUNT_RES_12BIT_1S);
        powerSupplyMonitors[i].calibrate(powerSupplyMonitorShuntR, powerSupplyMonitorMaxExpectedCurrent);
    }

    debug("Initializing sensors", DEBUG);
    sensors = new sensorInfo[numSensors];
    // the ordering in this array defines order of operation, not id
    sensors[0] = {"All Pressure",  FLIGHT_BRAIN_ADDR, 1, 1};
    sensors[1] = {"Battery Stats", FLIGHT_BRAIN_ADDR, 2, 3};
    sensors[2] = {"Cryo Temps",      FLIGHT_BRAIN_ADDR, 4, 3};
    sensors[3] = {"Lox PT Temperature",   FLIGHT_BRAIN_ADDR, 0, 4}; //&(testTempRead)}, //&(Thermocouple::readTemperatureData)},
    sensors[4] = {"Number Packets Sent", FLIGHT_BRAIN_ADDR, 5, 10};
    sensors[5] = {"LOX Gems Temp", FLIGHT_BRAIN_ADDR, 6, 4};

    debug("Initializing valves", DEBUG);
    valves = new valveInfo[numValves];
    valves[0] = {"LOX 2 Way", 20, &(Solenoids::armLOX), &(Solenoids::disarmLOX), &(Solenoids::getAllStates)};
    valves[1] = {"LOX 5 Way", 21, &(Solenoids::openLOX), &(Solenoids::closeLOX), &(Solenoids::getAllStates)};
    valves[2] = {"LOX GEMS", 22, &(Solenoids::ventLOXGems), &(Solenoids::closeLOXGems), &(Solenoids::getAllStates)};
    valves[3] = {"Propane 2 Way", 23, &(Solenoids::armPropane), &(Solenoids::disarmPropane), &(Solenoids::getAllStates)};
    valves[4] = {"Propane 5 Way", 24, &(Solenoids::openPropane), &(Solenoids::closePropane), &(Solenoids::getAllStates)};
    valves[5] = {"Propane GEMS", 25, &(Solenoids::ventPropaneGems), &(Solenoids::closePropaneGems), &(Solenoids::getAllStates)};
    valves[6] = {"High Pressure Solenoid", 26, &(Solenoids::activateHighPressureSolenoid), &(Solenoids::deactivateHighPressureSolenoid), &(Solenoids::getAllStates)};
    valves[7] = {"Arm Rocket", 27, &(Solenoids::armAll), &(Solenoids::disarmAll), &(Solenoids::getAllStates)};
    valves[8] = {"Launch Rocket", 28, &(Solenoids::LAUNCH), &(Solenoids::endBurn), &(Solenoids::getAllStates)};
    valves[9] = {"Perform Flow", 29, &(Automation::beginBothFlow), &(Automation::endBothFlow), &(Automation::flowConfirmation)};
    valves[10] = {"Perform LOX Flow", 30, &(Automation::beginLoxFlow), &(Automation::endLoxFlow), &(Automation::flowConfirmation)};


    pinMode(LOX_2_PIN, OUTPUT);
    pinMode(LOX_5_PIN, OUTPUT);
    pinMode(LOX_GEMS_PIN, OUTPUT);

    pinMode(PROP_2_PIN, OUTPUT);
    pinMode(PROP_5_PIN, OUTPUT);
    pinMode(PROP_GEMS_PIN, OUTPUT);

    pinMode(HIGH_SOL_PIN, OUTPUT);

    pinMode(LOX_ADAPTER_PT_HEATER_PIN, OUTPUT);

  }
}
