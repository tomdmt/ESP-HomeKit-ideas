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



homekit_characteristic_t cha_active = HOMEKIT_CHARACTERISTIC_(ACTIVE, 0);

homekit_characteristic_t cha_rotation_speed = HOMEKIT_CHARACTERISTIC_(ROTATION_SPEED, 50);

homekit_characteristic_t cha_current_air_purifier_state = HOMEKIT_CHARACTERISTIC_(CURRENT_AIR_PURIFIER_STATE, 0);


//homekit_characteristic_t cha_name = HOMEKIT_CHARACTERISTIC_(NAME, "Levoit Core 300");

homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_air_purifier , .services=(homekit_service_t*[]) {
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "Levoit-CORE-300-ESP32"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "Tom Duan"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "0127836788"),
            HOMEKIT_CHARACTERISTIC(MODEL, "ESP32-Levoit"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, my_accessory_identify),
            NULL
        }),
    		HOMEKIT_SERVICE(AIR_PURIFIER, .primary=true, .characteristics=(homekit_characteristic_t*[]){
    		  HOMEKIT_CHARACTERISTIC(NAME, "CORE-300-ESP32"),
    		  &cha_active,
          &cha_rotation_speed,
          &cha_current_air_purifier_state,
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
