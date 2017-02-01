/////////////////////////////////////////////////////////////
// MySensor definition
/////////////////////////////////////////////////////////////

#define MY_DEBUG 
#define MY_RADIO_NRF24

#ifdef MY_DEBUG
  #define MY_NODE_ID 11 // need that when you doesn't have controller
#endif

/////////////////////////////////////////////////////////////
// Includes
/////////////////////////////////////////////////////////////

#include <SPI.h>
#include <MySensors.h>  
#include <DallasTemperature.h>
#include <OneWire.h>
#include <NewPing.h>

/////////////////////////////////////////////////////////////
// Definition
/////////////////////////////////////////////////////////////

#define PIN_SENSOR_TEMP 3 //D3
#define PIN_BATTERY_SENSE 14 // A0
#define PIN_TRIGGER_WATER_LEVEL  6  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define PIN_WATER_LEVEL_ECHO   5  // Arduino pin tied to echo pin on the ultrasonic sensor.

#define TEMP_SENSOR_ID 0
#define WATER_LEVEL_SENSOR_ID 1

/////////////////////////////////////////////////////////////
// Global variables 
/////////////////////////////////////////////////////////////

/////////////////// BATTERY /////////////////////////////

int oldBatteryPcnt = -42;

/////////////////// TEMPERATURE /////////////////////////////

OneWire oneWireTemp(PIN_SENSOR_TEMP); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensorsTemp(&oneWireTemp); // Pass the oneWire reference to Dallas Temperature. 
float lastTemperature = -255.;
MyMessage tempMsg(TEMP_SENSOR_ID, V_TEMP);

/////////////////// WATER LEVEL /////////////////////////////

#define MAX_WATER_DISTANCE 200 // This value define the empty tank level(in cm)
#define MIN_WATER_DISTANCE 50 // This value define the full tank level(in cm)

NewPing sonar(PIN_TRIGGER_WATER_LEVEL, PIN_WATER_LEVEL_ECHO, MAX_WATER_DISTANCE);
MyMessage msg(CHILD_ID, V_DISTANCE);

/////////////////////////////////////////////////////////////
// Function
/////////////////////////////////////////////////////////////

void before()
{
  sensorsTemp.begin();
}

void setup() {

  sensorsTemp.setWaitForConversion(false);

  // For battery lvel
  #if defined(__AVR_ATmega2560__)
    analogReference(INTERNAL1V1);
  #else
    analogReference(INTERNAL);
  #endif 
}

void presentation() {

  sendSketchInfo("Water tank node", "1.0", true);
  present(TEMP_SENSOR_ID, S_TEMP);
  present(WATER_LEVEL_SENSOR_ID, S_DISTANCE);
 
}

void loop() {

  sendTemperature();
  sendWaterLevel();
  sendBatteryPercentage();
  sendHeartbeat();
  sleep(SLEEP_TIME);

}

/**
 * This function send the percentage of water level in tank
 */
void sendWaterLevel(){
    //TODO
}


/**
 * This function send the percentage of battery for this node
 */
void sendBatteryPercentage(){
 
  int sensorValue = analogRead(PIN_BATTERY_SENSE);
  int batteryPcnt = sensorValue / 10;
  #ifdef MY_DEBUG
   float batteryV  = sensorValue * 0.003363075;
   Serial.print("Battery Voltage: ");
   Serial.print(batteryV);
   Serial.println(" V");

   Serial.print("Battery percent: ");
   Serial.print(batteryPcnt);
   Serial.println(" %");
  #endif

   if (oldBatteryPcnt != batteryPcnt) {
     // Power up radio after sleep
     sendBatteryLevel(batteryPcnt);
     oldBatteryPcnt = batteryPcnt;
   }
   
}

/**
 * This funciton read the temparature and send it
 */
void sendTemperature() {
  
  sensorsTemp.requestTemperatures();

  int16_t conversionTime = sensorsTemp.millisToWaitForConversion(sensorsTemp.getResolution());
  wait(conversionTime);

  float temperature = (sensorsTemp.getTempCByIndex(0) * 10.) / 10.;
 
  if (lastTemperature != temperature && temperature != -127.00 && temperature != 85.00) {
      send(tempMsg.setSensor(TEMP_SENSOR_ID).set(temperature, 1));
      lastTemperature = temperature;
  }
}

