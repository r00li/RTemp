/* Copyright (c) Nordic Semiconductor ASA
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 *   1. Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 
 *   2. Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 * 
 *   3. Neither the name of Nordic Semiconductor ASA nor the names of other
 *   contributors to this software may be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 * 
 *   4. This software must only be used in a processor manufactured by Nordic
 *   Semiconductor ASA, or in a processor manufactured by a third party that
 *   is used in combination with a processor manufactured by Nordic Semiconductor.
 * 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */

#ifndef OUR_SERVICE_H__
#define OUR_SERVICE_H__

#include <stdint.h>
#include "ble.h"
#include "ble_srv_common.h"


#define BLE_UUID_OUR_BASE_UUID {0xBB, 0x28, 0x17, 0x60, 0x39, 0xA6, 0x11, 0xE6, 0x87, 0x4B, 0x00, 0x02, 0xA5, 0xD5, 0xC5, 0x1B} // 128-bit base UUID
#define BLE_UUID_OUR_SERVICE 0x0000 // 16 bit UUID for our service
#define BLE_UUID_CHAR_TEMPERATURE	0x0001
#define BLE_UUID_CHAR_HUMIDITY 0x0002
#define BLE_UUID_CHAR_TEMP_LOG 0x0003
#define BLE_UUID_CHAR_HUMIDITY_LOG 0x0004

#define MEASUREMENT_INTERVAL 30000
#define LOG_SIZE 255 // Number of log entries + 1
#define LOGGING_INTERVAL 30 // Every 30 measurements a new log entry is created - every 15 minutes. This gives us 2 day long log
 
/**
 * @brief This structure contains various status information for our service. 
 * It only holds one entry now, but will be populated with more items as we go.
 * The name is based on the naming convention used in Nordic's SDKs. 
 * 'ble’ indicates that it is a Bluetooth Low Energy relevant structure and 
 * ‘os’ is short for Our Service). 
 */
typedef struct
{
	uint16_t service_handle;     /**< Handle of Our Service (as provided by the BLE stack). */
	ble_gatts_char_handles_t temperature_characteristic_handle;
	ble_gatts_char_handles_t humidity_characteristic_handle;
	ble_gatts_char_handles_t temp_log_characteristic_handle;
	ble_gatts_char_handles_t humidity_log_characteristic_handle;
} ble_os_t;

typedef struct
{
	float temperature;
	float humidity;
} temperature_struct;

/**@brief Function for initializing our new service.
 *
 * @param[in]   p_our_service       Pointer to Our Service structure.
 */
void our_service_init(ble_os_t * p_our_service);

void add_characteristic_to_service(ble_os_t * p_our_service, uint16_t characteristic_uuid, ble_gatts_char_handles_t * handle, uint8_t len_in_bytes, uint8_t notify);

void set_characteristic_value(uint8_t *p_value, ble_gatts_char_handles_t * handle, uint8_t length);

void notify_characteristic_value(ble_gatts_char_handles_t * handle, uint8_t length, uint16_t * connection_handle);

void set_temperature(ble_os_t * service, temperature_struct *temp, uint16_t * connection_handle);

void set_humidity(ble_os_t * service, temperature_struct *hum, uint16_t * connection_handle);

void set_temperature_log(ble_os_t * service, uint8_t *log, temperature_struct *temp, uint16_t * connection_handle);

void set_humidity_log(ble_os_t * service, uint8_t *log, temperature_struct *temp, uint16_t * connection_handle);

#endif  /* _ OUR_SERVICE_H__ */
