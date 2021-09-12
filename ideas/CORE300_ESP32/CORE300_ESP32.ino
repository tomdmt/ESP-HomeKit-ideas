/*
 * Air_Purifier.ino
 *
 *  Created on: 2021-08-07
 *      Author: Tom Duan
 *
 * HAP Air Purifier
 *
 */

#include <Arduino.h>
#include <arduino_homekit_server.h>
#include "wifi_info.h"

#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <TelnetStream.h>


#define LOG_D(fmt, ...)   printf_P(PSTR(fmt "\n") , ##__VA_ARGS__);

uint8_t Active = 0;

float rotation_speed = 50;
//uint8_t current_rotation_speed = 2;

uint8_t current_air_purifier_state = 0;
//uint8_t target_air_purifier_state = 0;


unsigned long previousMillis = 0;
unsigned long interval = 30000;


void setup() {
	Serial.begin(112500);
  
	wifi_connect(); // in wifi_info.h
	


  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  //ArduinoOTA.setHostname("CORE-300-ESP32");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
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

  switch (TelnetStream.read()) {
    case 'R':
    TelnetStream.stop();
    delay(100);
    ESP.restart();
      break;
    case 'C':
      TelnetStream.println("bye bye");
      TelnetStream.flush();
      TelnetStream.stop();
      break;
    case 'T':
      TelnetStream.println("Hi");
      Serial.print("Connect with Telnet client to ");
      break;
  }
  
	my_homekit_loop();
	delay(10);
}

//==============================
// HomeKit setup and loop
//==============================

// access your HomeKit characteristics defined in my_accessory.c
extern "C" homekit_server_config_t config;
extern "C" homekit_characteristic_t cha_active;
extern "C" homekit_characteristic_t cha_rotation_speed;
extern "C" homekit_characteristic_t cha_current_air_purifier_state;

static uint32_t next_heap_millis = 0;





//Called when the switch value is changed by iOS Home APP
void cha_active_setter(const homekit_value_t value) {
	Active = value.int_value;
	cha_active.value.int_value = Active;	//sync the value
	Serial.print("Active: ");
  Serial.println(Active);
  if(!Active){
    digitalWrite(27, LOW);
    digitalWrite(26, LOW);
    digitalWrite(25, LOW);
    digitalWrite(33, LOW);
    }
  if(rotation_speed <25 && Active==1){
    digitalWrite(27, LOW);
    digitalWrite(26, LOW);
    digitalWrite(25, LOW);
    digitalWrite(33, HIGH);}

  if(rotation_speed >= 25 && rotation_speed < 50 && Active==1){
    digitalWrite(27, LOW);
    digitalWrite(26, LOW);
    digitalWrite(25, HIGH);
    digitalWrite(33, LOW);}

  if(rotation_speed >= 50 && rotation_speed < 75 && Active==1){
    digitalWrite(27, LOW);
    digitalWrite(26, HIGH);
    digitalWrite(25, LOW);
    digitalWrite(33, LOW);}

  if(rotation_speed >= 75 && Active==1){
    digitalWrite(27, HIGH);
    digitalWrite(26, LOW);
    digitalWrite(25, LOW);
    digitalWrite(33, LOW);}

  current_air_purifier_state = Active * 2;
  cha_current_air_purifier_state.value.int_value = current_air_purifier_state;
  homekit_characteristic_notify(&cha_current_air_purifier_state, cha_current_air_purifier_state.value);
}


void cha_rotation_speed_setter(const homekit_value_t value) {
  rotation_speed = value.float_value;
  cha_rotation_speed.value.float_value = rotation_speed;  //sync the value
  Serial.print("rotation_speed: ");
  Serial.println(rotation_speed);

  if(rotation_speed <25 && Active==1){
    digitalWrite(27, LOW);
    digitalWrite(26, LOW);
    digitalWrite(25, LOW);
    digitalWrite(33, HIGH);}

  if(rotation_speed >= 25 && rotation_speed < 50 && Active==1){
    digitalWrite(27, LOW);
    digitalWrite(26, LOW);
    digitalWrite(25, HIGH);
    digitalWrite(33, LOW);}

  if(rotation_speed >= 50 && rotation_speed < 75 && Active==1){
    digitalWrite(27, LOW);
    digitalWrite(26, HIGH);
    digitalWrite(25, LOW);
    digitalWrite(33, LOW);}

  if(rotation_speed >= 75 && Active==1){
    digitalWrite(27, HIGH);
    digitalWrite(26, LOW);
    digitalWrite(25, LOW);
    digitalWrite(33, LOW);}
}



void my_homekit_setup() {
  pinMode(2, OUTPUT); //internal LED for keeping track
  pinMode(27, OUTPUT);
  pinMode(26, OUTPUT);
  pinMode(25, OUTPUT);
  pinMode(33, OUTPUT);

  
  cha_active.value.int_value = Active;
  cha_current_air_purifier_state.value.int_value = current_air_purifier_state;
  cha_rotation_speed.value.float_value = rotation_speed;

  //report the lock-mechanism current-sate to HomeKit
  homekit_characteristic_notify(&cha_active, cha_active.value);
  homekit_characteristic_notify(&cha_current_air_purifier_state, cha_current_air_purifier_state.value);
  homekit_characteristic_notify(&cha_rotation_speed, cha_rotation_speed.value);

	//Add the .setter function to get the switch-event sent from iOS Home APP.
	//The .setter should be added before arduino_homekit_setup.
	//HomeKit sever uses the .setter_ex internally, see homekit_accessories_init function.
	//Maybe this is a legacy design issue in the original esp-homekit library,
	//and I have no reason to modify this "feature".
	cha_active.setter = cha_active_setter;
  cha_rotation_speed.setter = cha_rotation_speed_setter;
	arduino_homekit_setup(&config);

	//report the switch value to HomeKit if it is changed (e.g. by a physical button)
	//bool switch_is_on = true/false;
	//cha_switch_on.value.bool_value = switch_is_on;
	//homekit_characteristic_notify(&cha_switch_on, cha_switch_on.value);
}



void my_homekit_loop() {
  const uint32_t t = millis();

  digitalWrite(2, Active);
  
	arduino_homekit_loop();
	if (t > next_heap_millis) {
		// show heap info every 5 seconds
		next_heap_millis = t + 5 * 1000;
		LOG_D("Free heap: %d, HomeKit clients: %d",
				ESP.getFreeHeap(), arduino_homekit_connected_clients_count());

	}
}
