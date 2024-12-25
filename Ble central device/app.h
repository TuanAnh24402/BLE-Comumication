/***************************************************************************//**
 * @file
 * @brief Application interface provided to main().
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/

#ifndef APP_H
#define APP_H


#include <stdint.h>
#include "sl_bluetooth.h"

#define CONN_INTERVAL_MIN             80   //80ms
#define CONN_INTERVAL_MAX             80   //80ms
#define CONN_RESPONDER_LATENCY        0    //no latency
#define CONN_TIMEOUT                  100  //100ms
#define CONN_MIN_CE_LENGTH            0
#define CONN_MAX_CE_LENGTH            0xffff

#define SHORT_NAME_TYPE               0x08
#define TARGET_NAME_1                 "Server1"
#define TARGET_NAME_2                 "Server2"
#define TARGET_NAME_3                 "Server3"


#define LED_CONTROL                   0
#define FAN_CONTROL                   1

#define UUID_CHARACTERISTIC_LENGHT    2
#define LED_CONTROL_UUID              0xff01
#define FAN_CONTROL_UUID              0xff02

#define MAX_CONNECTION                3


typedef enum {
  scanning,
  connecting,
  opening,
  discover_services,
  discover_characteristics,
  running
}conn_state_t;

typedef struct {
  uint8_t led_status;
  uint8_t fan_status;
}device_data_t;

typedef struct {
  char name[10];
  char address[18];
  uint8_t ble_status;
} ble_device_t;

typedef struct {
  ble_device_t device;
  device_data_t data;
  uint8_t handle;
  uint8_t connected_ok;
} connection_info_t;

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
void app_init(void);

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
void app_process_action(void);

#endif // APP_H
