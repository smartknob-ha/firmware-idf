#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include <inttypes.h>
#include "esp_log.h"

#include "../sdk/mock_component/include/mock_component.hpp"
#include "../sdk/manager/include/manager.hpp"

#include "ring_lights.hpp"

void start_smartknob(void) {

	ring_lights::ring_lights ring_lights_;
	sdk::manager::instance().add_component(ring_lights_);

	sdk::manager::instance().start();

	// This is here for show, remove it if you want
	ring_lights::effect_msg msg;
	msg.primary_color = {HUE_AQUA, 255, 200};
	msg.effect = ring_lights::PERCENT;
	msg.param_a = 210;
	msg.param_b = 180;
	ring_lights_.enqueue(msg);

	while (true) {
		for (int16_t percent = 0; percent <= 100; percent += 1) {
			msg.param_c = percent;
			ring_lights_.enqueue(msg);
			vTaskDelay(pdMS_TO_TICKS(1000/CONFIG_LED_STRIP_REFRESH_RATE));
		}

		vTaskDelay(pdMS_TO_TICKS(3000));

		for (int16_t percent = 100; percent >= 0; percent -= 1) {
			msg.param_c = percent;
			ring_lights_.enqueue(msg);
			vTaskDelay(pdMS_TO_TICKS(1000/CONFIG_LED_STRIP_REFRESH_RATE));
		}

		vTaskDelay(pdMS_TO_TICKS(3000));
	}
}

extern "C" {

void app_main(void) {
	ESP_LOGI("main", "\nSleeping for 5 seconds before boot...\n");
	sleep(5);

	 /* Print chip information */
	esp_chip_info_t chip_info;
	uint32_t flash_size;
	esp_chip_info(&chip_info);
	printf("This is %s chip with %d CPU core(s), WiFi%s%s%s, ",
		   CONFIG_IDF_TARGET,
		   chip_info.cores,
		   (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
		   (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "",
		   (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

	unsigned major_rev = chip_info.revision / 100;
	unsigned minor_rev = chip_info.revision % 100;
	printf("silicon revision v%d.%d, ", major_rev, minor_rev);
	if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
		printf("Get flash size failed");
		return;
	}

	printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
		   (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

	printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());

	start_smartknob();
}

}