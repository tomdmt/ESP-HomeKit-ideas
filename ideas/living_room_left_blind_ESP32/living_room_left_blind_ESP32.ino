/*
 * WindowCovering.ino
 *
 *  Created on: 2021-07-23
 *  Last edit: 2021-08-15 (added OTA and Telnetstream)
 *      Author: Tom Duan
 *
 * HAP section 8.45 Window Covering
 * An accessory contains a switch.
 *
 * This example shows how to:
 * 1. define a switch accessory and its characteristics (in my_accessory.c).
 * 2. get the switch-event sent from iOS Home APP.
 * 3. report the switch value to HomeKit.
 *
 * You should:
 * 1. read and use the Example01_TemperatureSensor with detailed comments
 *    to know the basic concept and usage of this library before other examples。
 * 2. erase the full flash or call homekit_storage_reset() in setup()
 *    to remove the previous HomeKit pairing storage and
 *    enable the pairing with the new accessory of this new HomeKit example.
 */

#include <Arduino.h>
#include <arduino_homekit_server.h>
#include "wifi_info.h"
#include <Preferences.h>

Preferences preferences;
//#include <Schedule.h>

#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <TelnetStream.h>
//
//bool announce() {
//  MDNS.announce();
//  return true;
//}

#define LOG_D(fmt, ...)   printf_P(PSTR(fmt "\n") , ##__VA_ARGS__);

int step_number_angle = 0;
int step_number_position = 0;

uint8_t target_position;
uint8_t current_position;
float positionStepSize;

int target_vertical_tilt_angle;
int current_vertical_tilt_angle;
float angleStepSize;

uint32_t chipId = 0;

unsigned long previousMillis = 0;
unsigned long interval = 30000;

int moved = 0;

void setup() {
  Serial.begin(115200);
  Serial.println();

  for(int i=0; i<17; i=i+8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }

  // Open Preferences with my-app namespace. Each application module, library, etc
  // has to use a namespace name to prevent key name collisions. We will open storage in
  // RW-mode (second parameter has to be false).
  // Note: Namespace name is limited to 15 chars.
  preferences.begin("state", false);
  // Remove all preferences under the opened namespace
  // preferences.clear();

  // Or remove the counter key only
  //preferences.remove("key");

  // Set up the initial (default) values for what is to be stored in NVS
  current_position = preferences.getUInt("position", 50);
  target_position = preferences.getUInt("position", 50);

  target_vertical_tilt_angle = preferences.getInt("angle", 90);
  current_vertical_tilt_angle = preferences.getInt("angle", 90);

  // Close the Preferences
  // preferences.end();

  Serial.print("initial current position: ");
  Serial.println(current_position);
  Serial.print("initial current angle: ");
  Serial.println(current_vertical_tilt_angle);
  
	wifi_connect(); // in wifi_info.h


  //schedule_recurrent_function_us(announce, 5000000);

  // OTA section
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  //ArduinoOTA.setHostname("esp8266-left-living-room-blind");

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
  unsigned long currentMillis = millis();
  // if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >=interval)) {
    Serial.print(millis());
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    wifi_connect(); // in wifi_info.h
    previousMillis = currentMillis;
  }
  
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
      //MDNS.announce();
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


extern "C" char serial[16];


//static uint32_t next_heap_millis = 0;
static uint32_t next_report_millis = 0;

#define STEPPER_PIN_1 26
#define STEPPER_PIN_2 25
#define STEPPER_PIN_3 33
#define STEPPER_PIN_4 32

#define STEPPER_PIN_5 13
#define STEPPER_PIN_6 12
#define STEPPER_PIN_7 14
#define STEPPER_PIN_8 27




//Called when the target_position value is changed by iOS Home APP
void cha_target_position_setter(const homekit_value_t value) {
	target_position = value.int_value;
	cha_target_position.value.int_value = target_position;	//sync the value
	Serial.print("Target Position: ");
  Serial.println(target_position);
}

//Called when the target_vertical_tilt_angle value is changed by iOS Home APP
void cha_target_vertical_tilt_angle_setter(const homekit_value_t value) {
  target_vertical_tilt_angle = value.int_value;
  cha_target_vertical_tilt_angle.value.int_value = target_vertical_tilt_angle;  //sync the value
  Serial.print("Target Vertical Tilt Angle: ");
  Serial.println(target_vertical_tilt_angle);
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


  
  cha_current_position.value.int_value = current_position;
  cha_target_position.value.int_value = target_position;
  cha_current_vertical_tilt_angle.value.int_value = current_vertical_tilt_angle;
  cha_target_vertical_tilt_angle.value.int_value = target_vertical_tilt_angle;

  homekit_characteristic_notify(&cha_current_position, cha_current_position.value);
  homekit_characteristic_notify(&cha_target_position, cha_target_position.value);

  homekit_characteristic_notify(&cha_current_vertical_tilt_angle, cha_current_vertical_tilt_angle.value);
  homekit_characteristic_notify(&cha_target_vertical_tilt_angle, cha_target_vertical_tilt_angle.value);

	//Add the .setter function to get the switch-event sent from iOS Home APP.
	//The .setter should be added before arduino_homekit_setup.
	//HomeKit sever uses the .setter_ex internally, see homekit_accessories_init function.
	//Maybe this is a legacy design issue in the original esp-homekit library,
	//and I have no reason to modify this "feature".
	cha_target_position.setter = cha_target_position_setter;
  cha_target_vertical_tilt_angle.setter = cha_target_vertical_tilt_angle_setter;

  
  sprintf(serial, "SN%X\0", chipId);
  
	arduino_homekit_setup(&config);

	//report the switch value to HomeKit if it is changed (e.g. by a physical button)
	//bool switch_is_on = true/false;
	//cha_switch_on.value.bool_value = switch_is_on;
	//homekit_characteristic_notify(&cha_switch_on, cha_switch_on.value);
}

void my_homekit_loop() {
  arduino_homekit_loop();
  const uint32_t t = millis();


//  if (t > next_heap_millis) {
//    // show heap info every 5 seconds
//    next_heap_millis = t + 5 * 1000;
//    LOG_D("Free heap: %d, HomeKit clients: %d",
//        ESP.getFreeHeap(), arduino_homekit_connected_clients_count());
//
//  }
  
  if(target_position == current_position){
    digitalWrite(STEPPER_PIN_5, LOW);
    digitalWrite(STEPPER_PIN_6, LOW);
    digitalWrite(STEPPER_PIN_7, LOW);
    digitalWrite(STEPPER_PIN_8, LOW);

    if(current_position != preferences.getUInt("position", 50)){
      Serial.print("current position after if: ");
      Serial.println(current_position);
      Serial.print("NVS: ");
      Serial.println(preferences.getUInt("position", 50));

      preferences.putUInt("position", current_position);

      Serial.print("current position after commit: ");
      Serial.println(current_position);
      Serial.print("NVS: ");
      Serial.println(preferences.getUInt("position", 50));
    }
  }

  if(target_vertical_tilt_angle == current_vertical_tilt_angle){
    if(moved){
      if(target_vertical_tilt_angle == 90){
        for(int i = 0; i<angleStepSize/2; i++) {
          OneStepAngle(false);
          delay(3);
        }
      }
      else if(target_vertical_tilt_angle == -90){
        for(int i = 0; i<angleStepSize/2; i++) {
          OneStepAngle(true);
          delay(3);
        }
      }
      moved = 0;
    }
    digitalWrite(STEPPER_PIN_1, LOW);
    digitalWrite(STEPPER_PIN_2, LOW);
    digitalWrite(STEPPER_PIN_3, LOW);
    digitalWrite(STEPPER_PIN_4, LOW);

    if(current_vertical_tilt_angle != preferences.getInt("angle", 90)){
      Serial.print("current angle after if: ");
      Serial.println(current_vertical_tilt_angle);
      Serial.print("NVS: ");
      Serial.println(preferences.getInt("angle", 90));

      preferences.putInt("angle", current_vertical_tilt_angle);

      Serial.print("current angle after commit: ");
      Serial.println(current_vertical_tilt_angle);
      Serial.print("NVS: ");
      Serial.println(preferences.getInt("angle", 90));
    }
  }

  // position
  if(target_position > current_position){
    if(target_position == 100){
      positionStepSize = 325;
    }
    else{
      positionStepSize = 320;
    }
    
    for(int i = 0; i<positionStepSize; i++) {
      OneStepPosition(true);
      delay(3);
    }
    digitalWrite(STEPPER_PIN_5, LOW);
    digitalWrite(STEPPER_PIN_6, LOW);
    digitalWrite(STEPPER_PIN_7, LOW);
    digitalWrite(STEPPER_PIN_8, LOW);

    current_position++;
    Serial.print("Target Position: ");
    Serial.println(target_position);
    Serial.print("Current Position: ");
    Serial.println(current_position);
    //report the lock-mechanism current-sate to HomeKit
    cha_current_position.value.int_value = current_position;
    homekit_characteristic_notify(&cha_current_position, cha_current_position.value);
  }
  if(target_position < current_position && target_position){
    if(target_position == 0){
      positionStepSize = 325;
    }
    else{
      positionStepSize = 320;
    }
    
    for(int i = 0; i<positionStepSize; i++) {
      OneStepPosition(false);
      delay(3);
    }
    digitalWrite(STEPPER_PIN_5, LOW);
    digitalWrite(STEPPER_PIN_6, LOW);
    digitalWrite(STEPPER_PIN_7, LOW);
    digitalWrite(STEPPER_PIN_8, LOW);

    current_position--;
    Serial.print("Target Position: ");
    Serial.println(target_position);
    Serial.print("Current Position: ");
    Serial.println(current_position);
    //report the lock-mechanism current-sate to HomeKit
    cha_current_position.value.int_value = current_position;
    homekit_characteristic_notify(&cha_current_position, cha_current_position.value);
  }
  
  // Vertical Angle
  if(target_vertical_tilt_angle > current_vertical_tilt_angle){
    moved = 1;
    if(target_vertical_tilt_angle == 90){
      angleStepSize = 313*.74;
    }
    else{
      angleStepSize = 313*.73;
    }
    
    for(int i = 0; i<angleStepSize; i++) {
      OneStepAngle(true);
      delay(3);
    }
    digitalWrite(STEPPER_PIN_1, LOW);
    digitalWrite(STEPPER_PIN_2, LOW);
    digitalWrite(STEPPER_PIN_3, LOW);
    digitalWrite(STEPPER_PIN_4, LOW);

    current_vertical_tilt_angle++;
    Serial.print("Target vertical_tilt_angle: ");
    Serial.println(target_vertical_tilt_angle);
    Serial.print("Current vertical_tilt_angle: ");
    Serial.println(current_vertical_tilt_angle);
    //report the lock-mechanism current-sate to HomeKit
    cha_current_vertical_tilt_angle.value.int_value = current_vertical_tilt_angle;
    homekit_characteristic_notify(&cha_current_vertical_tilt_angle, cha_current_vertical_tilt_angle.value);
  }
  if(target_vertical_tilt_angle < current_vertical_tilt_angle){
    moved = 1;
    if(target_vertical_tilt_angle == -90){
      angleStepSize = 313*.74;
    }
    else{
      angleStepSize = 313*.73;
    }
    
    for(int i = 0; i<angleStepSize; i++) {
      OneStepAngle(false);
      delay(3);
    }
    digitalWrite(STEPPER_PIN_1, LOW);
    digitalWrite(STEPPER_PIN_2, LOW);
    digitalWrite(STEPPER_PIN_3, LOW);
    digitalWrite(STEPPER_PIN_4, LOW);

    current_vertical_tilt_angle--;
    Serial.print("Target vertical_tilt_angle: ");
    Serial.println(target_vertical_tilt_angle);
    Serial.print("Current vertical_tilt_angle: ");
    Serial.println(current_vertical_tilt_angle);
    //report the lock-mechanism current-sate to HomeKit
    cha_current_vertical_tilt_angle.value.int_value = current_vertical_tilt_angle;
    homekit_characteristic_notify(&cha_current_vertical_tilt_angle, cha_current_vertical_tilt_angle.value);
  }
}
