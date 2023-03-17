#ifndef MINITEL_GALLERY_H
#define MINITEL_GALLERY_H

#include <Arduino.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>    

#include "freertos/FreeRTOS.h" // needed for ESP Arduino < 2.0    
#include "freertos/queue.h"
#include "freertos/task.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"

#include "nvs.h"
#include "nvs_flash.h"

#ifdef ARDUINO_ARCH_ESP32
#include "esp32-hal-log.h"
#include "esp32-hal-bt.h"
#else
#include "esp_log.h"
#endif

#define ARDUINO_ARCH_ESP32

bool bt_startBT();
void bt_initNVS();
bool bt_initBT();

void bt_init();

#endif