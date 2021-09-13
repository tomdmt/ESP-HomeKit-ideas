/*
 * WindowCovering.ino
 *
 *  Created on: 2021-07-23
 *  Last edit: 2021-08-21 (position first, then angle)
 *  2021-08-15 (added OTA and Telnetstream)
 *      Author: Tom Duan
 *
 * HAP section 8.45 Window Covering
 * An accessory contains a switch.
 *
 */

#include <Arduino.h>
#include <arduino_homekit_server.h>
#include "wifi_info.h"
#include <ESP_EEPROM.h>
//#include <Schedule.h>
#include <ESP8266mDNS.h>

#include <math.h>



#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <TelnetStream.h>

//bool announce() {
//  MDNS.announce();
//  return true;
//}


#include <IRremoteESP8266.h>
#include <IRsend.h>

#define kIrLed 9 // SD2
IRsend irsend(kIrLed);  // Set the GPIO to be used to sending the message.

#include <IRrecv.h>
#include <IRutils.h>
#define kRecvPin 10
IRrecv irrecv(kRecvPin);
decode_results results;

#include "DHT.h"   //https://github.com/adafruit/DHT-sensor-library
#define DHT22_PIN 2 // D4

DHT DHT(DHT22_PIN,DHT22);

#define LOG_D(fmt, ...)   printf_P(PSTR(fmt "\n") , ##__VA_ARGS__);

int step_number_angle = 0;
int step_number_position = 0;

uint8_t target_position;
uint8_t current_position;

int target_vertical_tilt_angle;
int current_vertical_tilt_angle;

boolean ok;

uint8_t Active;
uint8_t currActive;

float current_rotation_speed;
float rotation_speed;

uint8_t target_heating_cooling_state;
uint8_t current_heating_cooling_state;

float target_temperature;
float current_target_temperature;

int current_mode; //1:cool, 2: fan, 3: heat, 4: eco

unsigned long previousMillis = 0;
unsigned long interval = 30000;

struct myEEPROMstruct {
  uint8_t EEPROM_position;
  int EEPROM_angle;
  uint8_t EEPROM_Active;
  float EEPROM_rotation_speed;
  uint8_t EEPROM_heating_cooling_state;
  float EEPROM_target_temperature;
  int EEPROM_current_mode;
  int counter;
  int spacer;
  } defaultEepromVar, localEepromVar;

void setup() {
  irsend.begin();
  irrecv.enableIRIn();  // Start the receiver
  Serial.begin(74880);
  Serial.println();

  Serial.print("struct size0: ");
  Serial.println(sizeof(myEEPROMstruct));

  // Set up the initial (default) values for what is to be stored in EEPROM
  defaultEepromVar.EEPROM_position = 50;
  defaultEepromVar.EEPROM_angle = 90;
  defaultEepromVar.EEPROM_Active = 1;
  defaultEepromVar.EEPROM_rotation_speed = 75;
  defaultEepromVar.EEPROM_heating_cooling_state = 2;
  defaultEepromVar.EEPROM_target_temperature = 20;
  defaultEepromVar.EEPROM_current_mode = 1;
  defaultEepromVar.counter = 1;
  defaultEepromVar.spacer = 5;

  Serial.print("struct size1: ");
  Serial.println(sizeof(myEEPROMstruct));

  
  EEPROM.begin(sizeof(myEEPROMstruct));
  Serial.print("EEPROM% used: ");
  Serial.println(EEPROM.percentUsed());

  if(EEPROM.percentUsed()<0) {
    Serial.println("Blank Slate");
    EEPROM.put(0, defaultEepromVar);
    // write the data to EEPROM
    ok = EEPROM.commit();
    Serial.println((ok) ? "Commit OK" : "Commit failed");
  }


  EEPROM.get(0, localEepromVar); 

  Serial.print("counter0: ");
  Serial.println(localEepromVar.counter);
  Serial.print("initial EEPROM_position0: ");
  Serial.println(localEepromVar.EEPROM_position);
  Serial.print("initial EEPROM_angle0: ");
  Serial.println(localEepromVar.EEPROM_angle);
  Serial.print("initial EEPROM_spacer: ");
  Serial.println(localEepromVar.spacer);


//  if(localEepromVar.counter==1) {
//    EEPROM.put(0, defaultEepromVar);
//    // write the data to EEPROM
//    ok = EEPROM.commit();
//    Serial.println((ok) ? "Commit OK1" : "Commit failed1");
//    EEPROM.get(0, localEepromVar);
//  }
//  else{
//    localEepromVar.counter = 1;
//    EEPROM.put(0, localEepromVar);
//    ok = EEPROM.commit();
//    Serial.println((ok) ? "Commit OK2" : "Commit failed2");
//  }



  Serial.print("counter: ");
  Serial.println(localEepromVar.counter);
  Serial.print("initial EEPROM_position: ");
  Serial.println(localEepromVar.EEPROM_position);
  Serial.print("initial EEPROM_angle: ");
  Serial.println(localEepromVar.EEPROM_angle);


  current_position = localEepromVar.EEPROM_position;
  target_position = localEepromVar.EEPROM_position;

  target_vertical_tilt_angle = localEepromVar.EEPROM_angle;
  current_vertical_tilt_angle = localEepromVar.EEPROM_angle;

  Serial.print("initial current position: ");
  Serial.println(current_position);
  Serial.print("initial current angle: ");
  Serial.println(current_vertical_tilt_angle);
  

  Active = localEepromVar.EEPROM_Active;
  currActive = localEepromVar.EEPROM_Active;
  
  current_rotation_speed = localEepromVar.EEPROM_rotation_speed;
  rotation_speed = localEepromVar.EEPROM_rotation_speed;
  
  target_heating_cooling_state = localEepromVar.EEPROM_heating_cooling_state;

  if(localEepromVar.EEPROM_heating_cooling_state > 2){
    current_heating_cooling_state = 2;
  }
  else{
    current_heating_cooling_state = localEepromVar.EEPROM_heating_cooling_state;
  }
  
  target_temperature = localEepromVar.EEPROM_target_temperature;
  current_target_temperature = localEepromVar.EEPROM_target_temperature;
  
  current_mode = localEepromVar.EEPROM_current_mode; //1:cool, 2: fan, 3: heat, 4: eco
  
	wifi_connect(); // in wifi_info.h
	
	DHT.begin();
  //schedule_recurrent_function_us(announce, 5000000);

  // OTA section
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  //ArduinoOTA.setHostname("esp8266-Thermostat");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("OTA Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  TelnetStream.begin();

  my_homekit_setup();
}

void loop() {
//  unsigned long currentMillis = millis();
//  // if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
//  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >=interval)) {
//    Serial.print(millis());
//    Serial.println("Reconnecting to WiFi...");
//    WiFi.disconnect();
//    wifi_connect(); // in wifi_info.h
//    my_homekit_setup();
//    schedule_recurrent_function_us(announce, 5000000);
//    previousMillis = currentMillis;
//  }
  
  ArduinoOTA.handle();
	my_homekit_loop();

  switch (TelnetStream.read()) {
    case 'R':
    TelnetStream.stop();
    delay(100);
    ESP.restart();
      break;
    case 'C':
      TelnetStream.println("bye bye bye");
      TelnetStream.flush();
      TelnetStream.stop();
      break;



    case 'M':
      TelnetStream.println("mDNS announce");
      delay(100);
      MDNS.announce();
      TelnetStream.println("mDNS announce done");
      break;
    case 'W':
      TelnetStream.println("WiFi.disconnect() and wifi_connect()");
      delay(100);
      WiFi.disconnect();
      wifi_connect(); // in wifi_info.h
      TelnetStream.println("WiFi.disconnect() and wifi_connect() done");
      break;
    case 'S':
      TelnetStream.println("setup()");
      delay(100);
      WiFi.disconnect();
      setup();
      TelnetStream.println("setup() done");
      break;
    case 'T':
      TelnetStream.println("erasing HomeKit pairing data");
      delay(100);
      homekit_storage_reset(); // to remove the previous HomeKit pairing storage when you first run this new HomeKit example
      break;
  }
	delay(10);
}

//==============================
// HomeKit setup and loop
//==============================

// access your HomeKit characteristics defined in my_accessory.c
extern "C" homekit_server_config_t config;
extern "C" homekit_characteristic_t cha_target_position;
extern "C" homekit_characteristic_t cha_current_position;

extern "C" homekit_characteristic_t cha_target_vertical_tilt_angle;
extern "C" homekit_characteristic_t cha_current_vertical_tilt_angle;


extern "C" homekit_characteristic_t cha_current_temperature;
extern "C" homekit_characteristic_t cha_target_temperature;
extern "C" homekit_characteristic_t cha_current_heating_cooling_state;
extern "C" homekit_characteristic_t cha_target_heating_cooling_state;
extern "C" homekit_characteristic_t cha_temperature_display_units;
extern "C" homekit_characteristic_t cha_humidity;

extern "C" homekit_characteristic_t cha_active;
extern "C" homekit_characteristic_t cha_rotation_speed;


//static uint32_t next_heap_millis = 0;
static uint32_t next_report_millis = 0;

#define STEPPER_PIN_1 0
#define STEPPER_PIN_2 4
#define STEPPER_PIN_3 5
#define STEPPER_PIN_4 16

#define STEPPER_PIN_5 15
#define STEPPER_PIN_6 13
#define STEPPER_PIN_7 12
#define STEPPER_PIN_8 14



float temperature_value = DHT.readTemperature();
float humidity_value = DHT.readHumidity();

//Called when the target_position value is changed by iOS Home APP
void cha_target_position_setter(const homekit_value_t value) {
	target_position = value.int_value;
	cha_target_position.value.int_value = target_position;	//sync the value
	Serial.print("Target Position: ");
  Serial.println(target_position);

  if(target_position == 0){
    target_vertical_tilt_angle = 90;
    cha_target_vertical_tilt_angle.value.int_value = target_vertical_tilt_angle;  //sync the value
    homekit_characteristic_notify(&cha_target_vertical_tilt_angle, cha_target_vertical_tilt_angle.value);
  }
  else if(target_position == 100){
    target_position = 50;
    cha_target_position.value.int_value = target_position;  //sync the value
    homekit_characteristic_notify(&cha_target_position, cha_target_position.value);
    
    target_vertical_tilt_angle = -90;
    cha_target_vertical_tilt_angle.value.int_value = target_vertical_tilt_angle;  //sync the value
    homekit_characteristic_notify(&cha_target_vertical_tilt_angle, cha_target_vertical_tilt_angle.value);
  }
}

//Called when the target_vertical_tilt_angle value is changed by iOS Home APP
void cha_target_vertical_tilt_angle_setter(const homekit_value_t value) {
  target_vertical_tilt_angle = value.int_value;
  cha_target_vertical_tilt_angle.value.int_value = target_vertical_tilt_angle;  //sync the value
  Serial.print("Target Vertical Tilt Angle: ");
  Serial.println(target_vertical_tilt_angle);

  if(target_vertical_tilt_angle == -90){
    target_position = 50;
    cha_target_position.value.int_value = target_position;  //sync the value
    homekit_characteristic_notify(&cha_target_position, cha_target_position.value);
  }
}

//Called when the switch value is changed by iOS Home APP
void cha_active_setter(const homekit_value_t value) {
  Active = value.int_value;
  cha_active.value.int_value = Active;  //sync the value
  TelnetStream.print("Active: ");
  TelnetStream.println(Active+10);

  if(Active && !currActive){
    irsend.sendNEC(0x8166817E);
    delay(300);
    if(current_mode == 1){
      irsend.sendNEC(0x8166D926);
      delay(300);
    }
    currActive = 1;
    cha_active.value.int_value = Active;
    homekit_characteristic_notify(&cha_active, cha_active.value);

    if(current_mode == 1){ // cooling
    target_heating_cooling_state = 2; // cooling
    current_heating_cooling_state = 2; // cooling
    }
    else if(current_mode == 2){ // fan
    target_heating_cooling_state = 0; // off
    current_heating_cooling_state = 0; // off
    }
    else if(current_mode == 3){ // heating
    target_heating_cooling_state = 1; // heating
    current_heating_cooling_state = 1; // heating
    }
    else if(current_mode == 4){ // eco
    target_heating_cooling_state = 3; // auto
    current_heating_cooling_state = 2; // cooling
    }
    
    cha_target_heating_cooling_state.value.int_value = target_heating_cooling_state;
    homekit_characteristic_notify(&cha_target_heating_cooling_state, cha_target_heating_cooling_state.value);
    
    cha_current_heating_cooling_state.value.int_value = current_heating_cooling_state;
    homekit_characteristic_notify(&cha_current_heating_cooling_state, cha_current_heating_cooling_state.value);
    
  }
  else if(!Active == currActive){
    irsend.sendNEC(0x8166817E);
    delay(300);
    currActive = 0;
    cha_active.value.int_value = Active;
    homekit_characteristic_notify(&cha_active, cha_active.value);

    target_heating_cooling_state = 0;
    cha_target_heating_cooling_state.value.int_value = target_heating_cooling_state;
    homekit_characteristic_notify(&cha_target_heating_cooling_state, cha_target_heating_cooling_state.value);
    
    current_heating_cooling_state = 0;
    cha_current_heating_cooling_state.value.int_value = current_heating_cooling_state;
    homekit_characteristic_notify(&cha_current_heating_cooling_state, cha_current_heating_cooling_state.value);
    }

  localEepromVar.counter++;
  localEepromVar.EEPROM_heating_cooling_state = target_heating_cooling_state;
  localEepromVar.EEPROM_Active = currActive;
  localEepromVar.EEPROM_current_mode = current_mode;
  localEepromVar.EEPROM_rotation_speed = rotation_speed;
  localEepromVar.EEPROM_target_temperature = current_target_temperature;
  EEPROM.put(0, localEepromVar);
  ok = EEPROM.commit();
  Serial.println((ok) ? "commit OK" : "Commit failed");
}

void cha_target_heating_cooling_state_setter(const homekit_value_t value) {
  target_heating_cooling_state = value.int_value;
  cha_target_heating_cooling_state.value.int_value = target_heating_cooling_state;  //sync the value
  TelnetStream.print("target_heating_cooling_state: ");
  TelnetStream.println(target_heating_cooling_state);
  TelnetStream.print("current_mode1: ");
  TelnetStream.println(current_mode);

  if(target_heating_cooling_state == 0 && Active){ //off
    TelnetStream.println("fan only");
    while(current_mode != 2) {
      irsend.sendNEC(0x8166D926);
      delay(300);
      current_mode++;
      if(current_mode > 4){
        current_mode = 1;
      }
    }
    current_heating_cooling_state = 0; //off
    cha_current_heating_cooling_state.value.int_value = current_heating_cooling_state;
    homekit_characteristic_notify(&cha_current_heating_cooling_state, cha_current_heating_cooling_state.value);
  }
  else if(target_heating_cooling_state != 0){
    if(!Active){
      irsend.sendNEC(0x8166817E);
      delay(300);
      if(current_mode == 1){
        irsend.sendNEC(0x8166D926);
        delay(300);
      }
      Active = 1;
      currActive = 1;
      cha_active.value.int_value = Active;
      homekit_characteristic_notify(&cha_active, cha_active.value);

      if(rotation_speed < 10){
      rotation_speed = 10;
      current_rotation_speed = rotation_speed;

      cha_rotation_speed.value.float_value = rotation_speed;
      homekit_characteristic_notify(&cha_rotation_speed, cha_rotation_speed.value);  
      }
    }
    if(target_heating_cooling_state == 1){ //heating
      TelnetStream.println("heating");
      while(current_mode != 3) { //heat
        irsend.sendNEC(0x8166D926);
        delay(300);
        current_mode++;
        TelnetStream.print("current_mode2: ");
        TelnetStream.println(current_mode);
        if(current_mode > 4){
          current_mode = 1;
        }
      }
      current_heating_cooling_state = 1; //heating
      cha_current_heating_cooling_state.value.int_value = current_heating_cooling_state;
      homekit_characteristic_notify(&cha_current_heating_cooling_state, cha_current_heating_cooling_state.value);
    }
    else if(target_heating_cooling_state == 2){ //cooling
      TelnetStream.println("cooling");
      while(current_mode != 1) { //cool
        irsend.sendNEC(0x8166D926);
        delay(300);
        current_mode++;
        if(current_mode > 4){
          current_mode = 1;
        }
      }
      current_heating_cooling_state = 2; //cooling
      cha_current_heating_cooling_state.value.int_value = current_heating_cooling_state;
      homekit_characteristic_notify(&cha_current_heating_cooling_state, cha_current_heating_cooling_state.value);
    }
    else if(target_heating_cooling_state == 3){ //auto
      TelnetStream.println("eco");
      while(current_mode != 4) { //eco
        irsend.sendNEC(0x8166D926);
        delay(300);
        current_mode++;
        if(current_mode > 4){
          current_mode = 1;
        }
      }
      current_heating_cooling_state = 2; // cooling
      cha_current_heating_cooling_state.value.int_value = current_heating_cooling_state;
      homekit_characteristic_notify(&cha_current_heating_cooling_state, cha_current_heating_cooling_state.value);
    }
  }

  localEepromVar.counter++;
  localEepromVar.EEPROM_heating_cooling_state = target_heating_cooling_state;
  localEepromVar.EEPROM_Active = currActive;
  localEepromVar.EEPROM_current_mode = current_mode;
  localEepromVar.EEPROM_rotation_speed = rotation_speed;
  localEepromVar.EEPROM_target_temperature = current_target_temperature;
  EEPROM.put(0, localEepromVar);
  ok = EEPROM.commit();
  Serial.println((ok) ? "commit OK" : "Commit failed");
}


//Called when the fan value is changed by iOS Home APP
void cha_rotation_speed_setter(const homekit_value_t value) {
  rotation_speed = value.float_value;
  cha_rotation_speed.value.float_value = rotation_speed;  //sync the value
  if(!Active){
    irsend.sendNEC(0x8166817E);
    delay(300);
    if(current_mode == 1){
      irsend.sendNEC(0x8166D926);
      delay(300);
    }
    Active = 1;
    currActive = 1;
    cha_active.value.int_value = Active;
    homekit_characteristic_notify(&cha_active, cha_active.value);


    if(current_mode == 1){ // cooling
    target_heating_cooling_state = 2; // cooling
    current_heating_cooling_state = 2; // cooling
    }
    else if(current_mode == 2){ // fan
    target_heating_cooling_state = 0; // off
    current_heating_cooling_state = 0; // off
    }
    else if(current_mode == 3){ // heating
    target_heating_cooling_state = 1; // heating
    current_heating_cooling_state = 1; // heating
    }
    else if(current_mode == 4){ // eco
    target_heating_cooling_state = 3; // cooling
    current_heating_cooling_state = 2; // cooling
    }

    
    cha_target_heating_cooling_state.value.int_value = target_heating_cooling_state;
    homekit_characteristic_notify(&cha_target_heating_cooling_state, cha_target_heating_cooling_state.value);
    
    cha_current_heating_cooling_state.value.int_value = current_heating_cooling_state;
    homekit_characteristic_notify(&cha_current_heating_cooling_state, cha_current_heating_cooling_state.value);
    }
  if(rotation_speed <50 && current_rotation_speed >= 50){
    irsend.sendNEC(0x81669966);
    current_rotation_speed = rotation_speed;

    cha_rotation_speed.value.float_value = rotation_speed;
    homekit_characteristic_notify(&cha_rotation_speed, cha_rotation_speed.value);
  }
  else if(rotation_speed >=50 && current_rotation_speed <50){
    irsend.sendNEC(0x81669966);
    current_rotation_speed = rotation_speed;

    cha_rotation_speed.value.float_value = rotation_speed;
    homekit_characteristic_notify(&cha_rotation_speed, cha_rotation_speed.value);
  }

  TelnetStream.print("rotation_speed: ");
  TelnetStream.println(rotation_speed);

  localEepromVar.counter++;
  localEepromVar.EEPROM_heating_cooling_state = target_heating_cooling_state;
  localEepromVar.EEPROM_Active = currActive;
  localEepromVar.EEPROM_current_mode = current_mode;
  localEepromVar.EEPROM_rotation_speed = rotation_speed;
  localEepromVar.EEPROM_target_temperature = current_target_temperature;
  EEPROM.put(0, localEepromVar);
  ok = EEPROM.commit();
  Serial.println((ok) ? "commit OK" : "Commit failed");
}


//Called when the target temperature value is changed by iOS Home APP
void cha_target_temperature_setter(const homekit_value_t value) {
  target_temperature = value.float_value;
  cha_target_temperature.value.float_value = target_temperature;  //sync the value
  Serial.print("target_temperature: ");
  Serial.println(target_temperature);
  TelnetStream.print("Active: ");
  TelnetStream.println(Active);
  
  if(target_temperature > current_target_temperature && Active){
    for(int i = 0; i < round((target_temperature*9/5+32) - (current_target_temperature*9/5+32)); i++) {
      irsend.sendNEC(0x8166A15E);
      delay(300);
    }
    current_target_temperature = target_temperature;
    }
  else if(target_temperature < current_target_temperature && Active){
    for(int i = 0; i < round((current_target_temperature*9/5+32) - (target_temperature*9/5+32)); i++) {
      irsend.sendNEC(0x816651AE);
      delay(300);
    }
    current_target_temperature = target_temperature;
    }

  localEepromVar.counter++;
  localEepromVar.EEPROM_heating_cooling_state = target_heating_cooling_state;
  localEepromVar.EEPROM_Active = currActive;
  localEepromVar.EEPROM_current_mode = current_mode;
  localEepromVar.EEPROM_rotation_speed = rotation_speed;
  localEepromVar.EEPROM_target_temperature = current_target_temperature;
  EEPROM.put(0, localEepromVar);
  ok = EEPROM.commit();
  Serial.println((ok) ? "commit OK" : "Commit failed");
}


void OneStepAngle(bool dir){
    if(dir){
switch(step_number_angle){
  case 0:
  digitalWrite(STEPPER_PIN_1, HIGH);
  digitalWrite(STEPPER_PIN_2, LOW);
  digitalWrite(STEPPER_PIN_3, LOW);
  digitalWrite(STEPPER_PIN_4, HIGH);
  break;
  case 1:
  digitalWrite(STEPPER_PIN_1, HIGH);
  digitalWrite(STEPPER_PIN_2, HIGH);
  digitalWrite(STEPPER_PIN_3, LOW);
  digitalWrite(STEPPER_PIN_4, LOW);
  break;
  case 2:
  digitalWrite(STEPPER_PIN_1, LOW);
  digitalWrite(STEPPER_PIN_2, HIGH);
  digitalWrite(STEPPER_PIN_3, HIGH);
  digitalWrite(STEPPER_PIN_4, LOW);
  break;
  case 3:
  digitalWrite(STEPPER_PIN_1, LOW);
  digitalWrite(STEPPER_PIN_2, LOW);
  digitalWrite(STEPPER_PIN_3, HIGH);
  digitalWrite(STEPPER_PIN_4, HIGH);
  break;
} 
  }else{
    switch(step_number_angle){
  case 0:
  digitalWrite(STEPPER_PIN_1, HIGH);
  digitalWrite(STEPPER_PIN_2, LOW);
  digitalWrite(STEPPER_PIN_3, LOW);
  digitalWrite(STEPPER_PIN_4, HIGH);
  break;
  case 1:
  digitalWrite(STEPPER_PIN_1, LOW);
  digitalWrite(STEPPER_PIN_2, LOW);
  digitalWrite(STEPPER_PIN_3, HIGH);
  digitalWrite(STEPPER_PIN_4, HIGH);
  break;
  case 2:
  digitalWrite(STEPPER_PIN_1, LOW);
  digitalWrite(STEPPER_PIN_2, HIGH);
  digitalWrite(STEPPER_PIN_3, HIGH);
  digitalWrite(STEPPER_PIN_4, LOW);
  break;
  case 3:
  digitalWrite(STEPPER_PIN_1, HIGH);
  digitalWrite(STEPPER_PIN_2, HIGH);
  digitalWrite(STEPPER_PIN_3, LOW);
  digitalWrite(STEPPER_PIN_4, LOW);
 
  
} 
  }
step_number_angle++;
  if(step_number_angle > 3){
    step_number_angle = 0;
  }
}


void OneStepPosition(bool dir){
    if(dir){
switch(step_number_position){
  case 0:
  digitalWrite(STEPPER_PIN_5, HIGH);
  digitalWrite(STEPPER_PIN_6, LOW);
  digitalWrite(STEPPER_PIN_7, LOW);
  digitalWrite(STEPPER_PIN_8, HIGH);
  break;
  case 1:
  digitalWrite(STEPPER_PIN_5, HIGH);
  digitalWrite(STEPPER_PIN_6, HIGH);
  digitalWrite(STEPPER_PIN_7, LOW);
  digitalWrite(STEPPER_PIN_8, LOW);
  break;
  case 2:
  digitalWrite(STEPPER_PIN_5, LOW);
  digitalWrite(STEPPER_PIN_6, HIGH);
  digitalWrite(STEPPER_PIN_7, HIGH);
  digitalWrite(STEPPER_PIN_8, LOW);
  break;
  case 3:
  digitalWrite(STEPPER_PIN_5, LOW);
  digitalWrite(STEPPER_PIN_6, LOW);
  digitalWrite(STEPPER_PIN_7, HIGH);
  digitalWrite(STEPPER_PIN_8, HIGH);
  break;
} 
  }else{
    switch(step_number_position){
  case 0:
  digitalWrite(STEPPER_PIN_5, HIGH);
  digitalWrite(STEPPER_PIN_6, LOW);
  digitalWrite(STEPPER_PIN_7, LOW);
  digitalWrite(STEPPER_PIN_8, HIGH);
  break;
  case 1:
  digitalWrite(STEPPER_PIN_5, LOW);
  digitalWrite(STEPPER_PIN_6, LOW);
  digitalWrite(STEPPER_PIN_7, HIGH);
  digitalWrite(STEPPER_PIN_8, HIGH);
  break;
  case 2:
  digitalWrite(STEPPER_PIN_5, LOW);
  digitalWrite(STEPPER_PIN_6, HIGH);
  digitalWrite(STEPPER_PIN_7, HIGH);
  digitalWrite(STEPPER_PIN_8, LOW);
  break;
  case 3:
  digitalWrite(STEPPER_PIN_5, HIGH);
  digitalWrite(STEPPER_PIN_6, HIGH);
  digitalWrite(STEPPER_PIN_7, LOW);
  digitalWrite(STEPPER_PIN_8, LOW);
 
  
} 
  }
step_number_position++;
  if(step_number_position > 3){
    step_number_position = 0;
  }
}


void my_homekit_setup() {
  // chain pins
	pinMode(STEPPER_PIN_1, OUTPUT);
  pinMode(STEPPER_PIN_2, OUTPUT);
  pinMode(STEPPER_PIN_3, OUTPUT);
  pinMode(STEPPER_PIN_4, OUTPUT);
  // rope pins
  pinMode(STEPPER_PIN_5, OUTPUT);
  pinMode(STEPPER_PIN_6, OUTPUT);
  pinMode(STEPPER_PIN_7, OUTPUT);
  pinMode(STEPPER_PIN_8, OUTPUT);
  // DHT22
  pinMode(DHT22_PIN, INPUT);

  
  cha_current_position.value.int_value = current_position;
  cha_target_position.value.int_value = target_position;
  cha_current_vertical_tilt_angle.value.int_value = current_vertical_tilt_angle;
  cha_target_vertical_tilt_angle.value.int_value = target_vertical_tilt_angle;

  homekit_characteristic_notify(&cha_current_position, cha_current_position.value);
  homekit_characteristic_notify(&cha_target_position, cha_target_position.value);

  homekit_characteristic_notify(&cha_current_vertical_tilt_angle, cha_current_vertical_tilt_angle.value);
  homekit_characteristic_notify(&cha_target_vertical_tilt_angle, cha_target_vertical_tilt_angle.value);


  cha_active.value.int_value = Active;
  cha_rotation_speed.value.float_value = rotation_speed;
  cha_target_heating_cooling_state.value.int_value = target_heating_cooling_state;
  cha_target_temperature.value.float_value = target_temperature;
//  cha_current_fan_state.value.int_value = current_fan_state;


  homekit_characteristic_notify(&cha_active, cha_active.value);
  homekit_characteristic_notify(&cha_rotation_speed, cha_rotation_speed.value);
  homekit_characteristic_notify(&cha_target_heating_cooling_state, cha_target_heating_cooling_state.value);
  homekit_characteristic_notify(&cha_target_temperature, cha_target_temperature.value);
//  homekit_characteristic_notify(&cha_current_fan_state, cha_current_fan_state.value);

	//Add the .setter function to get the switch-event sent from iOS Home APP.
	//The .setter should be added before arduino_homekit_setup.
	//HomeKit sever uses the .setter_ex internally, see homekit_accessories_init function.
	//Maybe this is a legacy design issue in the original esp-homekit library,
	//and I have no reason to modify this "feature".
	cha_target_position.setter = cha_target_position_setter;
  cha_target_vertical_tilt_angle.setter = cha_target_vertical_tilt_angle_setter;

  cha_active.setter = cha_active_setter;
  cha_rotation_speed.setter = cha_rotation_speed_setter;
  cha_target_heating_cooling_state.setter = cha_target_heating_cooling_state_setter;

  cha_target_temperature.setter = cha_target_temperature_setter;
  
	arduino_homekit_setup(&config);

	//report the switch value to HomeKit if it is changed (e.g. by a physical button)
	//bool switch_is_on = true/false;
	//cha_switch_on.value.bool_value = switch_is_on;
	//homekit_characteristic_notify(&cha_switch_on, cha_switch_on.value);
}

void my_homekit_loop() {
  arduino_homekit_loop();
  const uint32_t t = millis();
  if (t > next_report_millis) {
    // report sensor values every 10 seconds
    next_report_millis = t + 5 * 1000;
    my_homekit_report();
  }

  if (irrecv.decode(&results)) {

    switch(results.value){
      case 0x8166817E: //power
      irsend.sendNEC(0x8166817E);
      delay(300);
      TelnetStream.print("Active: ");
      TelnetStream.println(Active+100);
    
      if(!currActive){
        
        if(current_mode == 1){
          irsend.sendNEC(0x8166D926);
        }
        currActive = 1;
        Active = 1;
        cha_active.value.int_value = Active;
        homekit_characteristic_notify(&cha_active, cha_active.value);
    
        if(current_mode == 1){ // cooling
        target_heating_cooling_state = 2; // cooling
        current_heating_cooling_state = 2; // cooling
        }
        else if(current_mode == 2){ // fan
        target_heating_cooling_state = 0; // off
        current_heating_cooling_state = 0; // off
        }
        else if(current_mode == 3){ // heating
        target_heating_cooling_state = 1; // heating
        current_heating_cooling_state = 1; // heating
        }
        else if(current_mode == 4){ // eco
        target_heating_cooling_state = 3; // cooling
        current_heating_cooling_state = 2; // cooling
        }
    
        
        cha_target_heating_cooling_state.value.int_value = target_heating_cooling_state;
        homekit_characteristic_notify(&cha_target_heating_cooling_state, cha_target_heating_cooling_state.value);
        
        cha_current_heating_cooling_state.value.int_value = current_heating_cooling_state;
        homekit_characteristic_notify(&cha_current_heating_cooling_state, cha_current_heating_cooling_state.value);
        
      }
      else if(currActive){
        currActive = 0;
        Active = 0;
        cha_active.value.int_value = Active;
        homekit_characteristic_notify(&cha_active, cha_active.value);
    
        target_heating_cooling_state = 0;
        cha_target_heating_cooling_state.value.int_value = target_heating_cooling_state;
        homekit_characteristic_notify(&cha_target_heating_cooling_state, cha_target_heating_cooling_state.value);
        
        current_heating_cooling_state = 0;
        cha_current_heating_cooling_state.value.int_value = current_heating_cooling_state;
        homekit_characteristic_notify(&cha_current_heating_cooling_state, cha_current_heating_cooling_state.value);
      }

      break;
      case 0x816651AE: //tempDown

      if(Active){
        irsend.sendNEC(0x816651AE);

        target_temperature = (((target_temperature*9/5+32)-1) - 32) * 5/9 ;
        if(target_temperature < 15.5555){
          target_temperature = 15.5555;  
        }
        current_target_temperature = target_temperature;
        cha_target_temperature.value.float_value = target_temperature;  //sync the value
        homekit_characteristic_notify(&cha_target_temperature, cha_target_temperature.value);
        Serial.print("target_temperature: ");
        Serial.println(target_temperature);
        TelnetStream.print("Active: ");
        TelnetStream.println(Active);
        
      }

      break;
      case 0x8166A15E: //tempUp
      
      if(Active){
        irsend.sendNEC(0x8166A15E);

        target_temperature = (((target_temperature*9/5+32)+1) - 32) * 5/9 ;
        if(target_temperature > 30){
          target_temperature = 30;  
        }
        current_target_temperature = target_temperature;
        cha_target_temperature.value.float_value = target_temperature;  //sync the value
        homekit_characteristic_notify(&cha_target_temperature, cha_target_temperature.value);
        Serial.print("target_temperature: ");
        Serial.println(target_temperature);
        TelnetStream.print("Active: ");
        TelnetStream.println(Active);
        
      }

      break;
      case 0x81669966: //Fan Speed
      if(Active){
        irsend.sendNEC(0x81669966);

        if(current_rotation_speed >= 50){
          rotation_speed = 25;
          current_rotation_speed = rotation_speed;
        }
        else if(current_rotation_speed < 50){
          rotation_speed = 75;
          current_rotation_speed = rotation_speed;
        }

        cha_rotation_speed.value.float_value = rotation_speed;
        homekit_characteristic_notify(&cha_rotation_speed, cha_rotation_speed.value);
        TelnetStream.print("rotation_speed: ");
        TelnetStream.println(rotation_speed);
      }
      

      break;
      case 0x8166D926: //mode
      if(Active){
        irsend.sendNEC(0x8166D926);
        current_mode++;
        if(current_mode > 4){
          current_mode = 1;
        }
        if(current_mode == 1){ // cooling
          target_heating_cooling_state = 2; // cooling
          current_heating_cooling_state = 2; // cooling
        }
        else if(current_mode == 2){ // fan
          target_heating_cooling_state = 0; // off
          current_heating_cooling_state = 0; // off
        }
        else if(current_mode == 3){ // heating
          target_heating_cooling_state = 1; // heating
          current_heating_cooling_state = 1; // heating
        }
        else if(current_mode == 4){ // eco
          target_heating_cooling_state = 3; // cooling
          current_heating_cooling_state = 2; // cooling
        }
      
          
        cha_target_heating_cooling_state.value.int_value = target_heating_cooling_state;
        homekit_characteristic_notify(&cha_target_heating_cooling_state, cha_target_heating_cooling_state.value);
        
        cha_current_heating_cooling_state.value.int_value = current_heating_cooling_state;
        homekit_characteristic_notify(&cha_current_heating_cooling_state, cha_current_heating_cooling_state.value);
      }

      break;
      
    }
    localEepromVar.counter++;
    localEepromVar.EEPROM_heating_cooling_state = target_heating_cooling_state;
    localEepromVar.EEPROM_Active = currActive;
    localEepromVar.EEPROM_current_mode = current_mode;
    localEepromVar.EEPROM_rotation_speed = rotation_speed;
    localEepromVar.EEPROM_target_temperature = current_target_temperature;
    EEPROM.put(0, localEepromVar);
    ok = EEPROM.commit();
    Serial.println((ok) ? "commit OK" : "Commit failed");
  
    irrecv.resume();  // Receive the next value
  }

//  if (t > next_heap_millis) {
//    // show heap info every 5 seconds
//    next_heap_millis = t + 5 * 1000;
//    LOG_D("Free heap: %d, HomeKit clients: %d",
//        ESP.getFreeHeap(), arduino_homekit_connected_clients_count());
//
//  }
//    cha_active.value.int_value = Active;
//    homekit_characteristic_notify(&cha_active, cha_active.value);
//
//    cha_rotation_speed.value.float_value = rotation_speed;
//    homekit_characteristic_notify(&cha_rotation_speed, cha_rotation_speed.value);
//
//    current_fan_state = Active * 2;
//    cha_current_fan_state.value.int_value = current_fan_state;
//    homekit_characteristic_notify(&cha_current_fan_state, cha_current_fan_state.value);
  
  if(target_position == current_position){
    digitalWrite(STEPPER_PIN_5, LOW);
    digitalWrite(STEPPER_PIN_6, LOW);
    digitalWrite(STEPPER_PIN_7, LOW);
    digitalWrite(STEPPER_PIN_8, LOW);

    EEPROM.get(0, localEepromVar);
    if(current_position != localEepromVar.EEPROM_position){
      Serial.print("current position after if: ");
      Serial.println(current_position);
      Serial.print("EEPROM: ");
      Serial.println(localEepromVar.EEPROM_position);

      localEepromVar.counter++;
      localEepromVar.EEPROM_position = current_position;
      EEPROM.put(0, localEepromVar);
      ok = EEPROM.commit();
      Serial.println((ok) ? "commit OK" : "Commit failed");

      EEPROM.get(0, localEepromVar);
      Serial.print("counter: ");
      Serial.println(localEepromVar.counter);
      Serial.print("current position after commit: ");
      Serial.println(current_position);
      Serial.print("EEPROM: ");
      Serial.println(localEepromVar.EEPROM_position);
    }
  }

  if(target_vertical_tilt_angle == current_vertical_tilt_angle){
    digitalWrite(STEPPER_PIN_1, LOW);
    digitalWrite(STEPPER_PIN_2, LOW);
    digitalWrite(STEPPER_PIN_3, LOW);
    digitalWrite(STEPPER_PIN_4, LOW);

    EEPROM.get(0, localEepromVar);
    if(current_vertical_tilt_angle != localEepromVar.EEPROM_angle){
      Serial.print("current angle after if: ");
      Serial.println(current_vertical_tilt_angle);
      Serial.print("EEPROM: ");
      Serial.println(localEepromVar.EEPROM_angle);

      localEepromVar.counter++;
      localEepromVar.EEPROM_angle = current_vertical_tilt_angle;
      EEPROM.put(0, localEepromVar);
      ok = EEPROM.commit();
      Serial.println((ok) ? "commit OK" : "Commit failed");

      EEPROM.get(0, localEepromVar);
      Serial.print("counter: ");
      Serial.println(localEepromVar.counter);
      Serial.print("current angle after commit: ");
      Serial.println(current_vertical_tilt_angle);
      Serial.print("EEPROM: ");
      Serial.println(localEepromVar.EEPROM_angle);
    }
  }

  // position
  if(target_position > current_position){
    for(int i = 0; i<320; i++) {
      OneStepPosition(true);
      delay(3);
    }

    current_position++;
    Serial.print("Target Position: ");
    Serial.println(target_position);
    Serial.print("Current Position: ");
    Serial.println(current_position);
    //report the lock-mechanism current-sate to HomeKit
    cha_current_position.value.int_value = current_position;
    homekit_characteristic_notify(&cha_current_position, cha_current_position.value);
  }
  if(target_position < current_position && target_position != 0){
    for(int i = 0; i<320; i++) {
      OneStepPosition(false);
      delay(3);
    }

    current_position--;
    Serial.print("Target Position: ");
    Serial.println(target_position);
    Serial.print("Current Position: ");
    Serial.println(current_position);
    //report the lock-mechanism current-sate to HomeKit
    cha_current_position.value.int_value = current_position;
    homekit_characteristic_notify(&cha_current_position, cha_current_position.value);
  }
  if(target_position < current_position && target_position == 0 && target_vertical_tilt_angle == current_vertical_tilt_angle){
    for(int i = 0; i<325; i++) {
      OneStepPosition(false);
      delay(3);
    }

    current_position--;
    Serial.println("Target Position: Closing Completely");
    Serial.print("Current Position: ");
    Serial.println(current_position);
    //report the lock-mechanism current-sate to HomeKit
    cha_current_position.value.int_value = current_position;
    homekit_characteristic_notify(&cha_current_position, cha_current_position.value);
  }

  
  // Vertical Angle
  if(target_vertical_tilt_angle > current_vertical_tilt_angle){

    for(int i = 0; i<313; i++) {
      OneStepAngle(true);
      delay(3);
    }

    current_vertical_tilt_angle++;
    Serial.print("Target vertical_tilt_angle: ");
    Serial.println(target_vertical_tilt_angle);
    Serial.print("Current vertical_tilt_angle: ");
    Serial.println(current_vertical_tilt_angle);
    //report the lock-mechanism current-sate to HomeKit
    cha_current_vertical_tilt_angle.value.int_value = current_vertical_tilt_angle;
    homekit_characteristic_notify(&cha_current_vertical_tilt_angle, cha_current_vertical_tilt_angle.value);
  }
  if(target_vertical_tilt_angle < current_vertical_tilt_angle && target_vertical_tilt_angle != -90){
 
    for(int i = 0; i<313; i++) {
      OneStepAngle(false);
      delay(3);
    }

    current_vertical_tilt_angle--;
    Serial.print("Target vertical_tilt_angle: ");
    Serial.println(target_vertical_tilt_angle);
    Serial.print("Current vertical_tilt_angle: ");
    Serial.println(current_vertical_tilt_angle);
    //report the lock-mechanism current-sate to HomeKit
    cha_current_vertical_tilt_angle.value.int_value = current_vertical_tilt_angle;
    homekit_characteristic_notify(&cha_current_vertical_tilt_angle, cha_current_vertical_tilt_angle.value);
  }
  if(target_vertical_tilt_angle < current_vertical_tilt_angle && target_vertical_tilt_angle == -90){
    for(int i = 0; i<313; i++) {
      OneStepAngle(false);
      delay(3);
    }

    current_vertical_tilt_angle--;
    Serial.println("Target vertical_tilt_angle: Closing Completely");
    Serial.print("Current vertical_tilt_angle: ");
    Serial.println(current_vertical_tilt_angle);
    //report the lock-mechanism current-sate to HomeKit
    cha_current_vertical_tilt_angle.value.int_value = current_vertical_tilt_angle;
    homekit_characteristic_notify(&cha_current_vertical_tilt_angle, cha_current_vertical_tilt_angle.value);
  }

}

void my_homekit_report() {
  temperature_value = DHT.readTemperature();
  humidity_value = DHT.readHumidity();

  if(isnan(temperature_value)){
    temperature_value=10.0;
  }
  if(isnan(humidity_value)){
    humidity_value=10.0;
  }
  
  cha_current_temperature.value.float_value = temperature_value;
  cha_humidity.value.float_value = humidity_value;
  
  //LOG_D("Current temperature: %.1f", temperature_value);
  //LOG_D("Current hum: %.1f", humidity_value);
  
  homekit_characteristic_notify(&cha_current_temperature, cha_current_temperature.value);
  homekit_characteristic_notify(&cha_humidity, cha_humidity.value);
}
