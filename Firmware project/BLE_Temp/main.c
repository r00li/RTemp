/* Copyright (c) 2014 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

/** @file
 *
 * @brief BLE Services, a beginner's tutorial project main file.
 *
 * This example is meant to be used togheter with the tutorial "BLE Services, a beginner's tutorial" 
 * found at https://devzone.nordicsemi.com/tutorials/8/ble-services-a-beginners-tutorial/.  
 * The purpose of this tutorial is to get you started with the basics of the nRF51x22 and Bluetooth Low Energy (BLE). 
 * More specifically this tutorial will go through the bare minimum of steps to get your first services 
 * up and running. The tutorial is fairly superficial and is meant to be a hands-on introduction to BLE services. 
 * It is intended to be a natural continuation of the tutorial "BLE Advertising, a beginner's tutorial".
 * It can easily be used as a starting point for creating a new application, the comments identified
 * with 'OUR_JOB' indicates where the work described in the tutorial needs to be done.
 */

#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "nrf51_bitfields.h"
#include "ble.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "ble_dis.h"
#include "boards.h"
#include "softdevice_handler.h"
#include "app_timer.h"
#include "device_manager.h"
#include "pstorage.h"
#include "app_trace.h"
#include "our_service.h"
#include "SHT2x.h"
#include "ble_bas.h"
#include "nrf_delay.h"

#define LED_Pin 2

#define IS_SRVC_CHANGED_CHARACT_PRESENT  0                                          /**< Include or not the service_changed characteristic. if not enabled, the server's database cannot be changed for the lifetime of the device*/

#define MANUFACTURER_NAME "Roli"
#define FW_REV "1.0"
#define MODEL_NUMBER "RTemp 1.0"
#define DEVICE_NAME                      "RTemp"                               	/**< Name of device. Will be included in the advertising data. */
#define APP_ADV_INTERVAL                 1636                                        /**< The advertising interval (in units of 0.625 ms. This value corresponds to 25 ms). */
#define APP_ADV_TIMEOUT_IN_SECONDS       0                                        /**< The advertising timeout in units of seconds. */

#define APP_TIMER_PRESCALER              327                                       //Corresponds to about 100ms /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_MAX_TIMERS             (6)                  /**< Maximum number of simultaneously created timers. */
#define APP_TIMER_OP_QUEUE_SIZE          4                                          /**< Size of timer operation queues. */

#define MIN_CONN_INTERVAL                MSEC_TO_UNITS(500, UNIT_1_25_MS)           /**< Minimum acceptable connection interval (0.1 seconds). */
#define MAX_CONN_INTERVAL                MSEC_TO_UNITS(800, UNIT_1_25_MS)           /**< Maximum acceptable connection interval (0.2 second). */
#define SLAVE_LATENCY                    1                                          /**< Slave latency. */
#define CONN_SUP_TIMEOUT                 MSEC_TO_UNITS(5000, UNIT_10_MS)            /**< Connection supervisory timeout (5.9 seconds). */

#define FIRST_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(5000, APP_TIMER_PRESCALER) /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY    APP_TIMER_TICKS(30000, APP_TIMER_PRESCALER)/**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT     3                                          /**< Number of attempts before giving up the connection parameter negotiation. */

#define SEC_PARAM_BOND                   0                                          /**< Perform bonding. */
#define SEC_PARAM_MITM                   0                                          /**< Man In The Middle protection not required. */
#define SEC_PARAM_IO_CAPABILITIES        BLE_GAP_IO_CAPS_NONE                       /**< No I/O capabilities. */
#define SEC_PARAM_OOB                    0                                          /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE           7                                          /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE           16                                         /**< Maximum encryption key size. */

#define TX_POWER (-4) // accepted values are -40, -30, -20, -16, -12, -8, -4, 0, and 4 dBm

#define DEAD_BEEF                        0xDEADBEEF                                 /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */
static dm_application_instance_t         m_app_handle;                               /**< Application identifier allocated by device manager */

static uint16_t                          m_conn_handle = BLE_CONN_HANDLE_INVALID;   /**< Handle of the current connection. */

static app_timer_id_t                   measurement_timer;

static ble_bas_t                       m_bas;                                      /**< Structure used to identify the battery service. */
uint32_t battery_value_raw = 254;

uint8_t temp_log[LOG_SIZE];
uint8_t humidity_log[LOG_SIZE];
uint8_t log_counter = LOGGING_INTERVAL; // We want a log entry in the beginning

ble_os_t m_our_service;
temperature_struct temp_storage_struct;

// OUR_JOB: For advertising, declare a ble_uuid_t variable holding our service UUID 
                                   
/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num   Line number of the failing ASSERT call.
 * @param[in] file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}


/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 */
static void timers_init(void)
{
    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_MAX_TIMERS, APP_TIMER_OP_QUEUE_SIZE, false);
}


/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

		char name[20];
		sprintf(name, "%s_%04X", DEVICE_NAME, (NRF_FICR->DEVICEADDR[0] & 0xFFFF));
	
    err_code = sd_ble_gap_device_name_set(&sec_mode, (const uint8_t *)name, strlen(name));
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
																					
		err_code = sd_ble_gap_tx_power_set(TX_POWER);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing services that will be used by the application.
 */
static void services_init(void)
{
		uint32_t       err_code;
	
    // OUR_JOB: Add code to initialize the services used by the application.
		our_service_init(&m_our_service);
	
	// Initialize Battery Service.
	ble_bas_init_t bas_init;
    memset(&bas_init, 0, sizeof(bas_init));

    // Here the sec level for the Battery Service can be changed/increased.
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&bas_init.battery_level_char_attr_md.cccd_write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&bas_init.battery_level_char_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&bas_init.battery_level_char_attr_md.write_perm);

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&bas_init.battery_level_report_read_perm);

    bas_init.evt_handler          = NULL;
    bas_init.support_notification = true;
    bas_init.p_report_ref         = NULL;
    bas_init.initial_batt_level   = 100;

    err_code = ble_bas_init(&m_bas, &bas_init);
    APP_ERROR_CHECK(err_code);
		
		// Initialize Device Information Service.
		ble_dis_init_t dis_init;
    memset(&dis_init, 0, sizeof(dis_init));

    ble_srv_ascii_to_utf8(&dis_init.manufact_name_str, (char *)MANUFACTURER_NAME);
		ble_srv_ascii_to_utf8(&dis_init.model_num_str, (char *)MODEL_NUMBER);
		ble_srv_ascii_to_utf8(&dis_init.fw_rev_str, (char *)FW_REV);
		ble_srv_ascii_to_utf8(&dis_init.sw_rev_str, (char *)FW_REV);

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&dis_init.dis_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&dis_init.dis_attr_md.write_perm);

    err_code = ble_dis_init(&dis_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module which
 *          are passed to the application.
 *          @note All this function does is to disconnect. This could have been done by simply
 *                setting the disconnect_on_fail config parameter, but instead we use the event
 *                handler mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    uint32_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    uint32_t               err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for performing battery measurement and updating the Battery Level characteristic
 *        in Battery Service.
 */
static void battery_level_update(void)
{
    uint32_t err_code;
    uint8_t  battery_level;

		double voltage = 0.01411764706 * battery_value_raw; // (3.6V / 255) * measured_voltage
		if (voltage >= 2.9)
			battery_level = 100;
		else if (voltage >= 2.8)
			battery_level = 90;
		else if (voltage >= 2.6)
			battery_level = 70;
		else if (voltage >= 2.4)
			battery_level = 50;
		else if (voltage >= 2.35)
			battery_level = 15;
		else
		{
			battery_level = 0;
			nrf_gpio_pin_set(LED_Pin);
		}

    err_code = ble_bas_battery_level_update(&m_bas, battery_level);
    if ((err_code != NRF_SUCCESS) &&
        (err_code != NRF_ERROR_INVALID_STATE) &&
        (err_code != BLE_ERROR_NO_TX_BUFFERS) &&
        (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
       )
    {
        APP_ERROR_HANDLER(err_code);
    }
}

void measure_temperature_and_humidity()
{
		u8t  error = 0;
		nt16 temperature, humidity;
	
		error |= SHT2x_MeasurePoll(TEMP, &temperature);
		if (!error)
		{
			temp_storage_struct.temperature = SHT2x_CalcTemperatureC(temperature.u16);
		}
		else
		{
			nrf_gpio_pin_set(LED_Pin);
			nrf_delay_ms(200);
			nrf_gpio_pin_clear(LED_Pin);
		}
		
		error = 0;
		error |= SHT2x_MeasurePoll(HUMIDITY, &humidity);
		if (!error)
		{
			temp_storage_struct.humidity = SHT2x_CalcRH(humidity.u16);
		}
		else
		{
			nrf_gpio_pin_set(LED_Pin);
			nrf_delay_ms(200);
			nrf_gpio_pin_clear(LED_Pin);
		}
}

// ADC timer handler to start ADC sampling
static void read_battery_status()
{
	uint32_t p_is_running = 0;
		
	sd_clock_hfclk_request();
	while(! p_is_running) {  							//wait for the hfclk to be available
		sd_clock_hfclk_is_running((&p_is_running));
	}               
	NRF_ADC->TASKS_START = 1;							//Start ADC sampling
}

static void measurement_timer_handler(void * p_context)
{
    //nrf_gpio_pin_set(LED_Pin);
	
		measure_temperature_and_humidity();
		set_temperature(&m_our_service, &temp_storage_struct, &m_conn_handle);
		set_humidity(&m_our_service, &temp_storage_struct, &m_conn_handle);
	
		if (log_counter >= LOGGING_INTERVAL)
		{
			set_temperature_log(&m_our_service, (uint8_t *) &temp_log, &temp_storage_struct, &m_conn_handle);
			set_humidity_log(&m_our_service, (uint8_t *) &humidity_log, &temp_storage_struct, &m_conn_handle);
			log_counter = 0;
		}
		else
		{
			log_counter++;
		}
	
		read_battery_status();
		battery_level_update(); 
	
		//nrf_gpio_pin_clear(LED_Pin);
}


/**@brief Function for starting timers.
*/
static void application_timers_start(void)
{
	  uint32_t err_code;

    // Create timers
    err_code = app_timer_create(&measurement_timer, APP_TIMER_MODE_REPEATED, measurement_timer_handler);
    APP_ERROR_CHECK(err_code);
	
		err_code = app_timer_start(measurement_timer, APP_TIMER_TICKS(MEASUREMENT_INTERVAL, APP_TIMER_PRESCALER), NULL);
	  APP_ERROR_CHECK(err_code);
}


/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
static void sleep_mode_enter(void)
{
    uint32_t err_code;
	
    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling advertising events.
 *
 * @details This function will be called for advertising events which are passed to the application.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    //uint32_t err_code;

    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            break;
        case BLE_ADV_EVT_IDLE:
            sleep_mode_enter();
            break;
        default:
            break;
    }
}

/**@brief Function for handling the Application's BLE Stack events.
 *
 * @param[in] p_ble_evt  Bluetooth stack event.
 */
static void on_ble_evt(ble_evt_t * p_ble_evt)
{
    //uint32_t err_code;
    
    switch (p_ble_evt->header.evt_id) // Handle events
		{
        case BLE_GAP_EVT_CONNECTED:
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle; // Set connection handle 
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            m_conn_handle = BLE_CONN_HANDLE_INVALID; // Set connection handle to 0xFFFF
            break;

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for dispatching a BLE stack event to all modules with a BLE stack event handler.
 *
 * @details This function is called from the BLE Stack event interrupt handler after a BLE stack
 *          event has been received.
 *
 * @param[in] p_ble_evt  Bluetooth stack event.
 */
static void ble_evt_dispatch(ble_evt_t * p_ble_evt)
{
    dm_ble_evt_handler(p_ble_evt);
    ble_conn_params_on_ble_evt(p_ble_evt);
    on_ble_evt(p_ble_evt);
    ble_advertising_on_ble_evt(p_ble_evt);
	  ble_bas_on_ble_evt(&m_bas, p_ble_evt);
}


/**@brief Function for dispatching a system event to interested modules.
 *
 * @details This function is called from the System event interrupt handler after a system
 *          event has been received.
 *
 * @param[in] sys_evt  System stack event.
 */
static void sys_evt_dispatch(uint32_t sys_evt)
{
    pstorage_sys_event_handler(sys_evt);
    ble_advertising_on_sys_evt(sys_evt);
}


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    uint32_t err_code;

    // Initialize the SoftDevice handler module.
    SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_XTAL_20_PPM, NULL);

#if defined(S110) || defined(S130)
    // Enable BLE stack.
    ble_enable_params_t ble_enable_params;
    memset(&ble_enable_params, 0, sizeof(ble_enable_params));
#ifdef S130
    ble_enable_params.gatts_enable_params.attr_tab_size   = BLE_GATTS_ATTR_TAB_SIZE_DEFAULT;
#endif
    ble_enable_params.gatts_enable_params.service_changed = IS_SRVC_CHANGED_CHARACT_PRESENT;
    err_code = sd_ble_enable(&ble_enable_params);
    APP_ERROR_CHECK(err_code);
#endif

    // Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
    APP_ERROR_CHECK(err_code);

    // Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the Device Manager events.
 *
 * @param[in] p_evt  Data associated to the device manager event.
 */
static uint32_t device_manager_evt_handler(dm_handle_t const * p_handle,
                                           dm_event_t const  * p_event,
                                           ret_code_t        event_result)
{
    APP_ERROR_CHECK(event_result);

#ifdef BLE_DFU_APP_SUPPORT
    if (p_event->event_id == DM_EVT_LINK_SECURED)
    {
        app_context_load(p_handle);
    }
#endif // BLE_DFU_APP_SUPPORT

    return NRF_SUCCESS;
}


/**@brief Function for the Device Manager initialization.
 *
 * @param[in] erase_bonds  Indicates whether bonding information should be cleared from
 *                         persistent storage during initialization of the Device Manager.
 */
static void device_manager_init(bool erase_bonds)
{
    uint32_t               err_code;
    dm_init_param_t        init_param = {.clear_persistent_data = erase_bonds};
    dm_application_param_t register_param;

    // Initialize persistent storage module.
    err_code = pstorage_init();
    APP_ERROR_CHECK(err_code);

    err_code = dm_init(&init_param);
    APP_ERROR_CHECK(err_code);

    memset(&register_param.sec_param, 0, sizeof(ble_gap_sec_params_t));

    register_param.sec_param.bond         = SEC_PARAM_BOND;
    register_param.sec_param.mitm         = SEC_PARAM_MITM;
    register_param.sec_param.io_caps      = SEC_PARAM_IO_CAPABILITIES;
    register_param.sec_param.oob          = SEC_PARAM_OOB;
    register_param.sec_param.min_key_size = SEC_PARAM_MIN_KEY_SIZE;
    register_param.sec_param.max_key_size = SEC_PARAM_MAX_KEY_SIZE;
    register_param.evt_handler            = device_manager_evt_handler;
    register_param.service_type           = DM_PROTOCOL_CNTXT_GATT_SRVR_ID;

    err_code = dm_register(&m_app_handle, &register_param);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the Advertising functionality.
 */
static void advertising_init(void)
{
    uint32_t      err_code;
    ble_advdata_t advdata;
		int8_t        tx_power_level = TX_POWER;

    // Build advertising data struct to pass into ble_advertising_init().
    memset(&advdata, 0, sizeof(advdata));

    advdata.name_type               = BLE_ADVDATA_FULL_NAME;
		advdata.include_appearance      = true;
    advdata.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
		advdata.p_tx_power_level        = &tx_power_level;

    ble_adv_modes_config_t options = {0};
    options.ble_adv_fast_enabled  = BLE_ADV_FAST_ENABLED;
    options.ble_adv_fast_interval = APP_ADV_INTERVAL;
    options.ble_adv_fast_timeout  = APP_ADV_TIMEOUT_IN_SECONDS;

    // OUR_JOB: Create a scan response packet and include the list of UUIDs 
    ble_uuid_t m_adv_uuids[] = {{BLE_UUID_BATTERY_SERVICE, BLE_UUID_TYPE_BLE}, {BLE_UUID_OUR_SERVICE, BLE_UUID_TYPE_VENDOR_BEGIN}}; 

    ble_advdata_t srdata;
    memset(&srdata, 0, sizeof(srdata));
    srdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    srdata.uuids_complete.p_uuids = m_adv_uuids;

    err_code = ble_advertising_init(&advdata, &srdata, &options, on_adv_evt, NULL);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for the Power manager.
 */
static void power_manage(void)
{
    uint32_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}


//ADC initialization
static void adc_init(void)
{		
	/* Enable interrupt on ADC sample ready event*/		
	NRF_ADC->INTENSET = ADC_INTENSET_END_Msk;
		sd_nvic_SetPriority(ADC_IRQn, NRF_APP_PRIORITY_LOW);  

	NVIC_EnableIRQ(ADC_IRQn);	
	
	/* Configure ADC - set reference input source to internal 1.2V bandgap */
	NRF_ADC->CONFIG = (ADC_CONFIG_REFSEL_VBG << ADC_CONFIG_REFSEL_Pos);
	/* Configure ADC - set input source to VDD/3 */
	NRF_ADC->CONFIG |= (ADC_CONFIG_INPSEL_SupplyOneThirdPrescaling << ADC_CONFIG_INPSEL_Pos);
	/* Configure ADC - select 8 bit resolution */
	NRF_ADC->CONFIG |= (ADC_CONFIG_RES_8bit << ADC_CONFIG_RES_Pos);
	
	/* Enable ADC*/
	NRF_ADC->ENABLE = ADC_ENABLE_ENABLE_Enabled;	
}

/* Interrupt handler for ADC data ready event */
void ADC_IRQHandler(void)
{
	/* Clear dataready event */
  NRF_ADC->EVENTS_END = 0;	
	
	battery_value_raw = NRF_ADC->RESULT;
	
	//Use the STOP task to save current. Workaround for PAN_028 rev1.5 anomaly 1.
  NRF_ADC->TASKS_STOP = 1;
	
	//Release the external crystal
	sd_clock_hfclk_release();
}	

void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name)
{
	for (int i=0; i < 10; i ++)
	{
		nrf_gpio_pin_set(LED_Pin);
		nrf_delay_ms(300);
		nrf_gpio_pin_clear(LED_Pin);
		nrf_delay_ms(300);
	}

	NVIC_SystemReset();
}


/**@brief Function for application main entry.
 */
int main(void)
{		
		nrf_gpio_cfg_output(LED_Pin);
		nrf_gpio_pin_set(LED_Pin);
	
    uint32_t err_code;
    bool erase_bonds = true;
	
		memset(&temp_log, 0xFF, sizeof(temp_log));
		temp_log[0] = 1;
	
		memset(&humidity_log, 0xFF, sizeof(humidity_log));
		humidity_log[0] = 1;

    // Initialize.
    timers_init();
    ble_stack_init();
    device_manager_init(erase_bonds);
    gap_params_init();
    services_init();
    advertising_init();
    conn_params_init();
		adc_init();
	
		// Init temperature sensor
		I2c_Init();
		DelayMicroSeconds(15000);
		SHT2x_SoftReset();
		
		// Start execution.
    application_timers_start();
    err_code = ble_advertising_start(BLE_ADV_MODE_FAST);
    APP_ERROR_CHECK(err_code);

		nrf_gpio_pin_clear(LED_Pin);
		
		measurement_timer_handler(NULL);

    // Enter main loop.
    for (;;)
    {
        power_manage();
    }
}

/**
 * @}
 */
