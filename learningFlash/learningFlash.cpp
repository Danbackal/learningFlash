#include <stdio.h>
#include "pico/stdlib.h"
#include "string.h" // for memcpy
#include "pico/flash.h" // for XIP_BASE
#include "hardware/flash.h" // for the flash erasing and writing
#include "hardware/sync.h" // for the interrupts

#define FLASH_TARGET_OFFSET (512 * 1024) // choosing to start at 512K

#define PICO_DEFAULT_LED_PIN 14
#define LED_DELAY_MS 250

#define MAGIC_NUMBER 0xA5A5DEAA

struct WIFI_DATA {
    uint32_t magic_number; // for validation
    char ssid[40];
    char password[40];
};

WIFI_DATA wifi_data;

static void call_flash_range_erase(void *param) {
    uint32_t offset = (uint32_t)param;
    flash_range_erase(offset, FLASH_SECTOR_SIZE);
}

static void call_flash_range_program(void *param) {
    uint32_t offset = ((uintptr_t*)param)[0];
    const uint8_t *data = (const uint8_t *)((uintptr_t*)param)[1];
    flash_range_program(offset, data, FLASH_PAGE_SIZE);
}

void writeWifiData() {
    uint8_t *wifiDataBytes = (uint8_t *) &wifi_data;

    printf("Programming flash target region...\n");

    int rc = flash_safe_execute(call_flash_range_erase, (void*)FLASH_TARGET_OFFSET, UINT32_MAX);
    hard_assert(rc == PICO_OK);
    uintptr_t params[] = { FLASH_TARGET_OFFSET, (uintptr_t)wifiDataBytes};
    rc = flash_safe_execute(call_flash_range_program, params, UINT32_MAX);
    hard_assert(rc == PICO_OK);

    printf("Done writing wifi data...\n");
}

void readWifiData() {
    const uint8_t *flash_target_contents = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);
    memcpy(&wifi_data, flash_target_contents, sizeof(WIFI_DATA));
}

void pico_set_led(bool led_on) {
    gpio_put(PICO_DEFAULT_LED_PIN, led_on);
}

int main()
{
    stdio_init_all();

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    readWifiData();
    bool needToWrite = false;
    if (wifi_data.magic_number != MAGIC_NUMBER) {
        needToWrite = true;
        wifi_data.magic_number = MAGIC_NUMBER;
        strcpy(wifi_data.ssid, "MySSID");
        strcpy(wifi_data.password, "MyPassword");
        writeWifiData();
        readWifiData();
        printf("Flash data written and verified.\n");
    }


    while (true) {
        if (needToWrite) {
            printf("Writing Data to Flash:\n");
            printf("SSID: %s\n", wifi_data.ssid);
            printf("Password: %s\n", wifi_data.password);
        } else {
            printf("Reading Data from Flash:\n");
            printf("SSID: %s\n", wifi_data.ssid);
            printf("Password: %s\n", wifi_data.password);
        }
        pico_set_led(true);
        sleep_ms(LED_DELAY_MS);
        pico_set_led(false);
        sleep_ms(LED_DELAY_MS);
    }
}
