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
#include <Bounce2.h>

/////////////////////////////////////////////////////////////
// Definition
/////////////////////////////////////////////////////////////

#define PIN_SENSOR_TEMP 3 //D3
#define PIN_BATTERY_SENSE 14 // A0
#define PIN_RELAY_GARAGE_DOOR 18 // A4
#define PIN_CONTACT_GARAGE_DOOR_OPEN 4 // D4
#define PIN_CONTACT_GARAGE_DOOR_CLOSE 5 // D5

#define TEMP_SENSOR_ID 0
#define RELAY_GARAGE_DOOR_ID 1
#define CONTACT_GARAGE_DOOR_OPEN_ID 2
#define CONTACT_GARAGE_DOOR_CLOSE_ID 3

#define MAX_ATTACHED_DS18B20 16

#ifdef MY_DEBUG
  #define SLEEP_TIME  1000
#else
  #define SLEEP_TIME  30000
#endif

#define RELAY_ON LOW  
#define RELAY_OFF HIGH

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

/////////////////// DOOR CONTACT /////////////////////////////

Bounce debouncerDoorOpen = Bounce(); 
MyMessage doorOpenMsg(CONTACT_GARAGE_DOOR_OPEN_ID, V_TRIPPED);
int previousStatusContactDoorOpen = -42;

Bounce debouncerDoorClose = Bounce(); 
MyMessage doorCloseMsg(CONTACT_GARAGE_DOOR_CLOSE_ID, V_TRIPPED);
int previousStatusContactDoorClose = -42;

/////////////////////////////////////////////////////////////
// Function
/////////////////////////////////////////////////////////////

void before()
{
  sensorsTemp.begin();
}

void setup() {

  pinMode(PIN_RELAY_GARAGE_DOOR, OUTPUT);
  digitalWrite(PIN_RELAY_GARAGE_DOOR, RELAY_OFF);

  pinMode(PIN_CONTACT_GARAGE_DOOR_OPEN, INPUT);
  digitalWrite(PIN_CONTACT_GARAGE_DOOR_OPEN, HIGH);
  debouncerDoorOpen.attach(PIN_CONTACT_GARAGE_DOOR_OPEN);
  debouncerDoorOpen.interval(5);

  pinMode(PIN_CONTACT_GARAGE_DOOR_CLOSE, INPUT);
  digitalWrite(PIN_CONTACT_GARAGE_DOOR_CLOSE, HIGH);
  debouncerDoorClose.attach(PIN_CONTACT_GARAGE_DOOR_CLOSE);
  debouncerDoorClose.interval(5);

  sensorsTemp.setWaitForConversion(false);

  // For battery lvel
  #if defined(__AVR_ATmega2560__)
   analogReference(INTERNAL1V1);
  #else
     analogReference(INTERNAL);
  #endif 
}

void presentation() {

  sendSketchInfo("Node garage", "1.0", true);
  present(TEMP_SENSOR_ID, S_TEMP);
  present(RELAY_GARAGE_DOOR_ID, S_BINARY);
  present(CONTACT_GARAGE_DOOR_OPEN_ID, S_DOOR);  
  present(CONTACT_GARAGE_DOOR_CLOSE_ID, S_DOOR);  
}

void loop() {

  sendStatusContactDoor();
  sendTemperature();
  sendBatteryPercentage();
  sendHeartbeat();
  sleep(SLEEP_TIME);

}

void receive(const MyMessage &message) {
  if (message.type == V_STATUS) {
    if(message.sensor == RELAY_GARAGE_DOOR_ID){
       digitalWrite(PIN_RELAY_GARAGE_DOOR, message.getBool() ? RELAY_ON : RELAY_OFF);
    }
  }
}

/**
 * This contact send the value 
 */
void sendStatusContactDoor() {

  debouncerDoorOpen.update();
  int value = debouncerDoorOpen.read();
  #ifdef MY_DEBUG
    Serial.print("Contact door open: ");
    Serial.println((value == HIGH) ? "activated" : "unactivated");
  #endif
  if (value != previousStatusContactDoorOpen) {
     send(doorOpenMsg.set(value == HIGH ? 1 : 0));
     previousStatusContactDoorOpen = value;
  }

  debouncerDoorClose.update();
  value = debouncerDoorClose.read();
  #ifdef MY_DEBUG
    Serial.print("Contact door close: ");
    Serial.println((value == HIGH) ? "activated" : "unactivated");
  #endif
  if (value != previousStatusContactDoorClose) {
     send(doorCloseMsg.set(value == HIGH ? 1 : 0));
     previousStatusContactDoorClose = value;
  }
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

