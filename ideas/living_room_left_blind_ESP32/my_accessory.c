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


char serial[16] = "XXXXXX\0";

homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_window_covering, .services=(homekit_service_t*[]) {
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "ESP32_WindowCoveringPrefOTANoDHT"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "Tom Duan"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, serial),
            HOMEKIT_CHARACTERISTIC(MODEL, "ESP32"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, my_accessory_identify),
            NULL
        }),
        HOMEKIT_SERVICE(WINDOW_COVERING, .primary=true, .characteristics=(homekit_characteristic_t*[]){
          HOMEKIT_CHARACTERISTIC(NAME, "Window Covering"),
          &cha_target_position,
          &cha_current_position,
          &cha_target_vertical_tilt_angle,
          &cha_current_vertical_tilt_angle,
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
