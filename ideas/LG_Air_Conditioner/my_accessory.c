/*
 * my_accessory.c
 * Define the accessory in C language using the Macro in characteristics.h
 *
 *  Created on: 2020-05-15
 *      Author: Mixiaoxiao (Wang Bin)
 */

#include <homekit/homekit.h>
#include <homekit/characteristics.h>

void my_accessory_identify(homekit_value_t _value) {
  printf("accessory identify\n");
}

// Window Covering (HAP section 8.45)
// required: Target Position (9.117), Current Position (9.27 uint8 0 100)
// optional: NAME, Hold Position, Obstruction Detected

// format: int; HAP section 9.27;
homekit_characteristic_t cha_target_position = HOMEKIT_CHARACTERISTIC_(TARGET_POSITION, 50);

// format: int; HAP section 9.27;
homekit_characteristic_t cha_current_position = HOMEKIT_CHARACTERISTIC_(CURRENT_POSITION, 50);


// format: int; HAP section 9.123;
homekit_characteristic_t cha_target_vertical_tilt_angle = HOMEKIT_CHARACTERISTIC_(TARGET_VERTICAL_TILT_ANGLE, 90);

// format: int; HAP section 9.28;
homekit_characteristic_t cha_current_vertical_tilt_angle = HOMEKIT_CHARACTERISTIC_(CURRENT_VERTICAL_TILT_ANGLE, 90);


// format: string; HAP section 9.62; max length 64
//homekit_characteristic_t cha_name = HOMEKIT_CHARACTERISTIC_(NAME, "Window Covering");



// (required) format: float; HAP section 9.35; min 0, max 100, step 0.1, unit celsius
homekit_characteristic_t cha_current_temperature = HOMEKIT_CHARACTERISTIC_(CURRENT_TEMPERATURE, 25);

// (required) format: float; HAP section 9.35; min 10, max 38, step 0.1, unit celsius
homekit_characteristic_t cha_target_temperature = HOMEKIT_CHARACTERISTIC_(TARGET_TEMPERATURE, 20); // 60 to 86 F

// 0 ”off”, 1 ”heat”, 2 ”cool”, read, notify
homekit_characteristic_t cha_current_heating_cooling_state = HOMEKIT_CHARACTERISTIC_(CURRENT_HEATING_COOLING_STATE, 2);

// 0 ”off”, 1 ”heat”, 2 ”cool”, 3 ”auto”, read, write, notify
homekit_characteristic_t cha_target_heating_cooling_state = HOMEKIT_CHARACTERISTIC_(TARGET_HEATING_COOLING_STATE, 2);

// 0 ”C”, 1 ”F”, read, write, notify
homekit_characteristic_t cha_temperature_display_units = HOMEKIT_CHARACTERISTIC_(TEMPERATURE_DISPLAY_UNITS, 1);

// example for humidity
homekit_characteristic_t cha_humidity  = HOMEKIT_CHARACTERISTIC_(CURRENT_RELATIVE_HUMIDITY, 50);



homekit_characteristic_t cha_active = HOMEKIT_CHARACTERISTIC_(ACTIVE, 1);

homekit_characteristic_t cha_rotation_speed = HOMEKIT_CHARACTERISTIC_(ROTATION_SPEED, 75);

//homekit_characteristic_t cha_current_fan_state = HOMEKIT_CHARACTERISTIC_(CURRENT_FAN_STATE, 2);

homekit_accessory_t *accessories[] = {
  HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_bridge, .services=(homekit_service_t*[]) {
      // HAP section 8.17:
      // For a bridge accessory, only the primary HAP accessory object must contain this(INFORMATION) service.
      // But in my test,
      // the bridged accessories must contain an INFORMATION service,
      // otherwise the HomeKit will reject to pair.
      HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
        HOMEKIT_CHARACTERISTIC(NAME, "StringChainThermostatFAN-OTA2"),
        HOMEKIT_CHARACTERISTIC(MANUFACTURER, "Tom"),
        HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "16446759"),
        HOMEKIT_CHARACTERISTIC(MODEL, "ESP8266"),
        HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0"),
        HOMEKIT_CHARACTERISTIC(IDENTIFY, my_accessory_identify),
        NULL
      }),
      NULL
    }),
    HOMEKIT_ACCESSORY(.id=2, .category=homekit_accessory_category_window_covering, .services=(homekit_service_t*[]) {
      HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
        HOMEKIT_CHARACTERISTIC(NAME, "Window Covering"),
        HOMEKIT_CHARACTERISTIC(IDENTIFY, my_accessory_identify),
        NULL
      }),
      HOMEKIT_SERVICE(WINDOW_COVERING, .primary=true, .characteristics=(homekit_characteristic_t*[]) {
        HOMEKIT_CHARACTERISTIC(NAME, "Window Covering"),
        &cha_target_position,
        &cha_current_position,
        &cha_target_vertical_tilt_angle,
        &cha_current_vertical_tilt_angle,
        NULL
      }),
      NULL
    }),
  HOMEKIT_ACCESSORY(.id=3, .category=homekit_accessory_category_thermostat, .services=(homekit_service_t*[]) {
    HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
      HOMEKIT_CHARACTERISTIC(NAME, "LG Air Conditioner"),
      HOMEKIT_CHARACTERISTIC(IDENTIFY, my_accessory_identify),
      NULL
    }),
    HOMEKIT_SERVICE(THERMOSTAT, .primary=true, .characteristics=(homekit_characteristic_t*[]) {
      HOMEKIT_CHARACTERISTIC(NAME, "AC"),
      &cha_current_temperature,
      &cha_target_temperature,
      &cha_current_heating_cooling_state,
      &cha_target_heating_cooling_state,
      &cha_temperature_display_units,
      NULL
      }),
    HOMEKIT_SERVICE(FAN2, .characteristics=(homekit_characteristic_t*[]) {
      HOMEKIT_CHARACTERISTIC(NAME, "AC Fan"),
      &cha_active,
      &cha_rotation_speed,
      //&cha_current_fan_state,
      NULL
      }),
    HOMEKIT_SERVICE(HUMIDITY_SENSOR, .characteristics=(homekit_characteristic_t*[]) {
        HOMEKIT_CHARACTERISTIC(NAME, "Humidity Sensor"),
        &cha_humidity,
        NULL
        }),
    NULL
  }),
    NULL
};


homekit_server_config_t config = {
    .accessories = accessories,
    .password = "111-11-111"
};
