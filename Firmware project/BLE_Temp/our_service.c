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

#include <stdint.h>
#include <string.h>
#include "our_service.h"
#include "ble_srv_common.h"
#include "app_error.h"

/**@brief Function for initiating our new service.
 *
 * @param[in]   p_our_service        Our Service structure.
 *
 */
void our_service_init(ble_os_t * p_our_service)
{
    uint32_t   err_code; // Variable to hold return codes from library and softdevice functions
    
	  // OUR_JOB: Declare 16-bit service and 128-bit base UUIDs and add them to the BLE stack
    ble_uuid_t        service_uuid;
		ble_uuid128_t     base_uuid = BLE_UUID_OUR_BASE_UUID;
		service_uuid.uuid = BLE_UUID_OUR_SERVICE;
		err_code = sd_ble_uuid_vs_add(&base_uuid, &service_uuid.type);
		APP_ERROR_CHECK(err_code);
    
	  // OUR_JOB: Add our service
		err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &service_uuid, &p_our_service->service_handle);
		APP_ERROR_CHECK(err_code);

		add_characteristic_to_service(p_our_service, BLE_UUID_CHAR_TEMPERATURE, &p_our_service->temperature_characteristic_handle, 4, 1);
		add_characteristic_to_service(p_our_service, BLE_UUID_CHAR_HUMIDITY, &p_our_service->humidity_characteristic_handle, 1, 1);
		add_characteristic_to_service(p_our_service, BLE_UUID_CHAR_TEMP_LOG, &p_our_service->temp_log_characteristic_handle, LOG_SIZE, 0);
		add_characteristic_to_service(p_our_service, BLE_UUID_CHAR_HUMIDITY_LOG, &p_our_service->humidity_log_characteristic_handle, LOG_SIZE, 0);
}

void add_characteristic_to_service(ble_os_t * p_our_service, uint16_t characteristic_uuid, ble_gatts_char_handles_t * handle, uint8_t len_in_bytes, uint8_t notify)
{
		uint32_t err_code;

	  ble_gatts_attr_md_t cccd_md;
    memset(&cccd_md, 0, sizeof(cccd_md));
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);
    cccd_md.vloc = BLE_GATTS_VLOC_STACK;

		ble_gatts_char_md_t char_md;
    memset(&char_md, 0, sizeof(char_md));
    char_md.char_props.read = 1;
    char_md.char_props.notify = notify;
    char_md.char_props.indicate = 0;
    char_md.p_char_user_desc = NULL;
    char_md.p_char_pf = NULL;
    char_md.p_user_desc_md = NULL;
    char_md.p_cccd_md = &cccd_md;
    char_md.p_sccd_md = NULL;

    ble_uuid_t ble_uuid;
		ble_uuid.uuid = characteristic_uuid;
		ble_uuid128_t base_uuid = BLE_UUID_OUR_BASE_UUID;
    err_code = sd_ble_uuid_vs_add(&base_uuid, &ble_uuid.type);
    APP_ERROR_CHECK(err_code);

		ble_gatts_attr_md_t attr_md;
    memset(&attr_md, 0, sizeof(attr_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);

    attr_md.vloc    = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth = 0;
    attr_md.wr_auth = 0;
    attr_md.vlen    = 1;

		ble_gatts_attr_t attr_char_value;
    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid    = &ble_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len  = len_in_bytes;
    attr_char_value.init_offs = 0;
    attr_char_value.max_len   = len_in_bytes;
    attr_char_value.p_value   = NULL;

    err_code = sd_ble_gatts_characteristic_add(p_our_service->service_handle, &char_md, &attr_char_value, handle);
		APP_ERROR_CHECK(err_code);
}

void set_characteristic_value(uint8_t *p_value, ble_gatts_char_handles_t * handle, uint8_t length)
{
		uint32_t error_code;
	
		ble_gatts_value_t char_value;
		memset(&char_value, 0, sizeof(char_value));
	
		char_value.len = length;
		char_value.offset = 0;
		char_value.p_value = p_value;
		error_code = sd_ble_gatts_value_set(BLE_CONN_HANDLE_INVALID, handle->value_handle, &char_value); 
		APP_ERROR_CHECK(error_code);
}

void notify_characteristic_value(ble_gatts_char_handles_t * handle, uint8_t length, uint16_t * connection_handle)
{
    uint32_t err_code;

    // Send value if connected and notifying
    if (*connection_handle != BLE_CONN_HANDLE_INVALID)
    {
        uint16_t hvx_len = length;
			
        ble_gatts_hvx_params_t hvx_params;
        memset(&hvx_params, 0, sizeof(hvx_params));

        hvx_params.handle = handle->value_handle;
        hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
        hvx_params.offset = 0;
        hvx_params.p_len  = &hvx_len;
        hvx_params.p_data = NULL; // NULL means "Use current value".

        err_code = sd_ble_gatts_hvx(*connection_handle, &hvx_params);
        if ((err_code == NRF_SUCCESS) && (hvx_len != length))
        {
            err_code = NRF_ERROR_DATA_SIZE;
        }
    }
    else
    {
        err_code = NRF_ERROR_INVALID_STATE;
    }
}

void set_temperature(ble_os_t * service, temperature_struct *temp, uint16_t * connection_handle)
{
		int8_t temperature_no_decimal = temp->temperature;
		int8_t temperature_decimal = (temp->temperature-temperature_no_decimal)*10;
		int8_t temperature_sign = (temp->temperature >= 0)? 0 : 0xFF;
	
		uint32_t temperature_to_write = (uint32_t)temperature_sign << 24;
		temperature_to_write = temperature_to_write | (uint32_t)temperature_no_decimal << 16;
		temperature_to_write = temperature_to_write | (uint32_t)temperature_decimal << 8;
		temperature_to_write = temperature_to_write | (uint32_t)0xAA;
	
		set_characteristic_value((uint8_t *)&temperature_to_write, &service->temperature_characteristic_handle, 4);
		notify_characteristic_value(&service->temperature_characteristic_handle, 4, connection_handle);
}

void set_humidity(ble_os_t * service, temperature_struct *temp, uint16_t * connection_handle)
{
		uint8_t humidity = temp->humidity;
	
		set_characteristic_value((uint8_t *)&humidity, &service->humidity_characteristic_handle, 1);
		notify_characteristic_value(&service->humidity_characteristic_handle, 1, connection_handle);
}

void set_temperature_log(ble_os_t * service, uint8_t *log, temperature_struct *temp, uint16_t * connection_handle)
{
	uint8_t ind = log[0];
	if (ind >= LOG_SIZE)
	{
		ind = 1;
	}
	
	uint8_t log_entry = temp->temperature;
	log_entry ^= (-((temp->temperature >0)?0:1) ^ log_entry) & (1 << 7);
	log_entry ^= (-((temp->temperature-(uint8_t)temp->temperature >=0.5)?1:0) ^ log_entry) & (1 << 6);
	log[ind] = log_entry;
	log[0] = ind+1;
	
	set_characteristic_value(log, &service->temp_log_characteristic_handle, LOG_SIZE);
}

void set_humidity_log(ble_os_t * service, uint8_t *log, temperature_struct *temp, uint16_t * connection_handle)
{
	uint8_t ind = log[0];
	if (ind >= LOG_SIZE)
	{
		ind = 1;
	}
	
	uint8_t log_entry = temp->humidity;
	log[ind] = log_entry;
	log[0] = ind+1;
	
	set_characteristic_value(log, &service->humidity_log_characteristic_handle, LOG_SIZE);
}
