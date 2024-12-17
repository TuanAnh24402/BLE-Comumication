/***************************************************************************//**
 * @file
 * @brief Core application logic.
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
#include "em_common.h"
#include "app_assert.h"
#include "sl_bluetooth.h"
#include "app.h"
#include <stdio.h>


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

#define LED_CONTROL_UUID              0xff01
#define FAN_CONTROL_UUID              0xff02


// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

static conn_state_t state;
static sl_status_t sc;
uint8_t live_connections = 0;
connection_info_t conn[3];
uint8_t current_connection_status;


//Target service UUID
static const uint8_t service_uuid[2] = {0xFF, 0x00};

//BLE Handle
static uint32_t service_handle = 0;
static uint16_t characteristic_handle[2];

static void print_bluetooth_address(void);
static bd_addr *read_and_cache_bluetooth_address(uint8_t *address_type_out);
void string_handle(char *input_str);
void print_connections(uint8_t connection);
void bd_addr_to_string(const uint8_t *bd_addr, char *addr_str, size_t addr_str_len);
void check_status(const char *context, sl_status_t sc);

/**************************************************************************//**
 * Application Init.
 *****************************************************************************/
SL_WEAK void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////
}

/**************************************************************************//**
 * Application Process Action.
 *****************************************************************************/
SL_WEAK void app_process_action(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application code here!                              //
  // This is called infinitely.                                              //
  // Do not call blocking functions from here!                               //
  /////////////////////////////////////////////////////////////////////////////
}

/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the dummy weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  sl_status_t sc;

  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:
      char output[100];
      sprintf(output, "Bluetooth stack booted: v%d.%d.%d-b%d\n",
              evt->data.evt_system_boot.major,
              evt->data.evt_system_boot.minor,
              evt->data.evt_system_boot.patch,
              evt->data.evt_system_boot.build);
      app_log(output);
      print_bluetooth_address();
      uint16_t scan_interval = 320;
      uint16_t scan_window = 160;

      // Set scan parameters
      sc = sl_bt_scanner_set_parameters(sl_bt_scanner_scan_mode_passive,  // passive scan
                                        scan_interval,
                                        scan_window);
      check_status("Config scanner", sc);

      // Set the default connection parameters for subsequent connections
       sc = sl_bt_connection_set_default_parameters(CONN_INTERVAL_MIN,
                                                    CONN_INTERVAL_MAX,
                                                    CONN_RESPONDER_LATENCY,
                                                    CONN_TIMEOUT,
                                                    CONN_MIN_CE_LENGTH,
                                                    CONN_MAX_CE_LENGTH);
      check_status("->", sc);
      // Start scanning - looking for devices
      sc = sl_bt_scanner_start(sl_bt_scanner_scan_phy_1m,
                                sl_bt_scanner_discover_generic);
      check_status("Start discovery", sc);
      state = scanning;

      break;

    case sl_bt_evt_scanner_legacy_advertisement_report_id:
      uint8_t *adv_data = evt->data.evt_scanner_legacy_advertisement_report.data.data;
      uint8_t adv_len = evt->data.evt_scanner_legacy_advertisement_report.data.len;

      uint8_t i = 0;
      while (i < adv_len) {
        uint8_t length = adv_data[i];
        uint8_t ad_type = adv_data[i + 1];
        if (ad_type == SHORT_NAME_TYPE) {
          char name[length - 1];
          memcpy(name, &adv_data[i + 2], length - 1);
          name[length - 1] = '\0';
          if (strcmp(name, TARGET_NAME_1) == 0) {
            app_log("Found server 1, connecting..\n");
            // Initiate connection
            sl_bt_scanner_stop(); // Stop scanning
            sl_bt_connection_open(evt->data.evt_scanner_legacy_advertisement_report.address,
                                  evt->data.evt_scanner_legacy_advertisement_report.address_type,
                                  sl_bt_gap_phy_1m,
                                  &conn[0].handle);
            state = opening;
            break;
          } else if (strcmp(name, TARGET_NAME_2) == 0) {
            app_log("Found server 2, connecting..\n");
            // Initiate connection
            sl_bt_scanner_stop(); // Stop scanning
            sl_bt_connection_open(evt->data.evt_scanner_legacy_advertisement_report.address,
                                  evt->data.evt_scanner_legacy_advertisement_report.address_type,
                                  sl_bt_gap_phy_1m,
                                  &conn[1].handle);
            state = opening;
            break;
          }
        }
        i += length + 1;
      }

      break;

    // -------------------------------
    // This event indicates that a new connection was opened.

    case sl_bt_evt_connection_opened_id:
      uint8_t current_connection = evt->data.evt_connection_opened.connection;
      bd_addr connection_address = evt->data.evt_connection_opened.address;
      if (current_connection == conn[0].handle) {
          bd_addr_to_string(&connection_address, conn[0].device.address, 18);
          app_log("Connected to server 1\n");
      } else {
          bd_addr_to_string(&connection_address, conn[1].device.address, 18);
          app_log("Connected to server 2\n");
      }
      state = connecting;

      break;
    // -------------------------------
    // This event is generated when a new service is discovered
    case sl_bt_evt_gatt_service_id:
      service_handle = evt->data.evt_gatt_service.service;
      const uint8_t *uuid = evt->data.evt_gatt_service.uuid.data;
      size_t uuid_len = evt->data.evt_gatt_service.uuid.len;

      app_log("Service found with UUID: ");
      for (size_t i = 0; i < uuid_len; i++) {
        char output[20];
        sprintf(output, "%02X", uuid[i]);
        app_log(output);
        if (i < uuid_len - 1) {
          app_log(":");
        }
      }
      app_log("\n");
      app_log("Discovering characteristics...\n");

//        state = discover_services;
      break;

    // -------------------------------
    // This event is generated when a new characteristic is discovered
    case sl_bt_evt_gatt_characteristic_id:
      uint16_t handle = evt->data.evt_gatt_characteristic.characteristic;
      const uint8_t *uuid_char = evt->data.evt_gatt_characteristic.uuid.data;
      size_t uuid_char_len = evt->data.evt_gatt_characteristic.uuid.len;

      if (uuid_char_len == 2) {
        uint16_t uuid16 = uuid_char[1] << 8 | uuid_char[0];
        int index = -1;
        switch (uuid16) {
          case LED_CONTROL_UUID:
            index = LED_CONTROL;
            break;

          case FAN_CONTROL_UUID:
            index = FAN_CONTROL;
            break;
        }
        if (index != -1) {
            characteristic_handle[index] =  handle;
            char output[50];
            sprintf(output, "Saved characteristic with UUID 0x%04x at index %d\n", uuid16, index);
            app_log(output);
        }
      }
      break;

    // -------------------------------
    // This event is generated for various procedure completions, e.g. when a
    // write procedure is completed, or service discovery is completed
    case sl_bt_evt_gatt_procedure_completed_id:
      uint16_t res = evt->data.evt_gatt_procedure_completed.connection;
      uint8_t connection = evt->data.evt_gatt_procedure_completed.result;
      if (state == discover_services) {
        sc = sl_bt_gatt_discover_characteristics(conn[0].handle, service_handle);
        app_assert_status(sc);
        state = discover_characteristics;
        break;
      }
      if (state == connecting) {
        live_connections++;
        if (res == SL_STATUS_OK) {
          if (connection == conn[0].handle) {
            conn[0].connected_ok = true;
            app_log("Server 1 connected\n");
          } else if (connection == conn[1].handle) {
            conn[1].connected_ok = true;
            app_log("Server 2 connected\n");
          }
        } else {
          if (connection == conn[0].handle) {
            conn[0].connected_ok = false;
            app_log("Can not connected to server 1\n");
          } else if (connection == conn[1].handle) {
            conn[1].connected_ok = false;
            app_log("Can not connected to server 2\n");
          }
        }
        if (live_connections < 2) {
          app_log("Continue scanning\n");
          sc = sl_bt_scanner_start(sl_bt_scanner_scan_phy_1m,
                                   sl_bt_scanner_discover_generic);
          app_assert_status(sc);
          state = scanning;
        } else {
          sl_bt_scanner_stop();
          sc = sl_bt_gatt_discover_primary_services_by_uuid(conn[0].handle,
                                                            sizeof(service_uuid),
                                                            (const uint8_t*) service_uuid);
          if (sc == SL_STATUS_INVALID_HANDLE) {
            // Failed to open connection, res-tart scanning
            app_log_warning("Primary service discovery failed with invalid handle, dropping client\n");
            sc = sl_bt_scanner_start(sl_bt_gap_phy_1m, sl_bt_scanner_discover_generic);
            app_assert_status(sc);
            state = scanning;
            break;
          } else {
            app_assert_status(sc);
          }
          state = discover_services;
        }
      }
      break;
    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle,
                                                 sl_bt_advertiser_general_discoverable);
      app_assert_status(sc);

      // Restart advertising after client has disconnected.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                         sl_bt_legacy_advertiser_connectable);
      app_assert_status(sc);
      break;

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}


void check_status(const char *context, sl_status_t sc) {
  const char *default_context = "Operation";
  const char *used_context = (context && context[0] != '\0') ? context : default_context;
  char error_message[128];

  if (sc != SL_STATUS_OK) {
    snprintf(error_message, 128, "%s failed: 0x%X\n", used_context, (unsigned int)sc);
    app_log(error_message);
  } else {

  }
  memset(error_message, '\0', 128);
}

/***************************************************************************//**
 * App process string function.
 ******************************************************************************/



/**************************************************************************//**
 * @brief
 *   Function to Read and Cache Bluetooth Address.
 * @param address_type_out [out]
 *   A pointer to the outgoing address_type. This pointer can be NULL.
 * @return
 *   Pointer to the cached Bluetooth Address
 *****************************************************************************/
static bd_addr *read_and_cache_bluetooth_address(uint8_t *address_type_out)
{
  static bd_addr address;
  static uint8_t address_type;
  static bool cached = false;

  if (!cached) {
    sl_status_t sc = sl_bt_system_get_identity_address(&address, &address_type);
    app_assert_status(sc);
    cached = true;
  }

  if (address_type_out) {
    *address_type_out = address_type;
  }

  return &address;
}

static void print_bluetooth_address(void)
{
  uint8_t address_type;
  bd_addr *address = read_and_cache_bluetooth_address(&address_type);

  char output[100];
  sprintf(output, "Bluetooth %s address: %02X:%02X:%02X:%02X:%02X:%02X\n",
          address_type ? "static random" : "public device",
          address->addr[5],
          address->addr[4],
          address->addr[3],
          address->addr[2],
          address->addr[1],
          address->addr[0]);
  app_log(output);
}
// Print the status
void print_connections(uint8_t connection) {
  if (strlen(conn[connection].device.name) > 0) {
    char output[150];
    sprintf(output, "Status from connection %d:\n  Device name: %s\n  MAC Address: %s\n  BLE status : %d\n  LED1 status : %d\n  LED2 status : %d\n",
            (connection + 1),
            conn[connection].device.name,
            conn[connection].device.address,
            conn[connection].device.ble_status,
            conn[connection].data.led_status,
            conn[connection].data.fan_status);
            //conn[connection].data.temp_value);
    app_log(output);
  }
}

//Convert bd_addr to string
void bd_addr_to_string(const uint8_t *bd_addr, char *addr_str, size_t addr_str_len) {
    if (bd_addr == NULL || addr_str == NULL || addr_str_len < 18) {
        // The string's length must be 18
        return;
    }

    // Format any byte to "XX:XX:XX:XX:XX:XX"
    snprintf(addr_str, addr_str_len, "%02X:%02X:%02X:%02X:%02X:%02X",
             bd_addr[5], bd_addr[4], bd_addr[3], bd_addr[2], bd_addr[1], bd_addr[0]);
}


