/**
 * Copyright (c) 2014 - 2018, Nordic Semiconductor ASA
 * 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 * 
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 * 
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 * 
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 * 
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */
/**
 * @brief BLE LED Button Service central and client application main file.
 *
 * This example can be a central for up to 8 peripherals.
 * The peripheral is called ble_app_blinky and can be found in the ble_peripheral
 * folder.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "stdarg.h"
#include "nordic_common.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"

#include "app_timer.h"
#include "bsp_btn_ble.h"
#include "ble.h"
#include "ble_hci.h"
#include "ble_advdata.h"
#include "ble_conn_params.h"
#include "ble_db_discovery.h"
#include "ble_lbs_c_extended.h"
#include "ble_thingy_uis_c.h"
#include "ble_conn_state.h"
#include "nrf_ble_gatt.h"
#include "nrf_pwr_mgmt.h"
#include "ble_agg_config_service.h"
#include "app_aggregator.h"
#include "app_uart.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

//vinh
#include "nrf_delay.h"
#include "ble_advertising.h"
#include "ble_tes_c.h"

#define APP_BLE_CONN_CFG_TAG      1                                     /**< A tag that refers to the BLE stack configuration we set with @ref sd_ble_cfg_set. Default tag is @ref APP_BLE_CONN_CFG_TAG. */
#define APP_BLE_OBSERVER_PRIO     3                                     /**< Application's BLE observer priority. You shouldn't need to modify this value. */

// Peripheral parameters

/*------------
//student: CLuster head Configuration
----------*/
#define CLUSTER_ID      5
#define DEVICE_NAME             "CH"                    /**< Name of device. Will be included in the advertising data. */
#define SINK_ID         10       

//vinh
static char const m_target_periph_name[] = "NT:";                       /**< Name of the device we try to connect to. This name is searched for in the scan report data*/

//static char const m_target_periph_name[] = "Aggregator";
//static char const m_target_blinky_name[] = "MLThingy";
static char const m_target_blinky_name[] = "Thingy";
//static char const m_target_blinky_name[] = "AA";
//static char m_target_blinky_name[5];


//#define DEVICE_NAME                  "NT:1"                       /**< Name of device. Will be included in the advertising data. */

#define AGG_CFG_SERVICE_UUID_TYPE    BLE_UUID_TYPE_VENDOR_BEGIN         /**< UUID type for the Nordic UART Service (vendor specific). */
#define MIN_PERIPHERAL_CON_INT       MSEC_TO_UNITS(100, UNIT_1_25_MS)    /**< Determines minimum connection interval in milliseconds. */
#define MAX_PERIPHERAL_CON_INT       MSEC_TO_UNITS(200, UNIT_1_25_MS)   /**< Determines maximum connection interval in milliseconds. */
#define PERIPHERAL_SLAVE_LATENCY     0                                  /**< Slave latency. */
#define PERIPHERAL_CONN_SUP_TIMEOUT  MSEC_TO_UNITS(10000, UNIT_10_MS)    /**< Connection supervisory timeout (4 seconds), Supervision Timeout uses 10 ms units. */

#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)                       /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000)                      /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3    

#define PERIPHERAL_ADV_INTERVAL                100                      /**< The advertising interval (in units of 0.625 ms. This value corresponds to 40 ms). */
#define PERIPHERAL_ADV_TIMEOUT_IN_SECONDS      0                        /**< The advertising timeout (in units of seconds). */

#define PERIPHERAL_ADV_CON_LED      BSP_BOARD_LED_0
#define CENTRAL_SCANNING_LED        BSP_BOARD_LED_1
#define LEDBUTTON_LED               BSP_BOARD_LED_2                     /**< LED to indicate a change of state of the the Button characteristic on the peer. */
#define CODED_PHY_LED               BSP_BOARD_LED_3                     /**< connected to atleast one CODED phy */

#define CENTRAL_DISCONNECT_BUTTON   BSP_BUTTON_0                        /**< Button that will write to the LED characteristic of the peer. */
#define SCAN_START_STOP_BUTTON      BSP_BUTTON_1
#define LEDBUTTON_BUTTON            BSP_BUTTON_2
#define TEST_BUTTON            BSP_BUTTON_3

#define BUTTON_DETECTION_DELAY      APP_TIMER_TICKS(50)                 /**< Delay from a GPIOTE event until a button is reported as pushed (in number of timer ticks). */

#define SCAN_INTERVAL               160//0x00F0                         /**< Determines scan interval in units of 0.625 millisecond. */
#define SCAN_WINDOW                 80// 0x0050                         /**< Determines scan window in units of 0.625 millisecond. */
#define SCAN_TIMEOUT                0x0200                              /**< Timout when scanning. 0x0000 disables timeout. */

#define MIN_CONNECTION_INTERVAL     MSEC_TO_UNITS(100, UNIT_1_25_MS)     /**< Determines minimum connection interval in milliseconds. */
#define MAX_CONNECTION_INTERVAL     MSEC_TO_UNITS(200, UNIT_1_25_MS)    /**< Determines maximum connection interval in milliseconds. */
#define SLAVE_LATENCY               0                                   /**< Determines slave latency in terms of connection events. */
#define SUPERVISION_TIMEOUT         MSEC_TO_UNITS(8000, UNIT_10_MS)     /**< Determines supervision time-out in units of 10 milliseconds. */

#define UUID16_SIZE                 2                                   /**< Size of a UUID, in bytes. */

#if (CLUSTER_ID==SINK_ID)
  #define THINGY_RSSI_CONNECT_LIMIT   -10
  #define CLUSTERHEAD_RSSI_CONNECT_LIMIT   -110
  #define APP_DEFAULT_TX_POWER        4
#else
  #define THINGY_RSSI_CONNECT_LIMIT   -50
  #define CLUSTERHEAD_RSSI_CONNECT_LIMIT   -110
  #define APP_DEFAULT_TX_POWER        -40                               /**< Supported tx_power values: -40dBm, -20dBm, -16dBm, -12dBm, -8dBm, -4dBm, 0dBm, +3dBm and +4dBm.*/
#endif
 
#define MAX_USERDATA_BUFFER_BLOCK 16
#define MAX_USERDATA_BUFFER_BLOCKSIZE 32

NRF_BLE_GATT_DEF(m_gatt);                                               /**< GATT module instance. */

BLE_AGG_CFG_SERVICE_DEF(m_agg_cfg_service);                             /**< BLE NUS service instance. */

BLE_LBS_C_ARRAY_DEF(m_lbs_c, NRF_SDH_BLE_CENTRAL_LINK_COUNT);           /**< LED Button client instances. */
BLE_THINGY_UIS_C_ARRAY_DEF(m_thingy_uis_c, NRF_SDH_BLE_CENTRAL_LINK_COUNT);

//vinh ver4, array for saving thingy enviromental service
BLE_TES_C_ARRAY_DEF(m_thingy_tes_c, NRF_SDH_BLE_CENTRAL_LINK_COUNT);

BLE_DB_DISCOVERY_ARRAY_DEF(m_db_disc, NRF_SDH_BLE_CENTRAL_LINK_COUNT);  /**< Database discovery module instances. */

APP_TIMER_DEF(m_adv_led_blink_timer_id);
APP_TIMER_DEF(m_scan_led_blink_timer_id);
APP_TIMER_DEF(m_post_message_delay_timer_id);

//vinh
APP_TIMER_DEF(m_adv_timer_id);    //timer for changing advertising packet
APP_TIMER_DEF(m_hist_refresh_timer_id);    //timer for delete an ID in history buffer 
APP_TIMER_DEF(m_add_edata_adv_buff_timer_id);    //timer for add average enviroment data to broadcast buffer 

//static volatile bool m_service_discovery_in_process = false;
static char  m_target_clusterhead_name[20]=DEVICE_NAME;


static uint16_t   m_service_discovery_conn_handle = BLE_CONN_HANDLE_INVALID;
static uint16_t   m_coded_phy_conn_handle[NRF_SDH_BLE_TOTAL_LINK_COUNT];

static uint16_t   m_per_con_handle       = BLE_CONN_HANDLE_INVALID;
static ble_uuid_t m_adv_uuids[]          =                              /**< Universally unique service identifier. */
{
    {BLE_UUID_AGG_CFG_SERVICE_SERVICE, AGG_CFG_SERVICE_UUID_TYPE}
};

typedef enum {DEVTYPE_NONE, DEVTYPE_BLINKY, DEVTYPE_THINGY} device_type_t;

//device_type_t    m_device_type_being_connected_to = DEVTYPE_NONE;
char             m_device_name_being_connected_to[30];
connected_device_info_t m_device_being_connected_info = {DEVTYPE_NONE, m_device_name_being_connected_to, 0};

static ret_code_t led_status_send_by_mask(uint8_t button_state, uint8_t r, uint8_t g, uint8_t b, uint32_t mask);
static ret_code_t led_status_on_off_send_by_mask(bool on, uint32_t mask);
static ret_code_t post_connect_message(uint8_t conn_handle);
static void       advertising_start(void);


//vinh
/*@brief: check the destination of incoming broadcast msg is to current node 
@input: data to check
byte[1] of data->p_data is the destination 
@output: true if byte[1] match CLUSTER_ID
*/
bool vf_check_destination(uint8_array_t *data);
static void vf_start_broadcast_data(void);
static void vf_start_broadcast_data2(void);
static void vf_stop_broadcast_data(void);

void relay_adv_data2(void);
void vf_relay_adv_data3(void*);
//void vf_adv_thingy_connected(ble_evt_t *p_gap_evt) ;
/*  
@brief: add a packet to buffer for advertising
@input: data 
@output: error=1  if buffer is full*/
uint8_t vf_add_packet_to_buffer(uint8_array_t *data);
uint8_t vf_add_packet_to_buffer2(uint8_array_t *data);
uint8_t vf_add_packet_to_buffer3(uint8_array_t *data);

void vf_delete_block_buffer3(bool cond);

uint16_t vf_add_buff_adv_hist3(uint32_t ids);
uint16_t vf_find_id_buff_adv_hist3(uint32_t id);
void vf_delete_buff_adv_hist3(uint16_t pos);
/*  
@brief: check the destination of original incoming broadcast msg is from current node
@input: data to check
byte[0] of data->p_data is the source 
@output: true if byte[0] match CLUSTER_ID*/
bool vf_check_source(uint8_array_t *data);
void vf_process_adv_command(uint8_array_t *data);
void vf_process_adv_command3(uint8_array_t *br_data);

void relay_adv_data(void);
void vf_relay_adv_data(void*);
//check position of packet in buffer array
//by comparing the 1st 3 bytes:
//  1st: destination id of broadcast
//  2nd: source id of broadcast
//  3rd: packet id
//return position if packet is found (all 3 bytes matched)
//or 0xFFFF if no match packet in buffer

static uint16_t vf_validate_relay_packet(uint8_array_t *checkdata);
static uint16_t vf_validate_relay_packet2(uint8_array_t *checkdata);
static uint16_t vf_validate_relay_packet3(uint8_array_t *checkdata);
/*
@modify relay data by increasing TTL, byte[2] of input
*/
static void vf_modify_relay_data(uint8_array_t *checkdata);

static void vf_tes_c_evt_handler(ble_tes_c_t * p_tes_c, ble_tes_c_evt_t * p_tes_c_evt);
//vinh
//add for userdata
#define MAX_USERDATA_BUFFER MAX_USERDATA_BUFFER_BLOCK*MAX_USERDATA_BUFFER_BLOCKSIZE
static uint8_t garr_userdata[MAX_USERDATA_BUFFER];
uint8_array_t g_userdata={
  .size=0,.p_data=garr_userdata};
uint16_t g_userdata_lastpos=0;
uint16_t g_userdata_firstpos=0;
uint16_t g_userdata_currpos=0;
uint8_t g_packetID=0;
bool g_is_sink=false;

typedef struct struct_adv_history_buff_type
{
  uint32_t id;
  uint16_t  TTL;

}adv_history_buff_t;

#define MAX_HIST_ADV_BUFF_SIZE 128
//#define MAX_HIST_ADV_BUFF_SIZE 8
adv_history_buff_t g_buff_adv_hist[MAX_HIST_ADV_BUFF_SIZE];
uint8_t g_buff_adv_hist_size=0;
uint8_t g_buff_adv_hist_firstpos=0,g_buff_adv_hist_lastpos=0;

//vinh ver4
 
thingy_edata_t g_thingy_edata[NRF_SDH_BLE_CENTRAL_LINK_COUNT];


typedef struct struct_thingy_data_type
{
  uint8_t local_id;
  uint8_t link_state;
  uint8_t button;
  uint16_t  temperature;
  //uint16_t pressure;
  uint32_t pressure;
  uint16_t humidity;
}thingy_data_t;

void vf_adv_thingy_data(thingy_data_t*);

#define ENABLE_PIN_DEBUGGING 0

#if(ITS_ENABLE_PIN_DEBUGGING == 1)
#define DEBUG_PIN_SET(_pin) nrf_gpio_pin_set(DBG_PIN_ ## _pin)
#define DEBUG_PIN_CLR(_pin) nrf_gpio_pin_clear(DBG_PIN_ ## _pin)
#else
#define DEBUG_PIN_SET(_pin) 
#define DEBUG_PIN_CLR(_pin) 
#endif

#define DBG_PIN_0 14
#define DBG_PIN_1 15
#define DBG_PIN_2 16
#define DBG_PIN_3 3
#define DBG_PIN_4 4

static void enable_gpio_debug(void)
{
#if(ENABLE_PIN_DEBUGGING == 1)
    nrf_gpio_cfg_output(DBG_PIN_0);
    nrf_gpio_cfg_output(DBG_PIN_1);
    nrf_gpio_cfg_output(DBG_PIN_2);
    
    // Configure two GPIO's to signal TX and RX activity on the radio, for debugging throughput issues on different phones
    NRF_GPIOTE->CONFIG[0] = GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos |
                            GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos | 
                            DBG_PIN_3 << GPIOTE_CONFIG_PSEL_Pos;
    NRF_GPIOTE->CONFIG[1] = GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos |
                            GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos | 
                            DBG_PIN_4 << GPIOTE_CONFIG_PSEL_Pos;
    
    NRF_PPI->CH[0].EEP = (uint32_t)&NRF_RADIO->EVENTS_TXREADY;
    NRF_PPI->CH[0].TEP = (uint32_t)&NRF_GPIOTE->TASKS_SET[0];
    
    NRF_PPI->CH[1].EEP = (uint32_t)&NRF_RADIO->EVENTS_RXREADY;
    NRF_PPI->CH[1].TEP = (uint32_t)&NRF_GPIOTE->TASKS_SET[1];
    
    NRF_PPI->CH[2].EEP = (uint32_t)&NRF_RADIO->EVENTS_DISABLED;
    NRF_PPI->CH[2].TEP = (uint32_t)&NRF_GPIOTE->TASKS_CLR[0];
    NRF_PPI->FORK[2].TEP = (uint32_t)&NRF_GPIOTE->TASKS_CLR[1];
    
    NRF_PPI->CHENSET = 0x07;
#endif
}

static uint8_t m_scan_buffer_data[BLE_GAP_SCAN_BUFFER_EXTENDED_MIN]; /**< buffer where advertising reports will be stored by the SoftDevice. */

/**@brief Pointer to the buffer where advertising reports will be stored by the SoftDevice. */
static ble_data_t m_scan_buffer =
{
    m_scan_buffer_data,
    BLE_GAP_SCAN_BUFFER_EXTENDED_MIN
};
static bool m_scanning_enabled = true;

/**@brief Scan parameters requested for scanning and connection. */
static ble_gap_scan_params_t m_scan_params =
{
    .active   = 1,
    .interval = SCAN_INTERVAL,
    .window   = SCAN_WINDOW,
    .report_incomplete_evts = 0, 
    .extended = 0,
    .timeout           = SCAN_TIMEOUT,
    .scan_phys         = BLE_GAP_PHY_1MBPS,
    .filter_policy     = BLE_GAP_SCAN_FP_ACCEPT_ALL,
    .channel_mask      = {0,0,0,0,0},
};

/**@brief Connection parameters requested for connection. */
static ble_gap_conn_params_t const m_connection_param =
{
    (uint16_t)MIN_CONNECTION_INTERVAL,
    (uint16_t)MAX_CONNECTION_INTERVAL,
    (uint16_t)SLAVE_LATENCY,
    (uint16_t)SUPERVISION_TIMEOUT
};


void uart_printf(const char *fmt, ...)
{
    char buf[256], *p;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    for (p = buf; *p; ++p)
    app_uart_put(*p);
    va_end(ap);
}


/**@brief Function to handle asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num     Line number of the failing ASSERT call.
 * @param[in] p_file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(0xDEADBEEF, line_num, p_file_name);
}


/**@brief Function for the GAP initialization.
 *
 * @details This function will set up all the necessary GAP (Generic Access Profile) parameters of
 *          the device. It also sets the permissions and appearance.
 */
static void gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    //vinh
    //err_code = sd_ble_gap_device_name_set(&sec_mode,(const uint8_t *) DEVICE_NAME,strlen(DEVICE_NAME));

    err_code = sd_ble_gap_device_name_set(&sec_mode,(const uint8_t *) m_target_clusterhead_name,strlen(m_target_clusterhead_name));

    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_PERIPHERAL_CON_INT;
    gap_conn_params.max_conn_interval = MAX_PERIPHERAL_CON_INT;
    gap_conn_params.slave_latency     = PERIPHERAL_SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = PERIPHERAL_CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);

    err_code = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_SCAN_INIT, BLE_CONN_HANDLE_INVALID, APP_DEFAULT_TX_POWER);
    APP_ERROR_CHECK(err_code);
}

enum {APPCMD_ERROR, APPCMD_SET_LED_ALL, APPCMD_SET_LED_ON_OFF_ALL, 
      APPCMD_POST_CONNECT_MESSAGE, APPCMD_DISCONNECT_PERIPHERALS,
      APPCMD_DISCONNECT_CENTRAL};


static volatile uint32_t agg_cmd_received = 0;
static uint8_t           agg_cmd[32];

static void agg_cfg_service_data_handler(ble_agg_cfg_service_evt_t * p_evt)
{
    if (p_evt->type == BLE_AGG_CFG_SERVICE_EVT_RX_DATA)
    {
        //while(agg_cmd_received != 0);
        if(agg_cmd_received == 0)
        {
            agg_cmd_received = p_evt->params.rx_data.p_data[0];
            memcpy(agg_cmd, &p_evt->params.rx_data.p_data[1], p_evt->params.rx_data.length);
        }
        else NRF_LOG_WARNING("AGG CMD OVERFLOW!!\r\n");
    }
}

/**@brief Function for the LEDs initialization.
 *
 * @details Initializes all LEDs used by the application.
 */
static void leds_init(void)
{
    bsp_board_init(BSP_INIT_LEDS);
}


/**
 * @brief Parses advertisement data, providing length and location of the field in case
 *        matching data is found.
 *
 * @param[in]  type       Type of data to be looked for in advertisement data.
 * @param[in]  p_advdata  Advertisement report length and pointer to report.
 * @param[out] p_typedata If data type requested is found in the data report, type data length and
 *                        pointer to data will be populated here.
 *
 * @retval NRF_SUCCESS if the data type is found in the report.
 * @retval NRF_ERROR_NOT_FOUND if the data type could not be found.
 */
static uint32_t adv_report_parse(uint8_t type, uint8_array_t * p_advdata, uint8_array_t * p_typedata)
{
    uint32_t  index = 0;
    uint8_t * p_data;

    p_data = p_advdata->p_data;

    while (index < p_advdata->size)
    {
        uint8_t field_length = p_data[index];
        uint8_t field_type   = p_data[index + 1];

        if (field_type == type)
        {
            p_typedata->p_data = &p_data[index + 2];
            p_typedata->size   = field_length - 1;
            return NRF_SUCCESS;
        }
        index += field_length + 1;
    }
    return NRF_ERROR_NOT_FOUND;
}

static bool m_scan_mode_coded_phy = false;

static void adv_led_blink_callback(void *p)
{
    bsp_board_led_invert(PERIPHERAL_ADV_CON_LED);
}

static void scan_led_blink_callback(void *p)
{
    bsp_board_led_invert(CENTRAL_SCANNING_LED);
}


static void scan_led_state_set(bool adv_enabled, bool coded_phy)
{
    static uint32_t current_state = 0;
    if(current_state != ((adv_enabled ? 0x01 : 0) | (coded_phy ? 0x02 : 0)))
    {
        app_timer_stop(m_scan_led_blink_timer_id);
        bsp_board_led_off(CENTRAL_SCANNING_LED);
        if(adv_enabled)
        {
            app_timer_start(m_scan_led_blink_timer_id, APP_TIMER_TICKS(coded_phy ? 400 : 100), 0);
        }
        current_state = (adv_enabled ? 0x01 : 0) | (coded_phy ? 0x02 : 0);
    }
}


/**@brief Function to start scanning. */
static void scan_start(bool coded_phy)
{
    ret_code_t ret;
#ifndef NRF52840_XXAA
    coded_phy = false;
#endif
    //(void) sd_ble_gap_scan_stop();
    if(m_scanning_enabled)
    {
        NRF_LOG_DEBUG("Scan start: Name - %s, phy - %s", (uint32_t)m_target_periph_name, coded_phy ? "Coded" : "1Mbps");
        m_scan_buffer.len = BLE_GAP_SCAN_BUFFER_EXTENDED_MIN;
        m_scan_params.scan_phys = coded_phy ? BLE_GAP_PHY_CODED : BLE_GAP_PHY_1MBPS;
        m_scan_params.extended = coded_phy ? 1 : 0;
        ret = sd_ble_gap_scan_start(&m_scan_params, &m_scan_buffer);
        if(ret == NRF_ERROR_INVALID_STATE)
        {
            NRF_LOG_INFO("scan start invalid state");
        }
        else APP_ERROR_CHECK(ret);

        scan_led_state_set(true, coded_phy);
        m_scan_mode_coded_phy = coded_phy;
    }
}

static void scan_stop(void)
{
    NRF_LOG_DEBUG("scan_stop()");
    uint32_t err_code = sd_ble_gap_scan_stop();
    if(err_code != NRF_SUCCESS)
    {
        NRF_LOG_ERROR("scan_stop() failed with error code: %x", err_code);
    }  
    scan_led_state_set(false, false);
}


/**@brief Handles events coming from the LED Button central module.
 *
 * @param[in] p_lbs_c     The instance of LBS_C that triggered the event.
 * @param[in] p_lbs_c_evt The LBS_C event.
 */
static void lbs_c_evt_handler(ble_lbs_c_t * p_lbs_c, ble_lbs_c_evt_t * p_lbs_c_evt)
{
    switch (p_lbs_c_evt->evt_type)
    {
        case BLE_LBS_C_EVT_DISCOVERY_COMPLETE:
            {
                ret_code_t err_code;

                NRF_LOG_INFO("LED Button service discovered on conn_handle 0x%x",
                             p_lbs_c_evt->conn_handle);

                err_code = app_button_enable();
                APP_ERROR_CHECK(err_code);

                // LED Button service discovered. Enable notification of Button.
                err_code = ble_lbs_c_button_notif_enable(p_lbs_c);
                APP_ERROR_CHECK(err_code);
            
                ble_gap_conn_params_t conn_params;
                conn_params.max_conn_interval = MAX_CONNECTION_INTERVAL;
                conn_params.min_conn_interval = MIN_CONNECTION_INTERVAL;
                conn_params.slave_latency     = SLAVE_LATENCY;
                conn_params.conn_sup_timeout  = SUPERVISION_TIMEOUT;

                sd_ble_gap_conn_param_update(p_lbs_c_evt->conn_handle, &conn_params);
                
                scan_start(m_scan_mode_coded_phy);
            } 
            break; // BLE_LBS_C_EVT_DISCOVERY_COMPLETE

        case BLE_LBS_C_EVT_BUTTON_NOTIFICATION:
            {
                NRF_LOG_INFO("Link 0x%x, Button state changed on peer to 0x%x",
                             p_lbs_c_evt->conn_handle,
                             p_lbs_c_evt->params.button.button_state);

                if (p_lbs_c_evt->params.button.button_state)
                {
                    bsp_board_led_on(LEDBUTTON_LED);
                }
                else
                {
                    bsp_board_led_off(LEDBUTTON_LED);
                }
            
                // Forward the data to the app aggregator module
                app_aggregator_on_blinky_data(p_lbs_c_evt->conn_handle, p_lbs_c_evt->params.button.button_state);
            } 
            break; // BLE_LBS_C_EVT_BUTTON_NOTIFICATION

        default:
            // No implementation needed.
            break;
    }
}
    
    
/**@brief Handles events coming from the Thingy UI central module.
 *
 * @param[in] p_thingy_uis_c     The instance of THINGY_UIS_C that triggered the event.
 * @param[in] p_thingy_uis_c_evt The THINGY_UIS_C event.
 */
static void thingy_uis_c_evt_handler(ble_thingy_uis_c_t * p_thingy_uis_c, ble_thingy_uis_c_evt_t * p_thingy_uis_c_evt)
{
    ret_code_t err_code;

    //vinh ver3
    thingy_data_t thingy_data;

    switch (p_thingy_uis_c_evt->evt_type)
    {
        case BLE_LBS_C_EVT_DISCOVERY_COMPLETE:
        {
            NRF_LOG_INFO("Thingy UI service discovered on conn_handle 0x%x\r\n", p_thingy_uis_c_evt->conn_handle);
            
            // Thingy UI service discovered. Enable notification of Button.
            err_code = ble_thingy_uis_c_button_notif_enable(p_thingy_uis_c);
            APP_ERROR_CHECK(err_code);
        
            ble_thingy_uis_led_set_constant(p_thingy_uis_c, 255, 255, 255);
            
            ble_gap_conn_params_t conn_params;
            conn_params.max_conn_interval = MAX_CONNECTION_INTERVAL;
            conn_params.min_conn_interval = MIN_CONNECTION_INTERVAL;
            conn_params.slave_latency     = SLAVE_LATENCY;
            conn_params.conn_sup_timeout  = SUPERVISION_TIMEOUT;

            sd_ble_gap_conn_param_update(p_thingy_uis_c_evt->conn_handle, &conn_params);
            
            scan_start(m_scan_mode_coded_phy);

        } break; // BLE_LBS_C_EVT_DISCOVERY_COMPLETE

        case BLE_LBS_C_EVT_BUTTON_NOTIFICATION:
        {
            // Forward the data to the app aggregator module
            //vinh change button -> send all sensor data

            if(g_is_sink==false)
            {
                g_thingy_edata[p_thingy_uis_c_evt->conn_handle].button=p_thingy_uis_c_evt->params.button.button_state;
                //vinh ver3
                //send button state to sink
                thingy_data.local_id=p_thingy_uis_c_evt->conn_handle;
                thingy_data.link_state=AGG_NODE_LINK_DATA_UPDATE;
                thingy_data.button=p_thingy_uis_c_evt->params.button.button_state;
                thingy_data.temperature= g_thingy_edata[p_thingy_uis_c_evt->conn_handle].temperature.avg;
                thingy_data.pressure=g_thingy_edata[p_thingy_uis_c_evt->conn_handle].pressure.avg;
                thingy_data.humidity=g_thingy_edata[p_thingy_uis_c_evt->conn_handle].humidity.avg;
                vf_adv_thingy_data(&thingy_data);  //broadcast data to sink
            }
            else
            {
              app_aggregator_on_blinky_data(p_thingy_uis_c_evt->conn_handle, p_thingy_uis_c_evt->params.button.button_state);
            }
        } break; // BLE_LBS_C_EVT_BUTTON_NOTIFICATION

        default:
            // No implementation needed.
            break;
    }
}



/**@brief Function for handling the advertising report BLE event.
 *
 * @param[in] p_adv_report  Advertising report from the SoftDevice.
 */





static void on_adv_report(ble_evt_t const * p_ble_evt)
{
    uint32_t      err_code;
    uint8_array_t adv_data;
    uint8_array_t dev_name;
    uint8_array_t service_uuid;
    //vinh
    uint8_array_t userdata;
    size_t i,trim_pos;
    //vinh
    static char str_uart[128],str2[10];
    static uint16_t str_uart_len=0;
    //NRF_LOG_INFO("ADV");

    if (m_device_being_connected_info.dev_type == DEVTYPE_NONE)
    {

        // For readibility.
        ble_gap_evt_t  const * p_gap_evt  = &p_ble_evt->evt.gap_evt;
        ble_gap_addr_t const * peer_addr  = &p_gap_evt->params.adv_report.peer_addr;

        // Prepare advertisement report for parsing.
        adv_data.p_data = (uint8_t *)p_gap_evt->params.adv_report.data.p_data;
        adv_data.size   = p_gap_evt->params.adv_report.data.len;

        // Search for advertising names.
        bool found_name = false, found_blinky_name = false,found_clusterhead_data=false;
        err_code = adv_report_parse(BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME,
                                    &adv_data,
                                    &dev_name);
        if (err_code != NRF_SUCCESS)
        {
            // Look for the short local name if it was not found as complete.
            err_code = adv_report_parse(BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME, &adv_data, &dev_name);
            if (err_code != NRF_SUCCESS)
            {
                // If we can't parse the data, then exit.
                //return;
            }
            else
            {
                found_name = true;
            }
        }
        else
        {
            found_name = true;
        }

        if (found_name)
        {
            //NRF_LOG_INFO("ADV name: %s", dev_name.p_data);
            // Check if the device name matches the BLINKY name filter
            if (strlen(m_target_periph_name) != 0)
            {
                if (memcmp(m_target_periph_name, dev_name.p_data, strlen(m_target_periph_name)) == 0)
                {
                    // Copy the name to a static variable, to pass it on to the smart phone later
                    if(dev_name.size > strlen(m_target_periph_name))
                    {
                        memcpy(m_device_name_being_connected_to, 
                               dev_name.p_data + strlen(m_target_periph_name), 
                               dev_name.size - strlen(m_target_periph_name));
                        m_device_name_being_connected_to[dev_name.size - strlen(m_target_periph_name)] = 0;
                    }
                    m_device_being_connected_info.dev_type = DEVTYPE_BLINKY;
                }
                else if(memcmp(m_target_blinky_name, dev_name.p_data, strlen(m_target_blinky_name)) == 0) //vinh, m_target_blinky_name -> THINGY
                {

                    found_blinky_name = true;
                    //vinh, not add Thingy to the list here since check RSSI at below code 
                }


                //else if(memcmp(m_target_clusterhead_name, dev_name.p_data, strlen(m_target_clusterhead_name)) == 0) //vinh, m_target_clusterhead_name -> CLUSTERHEAD
 
                /*--------------------------------
                //student: advertising receive--
                ------------------------*/
                else if(memcmp((const uint8_t *) DEVICE_NAME, dev_name.p_data, strlen((const uint8_t *) DEVICE_NAME)) == 0) //vinh, m_target_clusterhead_name -> CLUSTERHEAD
              {//found clusterhead name
                  found_clusterhead_data= true;

                  //vinh
                  uint32_t counter = app_timer_cnt_get();
                  uart_printf("find cluster head @time:%d\n\r", counter);//print to COM port

                  //parse data
                  err_code = adv_report_parse(BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA, &adv_data, &userdata);
                  if((err_code == NRF_SUCCESS)&&(p_gap_evt->params.adv_report.rssi >CLUSTERHEAD_RSSI_CONNECT_LIMIT))
                  {//have user data
                    uart_printf("find userdata, $RSSI=%d \n\r",p_gap_evt->params.adv_report.rssi);
                    str_uart[0]=0;
                    for(i=0;i<userdata.size;i++)
                    {
                     sprintf(str2,"%d ",userdata.p_data[i]);
                     strcat(str_uart,str2);
                    }
                    strcat(str_uart,"\n\r");
                    uart_printf(str_uart);

                      
                    if(vf_check_source(&userdata)==true)
                    {//message return to source, do nothing
                       strcpy(str_uart,"reflect data \n\r");
                       uart_printf(str_uart); //print to COM port
                    }
                    else
                    {//message from another source
                        if(vf_check_destination(&userdata)==true)
                        {//match destination -> process data 
                            strcpy(str_uart,"process data \n\r");
                            uart_printf(str_uart);  //print to COM port
                            vf_process_adv_command3(&userdata); 
                        }
                        else 
                        {//not destination -> message to be relayed
                        //validate message(check for redundant message in buffer)
                          if(vf_validate_relay_packet3(&userdata)==0xFFFF) 
                            err_code=vf_add_packet_to_buffer3(&userdata); //new message, add msg to buffer for advertising
                          else
                            uart_printf("Redundant packet\n\r"); //message has already been in buffer 
                        }
                    }//end message from another source
                  }//end having user data
              }//end found cluster head
            }
            //vinh ver3
            //relay_adv_data2();  //broadcast next packet in buffer
            /*-----------------
            end clusterhead advertising recevier handler
            ---------------------*/

        
            // Look for Thingy UUID
            // Filter on RSSI to avoid connecting to everything in the room
            const uint8_t thingy_service_uuid[] = {0x42, 0x00, 0x74, 0xA9, 0xFF, 0x52, 0x10, 0x9B, 0x33, 0x49, 0x35, 0x9B, 0x00, 0x01, 0x68, 0xEF};
            //vinh 
            //if(p_gap_evt->params.adv_report.rssi > THINGY_RSSI_CONNECT_LIMIT || found_blinky_name)
            if(p_gap_evt->params.adv_report.rssi > THINGY_RSSI_CONNECT_LIMIT && found_blinky_name)
            {
                err_code = adv_report_parse(BLE_GAP_AD_TYPE_128BIT_SERVICE_UUID_MORE_AVAILABLE, &adv_data, &service_uuid);
                if (err_code == NRF_SUCCESS)
                {
                    if (memcmp(service_uuid.p_data, thingy_service_uuid, 16) == 0)
                    {
                        NRF_LOG_INFO(found_blinky_name ? "Named Thingy!!" : "Thingy!!");
                        if(found_name)
                        {
                            memcpy(m_device_name_being_connected_to, 
                                   dev_name.p_data, dev_name.size);
                            m_device_name_being_connected_to[dev_name.size] = 0;
                        }
                        m_device_being_connected_info.dev_type = DEVTYPE_THINGY;
                    }
                }
            }

        


            if ((m_device_being_connected_info.dev_type != DEVTYPE_NONE)&& (found_clusterhead_data==false))
            //vinh, add cluster head to check condition   
            {
                m_device_being_connected_info.phy = m_scan_params.scan_phys;
            
                // Initiate connection.
                err_code = sd_ble_gap_connect(peer_addr, &m_scan_params, &m_connection_param, APP_BLE_CONN_CFG_TAG);
                if (err_code != NRF_SUCCESS)
                {
                    NRF_LOG_ERROR("Connection Request Failed, reason %d", err_code);
                }
            }
        }
    }
    if(m_device_being_connected_info.dev_type == DEVTYPE_NONE)
    {
        // As of SoftDevice version 6.0 scanning must be started manually after each received packet
        err_code = sd_ble_gap_scan_start(NULL, &m_scan_buffer);
        if(err_code == NRF_ERROR_INVALID_STATE)
        {
            NRF_LOG_ERROR("scan_start invalid state!!");
        }
        else APP_ERROR_CHECK(err_code);
    }
    else
    {
        scan_led_state_set(false, false);
    }
}

static uint8_t peer_addr_LR[NRF_SDH_BLE_CENTRAL_LINK_COUNT][6];

/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */

 //vinh 0, important function
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    ret_code_t err_code;
    static uint8_t coded_phy_conn_count = 0;

    //vinh ver3
    thingy_data_t thingy_data;

    // For readability.
    ble_gap_evt_t const * p_gap_evt = &p_ble_evt->evt.gap_evt;
    //NRF_LOG_INFO("Evt: %i", p_ble_evt->header.evt_id);
    switch (p_ble_evt->header.evt_id)
    {
        // Upon connection, check which peripheral has connected, initiate DB
        // discovery, update LEDs status and resume scanning if necessary.
        case BLE_GAP_EVT_CONNECTED:
        {
            // Handle central connections
            if(p_gap_evt->params.connected.role == BLE_GAP_ROLE_CENTRAL)
            {
                        NRF_LOG_INFO("Peer addr  %02x%02x%02x%02x%02x%02x",
                        peer_addr_LR[p_gap_evt->conn_handle][0],
                        peer_addr_LR[p_gap_evt->conn_handle][1],
                        peer_addr_LR[p_gap_evt->conn_handle][2],
                        peer_addr_LR[p_gap_evt->conn_handle][3],
                        peer_addr_LR[p_gap_evt->conn_handle][4],
                        peer_addr_LR[p_gap_evt->conn_handle][5]);

                NRF_LOG_INFO("Connection 0x%x established , starting DB discovery.",
                             p_gap_evt->conn_handle);

                memcpy(&peer_addr_LR[p_gap_evt->conn_handle][0], &p_gap_evt->params.connected.peer_addr.addr[0], 6);

                //APP_ERROR_CHECK_BOOL(p_gap_evt->conn_handle < NRF_SDH_BLE_CENTRAL_LINK_COUNT);

                switch(m_device_being_connected_info.dev_type)
                {
                    case DEVTYPE_BLINKY:
                        err_code = ble_lbs_c_handles_assign(&m_lbs_c[p_gap_evt->conn_handle],
                                                            p_gap_evt->conn_handle, NULL);
                        APP_ERROR_CHECK(err_code);
                        break;
                    
                    case DEVTYPE_THINGY:
                        
                        err_code = ble_thingy_uis_c_handles_assign(&m_thingy_uis_c[p_gap_evt->conn_handle],
                                                                   p_gap_evt->conn_handle, NULL);


                        NRF_LOG_INFO("Connected.");      
                        err_code = ble_tes_c_handles_assign(&m_thingy_tes_c[p_gap_evt->conn_handle], p_gap_evt->conn_handle, NULL);
                        APP_ERROR_CHECK(err_code);


                        //vinh ver2
                        if(g_is_sink==false)
                        {//advertise this new node to sink
                          thingy_data.local_id=p_gap_evt->conn_handle;
                          thingy_data.link_state=AGG_NODE_LINK_CONNECTED;
                          thingy_data.button=0;
                          thingy_data.temperature=0; 
                          thingy_data.pressure=0;
                          thingy_data.humidity=0;
                          vf_adv_thingy_data(&thingy_data);
                        //vf_adv_thingy_data(p_gap_evt,AGG_NODE_LINK_CONNECTED); //advertising to sink new thingy 
                        }

                        APP_ERROR_CHECK(err_code);
                        break;
                    
                    default:
                        break;
                }

                m_service_discovery_conn_handle = p_gap_evt->conn_handle;
                memset(&m_db_disc[p_gap_evt->conn_handle], 0x00, sizeof(ble_db_discovery_t));
                err_code = ble_db_discovery_start(&m_db_disc[p_gap_evt->conn_handle],
                                                  p_gap_evt->conn_handle);
                if (err_code != NRF_ERROR_BUSY)
                {
                    APP_ERROR_CHECK(err_code);
                }
                
                err_code = sd_ble_gap_rssi_start(p_gap_evt->conn_handle, 5, 4);
                APP_ERROR_CHECK(err_code);

                err_code = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_CONN, p_gap_evt->conn_handle, APP_DEFAULT_TX_POWER);
                APP_ERROR_CHECK(err_code);

                // Notify the aggregator service
                app_aggregator_on_central_connect(p_gap_evt, &m_device_being_connected_info);
                
                // Update LEDs status, and check if we should be looking for more
                if (ble_conn_state_central_conn_count() == NRF_SDH_BLE_CENTRAL_LINK_COUNT)
                {
                    bsp_board_led_off(CENTRAL_SCANNING_LED);
                }
                else
                {
                    // Resume scanning.
                    //bsp_board_led_on(CENTRAL_SCANNING_LED);
                    //scan_start();
                }
                
                m_device_being_connected_info.dev_type = DEVTYPE_NONE;

                // check if it was a coded phy connection
                if(BLE_GAP_PHY_CODED == m_scan_params.scan_phys)
                {
                    coded_phy_conn_count++;
                    m_coded_phy_conn_handle[p_gap_evt->conn_handle] = p_gap_evt->conn_handle;
                    bsp_board_led_on(CODED_PHY_LED);
                }
            }
            // Handle links as a peripheral here
            else
            {//connected to phone

                m_per_con_handle = p_gap_evt->conn_handle;
                NRF_LOG_INFO("Peripheral connection 0x%x established.", m_per_con_handle);    
                //vinh, start timer  to update all thingy connection to smartphone
                app_timer_start(m_post_message_delay_timer_id, APP_TIMER_TICKS(2000), 0);
                
                app_timer_stop(m_adv_led_blink_timer_id);
                
                bsp_board_led_on(PERIPHERAL_ADV_CON_LED);
            }
        } break; // BLE_GAP_EVT_CONNECTED

        // Upon disconnection, reset the connection handle of the peer which disconnected, update
        // the LEDs status and start scanning again.
        case BLE_GAP_EVT_DISCONNECTED:
        {
                NRF_LOG_INFO("Peer addr  %02x%02x%02x%02x%02x%02x",
                        peer_addr_LR[p_gap_evt->conn_handle][0],
                        peer_addr_LR[p_gap_evt->conn_handle][1],
                        peer_addr_LR[p_gap_evt->conn_handle][2],
                        peer_addr_LR[p_gap_evt->conn_handle][3],
                        peer_addr_LR[p_gap_evt->conn_handle][4],
                        peer_addr_LR[p_gap_evt->conn_handle][5]);

            NRF_LOG_INFO("GAP_EVT_DISCONNECT: %i", p_gap_evt->conn_handle);

            // Handle central disconnections
            if(p_gap_evt->conn_handle != m_per_con_handle)
            {//thingy disconnected
                NRF_LOG_INFO("LBS central link 0x%x disconnected (reason: 0x%x)",
                             p_gap_evt->conn_handle,
                             p_gap_evt->params.disconnected.reason);

                if(p_gap_evt->conn_handle == m_service_discovery_conn_handle)
                {
                    m_service_discovery_conn_handle = BLE_CONN_HANDLE_INVALID;
                }
                
                // Notify aggregator service
                app_aggregator_on_central_disconnect(p_gap_evt);

                if(m_coded_phy_conn_handle[p_gap_evt->conn_handle] != BLE_CONN_HANDLE_INVALID)
                {
                    // This is a coded phy link that got disconnected
                    m_coded_phy_conn_handle[p_gap_evt->conn_handle] = BLE_CONN_HANDLE_INVALID;
                    if(--coded_phy_conn_count == 0)
                    {
                        bsp_board_led_off(CODED_PHY_LED);
                    }
                }

                //vinh
                uart_printf("Disconnect in main \n\r");

                //vinh
                if(g_is_sink==false)
                {//advertise this new node to sink
                          thingy_data.local_id=p_gap_evt->conn_handle;
                          thingy_data.link_state=AGG_NODE_LINK_DISCONNECTED;
                          vf_adv_thingy_data(&thingy_data);

                 // vf_adv_thingy_data(p_gap_evt->conn_handle,AGG_NODE_LINK_DISCONNECTED);
                //vf_adv_thingy_data(p_gap_evt,AGG_NODE_LINK_CONNECTED); //advertising to sink new thingy 
                }
                
                // Start scanning, in case the disconnect happened during service discovery
                scan_start(m_scan_mode_coded_phy);
            }
            // Handle peripheral disconnect
            else
            {//phone disconnected

                //vinh
                //g_is_sink=false;


                NRF_LOG_INFO("Peripheral connection disconnected (reason: 0x%x)", p_gap_evt->params.disconnected.reason);
                m_per_con_handle = BLE_CONN_HANDLE_INVALID;
                
                app_aggregator_clear_buffer();
                app_timer_stop(m_post_message_delay_timer_id);
                
                bsp_board_led_off(PERIPHERAL_ADV_CON_LED);
         
                    
                // Start advertising
                advertising_start();
            }
        } break;

        case BLE_GAP_EVT_ADV_REPORT:
            on_adv_report(p_ble_evt);
            break;

        case BLE_GAP_EVT_TIMEOUT:
        {
            if (p_gap_evt->params.timeout.src == BLE_GAP_TIMEOUT_SRC_CONN)
            {
                // This can only happen with central (initiator request timeout)
                NRF_LOG_INFO("Connection request timed out.");

                scan_start(m_scan_mode_coded_phy);
            }
            else if(p_gap_evt->params.timeout.src == BLE_GAP_TIMEOUT_SRC_SCAN)
            {
                // On scan timeout, restart scanning in the opposite mode (1M vs coded)
                scan_start(!m_scan_mode_coded_phy);
            }
        } break;
        
        case BLE_GAP_EVT_ADV_SET_TERMINATED:
            NRF_LOG_INFO("Advertise timeout - Restarting...");

            // Start advertising
            advertising_start();
            break;

        case BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST:
        {
            NRF_LOG_DEBUG("BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST.");
            // Accept parameters requested by peer.
            err_code = sd_ble_gap_conn_param_update(p_gap_evt->conn_handle,
                                        &p_gap_evt->params.conn_param_update_request.conn_params);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            NRF_LOG_DEBUG("PHY update request.");
            ble_gap_phys_t const phys =
            {
                .rx_phys = BLE_GAP_PHY_AUTO,
                .tx_phys = BLE_GAP_PHY_AUTO,
            };
            err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GAP_EVT_PHY_UPDATE:
        {
            ble_gap_evt_phy_update_t phy_update = p_ble_evt->evt.gap_evt.params.phy_update;
            if(phy_update.status == BLE_HCI_STATUS_CODE_SUCCESS)
            {
                NRF_LOG_INFO("PHY updated: %i, %i", phy_update.tx_phy, phy_update.rx_phy);
                app_aggregator_phy_update(p_ble_evt->evt.gap_evt.conn_handle, phy_update.tx_phy, phy_update.rx_phy);
            }
        } break;

        case BLE_GATTC_EVT_TIMEOUT:
        {
          //vinh
            //g_is_sink=false;

            // Disconnect on GATT Client timeout event.
            NRF_LOG_DEBUG("GATT Client Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GATTS_EVT_TIMEOUT:
        {
                //vinh
                //g_is_sink=false;

            // Disconnect on GATT Server timeout event.
            NRF_LOG_DEBUG("GATT Server Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
        } break;
        
        case BLE_GAP_EVT_RSSI_CHANGED:
            app_aggregator_rssi_changed(p_ble_evt->evt.gap_evt.conn_handle, p_ble_evt->evt.gap_evt.params.rssi_changed.rssi);
            break;
            
        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for initializing services that will be used by the application.
 */
static void services_init(void)
{
    uint32_t       err_code;
    ble_agg_cfg_service_init_t agg_cfg_service_init;

    memset(&agg_cfg_service_init, 0, sizeof(agg_cfg_service_init));

    agg_cfg_service_init.data_handler = agg_cfg_service_data_handler;

    err_code = ble_agg_cfg_service_init(&m_agg_cfg_service, &agg_cfg_service_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief LED Button collector initialization. */
static void lbs_c_init(void)
{
    ret_code_t       err_code;
    ble_lbs_c_init_t lbs_c_init_obj;

    lbs_c_init_obj.evt_handler = lbs_c_evt_handler;

    for (uint32_t i = 0; i < NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++)
    {
        err_code = ble_lbs_c_init(&m_lbs_c[i], &lbs_c_init_obj);
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief LED Button collector initialization. */
static void thingy_uis_c_init(void)
{
    ret_code_t       err_code;
    ble_thingy_uis_c_init_t thingy_uis_c_init_obj;

    thingy_uis_c_init_obj.evt_handler = thingy_uis_c_evt_handler;

    for (uint32_t i = 0; i < NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++)
    {
        err_code = ble_thingy_uis_c_init(&m_thingy_uis_c[i], &thingy_uis_c_init_obj);
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief enviroment collector initialization. */
static void thingy_tes_c_init(void)
{


    ret_code_t       err_code;
    ble_tes_c_init_t thingy_tes_c_init_obj;

    thingy_tes_c_init_obj.evt_handler = vf_tes_c_evt_handler;

    for (uint32_t i = 0; i < NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++)
    {
        err_code = ble_tes_c_init(&m_thingy_tes_c[i], &thingy_tes_c_init_obj);
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupts.
 */
static void ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}

#define ADV_MAX_LENGTH  31 
static uint8_t m_adv_handle = 0;
static uint8_t m_adv_handle2 = 0;
static ble_advdata_t adv_data = {0}; 
static uint8_t adv_data_buf[ADV_MAX_LENGTH];
static uint8_t m_adv_data_buf[ADV_MAX_LENGTH];
static uint8_t m_sr_data_buf[ADV_MAX_LENGTH];
//static uint8_t adv_data_buf2[ADV_MAX_LENGTH];
static uint8_t rolling_count=0;

static ble_advdata_t sr_data = {0}; 
static uint8_t sr_data_buf[ADV_MAX_LENGTH];
static ble_gap_adv_data_t adv_packet;
static ble_gap_adv_data_t adv_packet2;
static ble_gap_adv_params_t adv_params = {0};
static ble_gap_adv_params_t adv_params2 = {0};
static int org_adv_data_size=0;

static ble_advertising_t  gp_advertising={
.adv_data.adv_data.p_data=m_adv_data_buf,
.adv_data.adv_data.len=ADV_MAX_LENGTH,
.adv_data.scan_rsp_data.p_data=m_sr_data_buf,
.adv_data.scan_rsp_data.len=ADV_MAX_LENGTH
};



void relay_adv_data(void)
{

    ret_code_t err_code;
    uint8_t i;

    uint8_t advlen=org_adv_data_size;
    static uint8_t relay_data[32];
     uint16_t pos=g_userdata_firstpos;
    uint8_t relay_size=g_userdata.p_data[pos++];
    if(g_userdata.size>0)
    {
      while(1)
      {
          pos=g_userdata_firstpos;
          relay_size=g_userdata.p_data[pos++];
          if((relay_size==0)||(g_userdata.size>MAX_USERDATA_BUFFER)\
                    ||(g_userdata_firstpos==g_userdata_lastpos))
          {//reset array
            g_userdata_firstpos=g_userdata_lastpos;
            g_userdata.p_data[g_userdata_firstpos]=0;
            g_userdata.size=0;
            break;
          }
          else
          {
            g_userdata.size-=relay_size;
            relay_data[0]=relay_size+1;
            relay_data[1]=0xff; //type: MANU
           
            if(g_userdata_firstpos+relay_size<MAX_USERDATA_BUFFER)
            {
              memcpy(&relay_data[2],&g_userdata.p_data[pos],relay_size-1);
              g_userdata_firstpos+=relay_size;
            }
            else
            {
              memcpy(&relay_data[2],&g_userdata.p_data[pos],MAX_USERDATA_BUFFER-pos);
              memcpy(&relay_data[2+MAX_USERDATA_BUFFER-pos],&g_userdata.p_data[0],relay_size-(MAX_USERDATA_BUFFER-pos)-1);
              g_userdata_firstpos=relay_size-(MAX_USERDATA_BUFFER-pos)-1;
            }

            if(relay_data[5]<8) 
            {
              //g_userdata.p_data[1]+=1;
              //adv_packet.adv_data.p_data[advlen++]=g_userdata.size+1; //length
              //adv_packet.adv_data.p_data[advlen++]=0xff; //type
              //memcpy(&adv_packet.adv_data.p_data[advlen],g_userdata.p_data,g_userdata.size);
              memcpy(&adv_packet.adv_data.p_data[advlen],relay_data,relay_size+1);
              adv_packet.adv_data.len=org_adv_data_size+relay_size+1;
    


             uart_printf("hello %d hello @%d ",adv_packet.adv_data.len,g_userdata_firstpos );
             for(int i=0;i<adv_packet.adv_data.len;i++)
              {
                uart_printf("%d  ",adv_packet.adv_data.p_data[i] );
              }
              uart_printf("\n\r");
              //nrf_delay_ms(100);

             break;

            }
          }
      }//while
    }

    
}


void vf_delete_block_buffer(void)
{//remove current block to the chain
  uint8_t pos,nextpos,prepos;
  pos=g_userdata_currpos;
            if(g_userdata.size==1)
            {//only 1 block
              g_userdata.p_data[pos*MAX_USERDATA_BUFFER_BLOCKSIZE]=0x00;
              g_userdata_firstpos=g_userdata_currpos=g_userdata_lastpos=0;
              g_userdata.size--;
            }
            else
            {
              if(pos==g_userdata_firstpos)
              {//current block is the first block
                 nextpos=g_userdata.p_data[pos*MAX_USERDATA_BUFFER_BLOCKSIZE+1];                 
                 g_userdata.p_data[nextpos*MAX_USERDATA_BUFFER_BLOCKSIZE+2]=0x00;
                 g_userdata_currpos=nextpos;
              }
              else if (pos==g_userdata_lastpos)
              {//current block is the last block
                prepos=g_userdata.p_data[pos*MAX_USERDATA_BUFFER_BLOCKSIZE+2];
                 g_userdata.p_data[prepos*MAX_USERDATA_BUFFER_BLOCKSIZE+1]=0x00;
                g_userdata_currpos=g_userdata_firstpos;
              }
              else{
                 nextpos=g_userdata.p_data[pos*MAX_USERDATA_BUFFER_BLOCKSIZE+1];
                 prepos=g_userdata.p_data[pos*MAX_USERDATA_BUFFER_BLOCKSIZE+2];
                 g_userdata.p_data[nextpos*MAX_USERDATA_BUFFER_BLOCKSIZE+2]=prepos;
                 g_userdata.p_data[prepos*MAX_USERDATA_BUFFER_BLOCKSIZE+1]=nextpos;
                 g_userdata_currpos=nextpos;
              }
              g_userdata.size--;
            }
}


/*---------------------
@Brief: remove a block from advertising buffer chain
input: @ref cond:
                    true: remove block ids (0x00AABBCC, AA:<source id>, BB<dest id>, CC<packet id)) will be added to hist buff
                    false: not added
       @ref g_userdata_currpos: block to be deleted in advertising buffer (g_userdata)
ouput:
update:
  g_userdata_currpos: point to next position in buffer
  g_userdata.size: number of used block in buffer
*/

void vf_delete_block_buffer3(bool cond)
{//remove current block to the chain
  uint16_t pos,nextpos,prepos,pos1;
  size_t i;
  uint32_t ids;

  //save current id
  pos1=g_userdata_currpos;
  pos=g_userdata_currpos*MAX_USERDATA_BUFFER_BLOCKSIZE;
  ids=((uint32_t)g_userdata.p_data[pos+5]<<16)+((uint32_t)g_userdata.p_data[pos+6]<<8)+(uint32_t)g_userdata.p_data[pos+7];

  if(g_userdata.size==0)
  {//no available block in buffer
    return;
  }
  else if(g_userdata.size==1)
  {//only 1 block, then reset all pointer to initial state
    g_userdata.p_data[pos]=0x00;
    g_userdata.p_data[pos+1]=g_userdata.p_data[pos+2]=0xFF;//NULL
    g_userdata_firstpos=g_userdata_currpos=g_userdata_lastpos=0;
    g_userdata.size=0;
   
  }
  else
  {
    if(pos1==g_userdata_firstpos)
    {//current block is the first block
       nextpos=g_userdata.p_data[pos+1];
       g_userdata.p_data[pos]=0x00;//mark deleted block as free block
       g_userdata.p_data[pos+1]=g_userdata.p_data[pos+2]=0xFF;//NULL

       g_userdata.p_data[nextpos*MAX_USERDATA_BUFFER_BLOCKSIZE+2]= 0xFF; //NULL
       g_userdata_firstpos=nextpos;

       //vinh ver4
       //if(g_userdata_currpos==pos1)
       //{
          g_userdata_currpos=g_userdata_firstpos;
       //}

    }
    else if (pos1==g_userdata_lastpos)
    {//current block is the last block
      prepos=g_userdata.p_data[pos+2];
       g_userdata.p_data[pos]=0x00; //mark as free block
       g_userdata.p_data[pos+1]=g_userdata.p_data[pos+2]=0xFF;//NULL

       g_userdata.p_data[prepos*MAX_USERDATA_BUFFER_BLOCKSIZE+1]= 0xFF; //NULL
       g_userdata_lastpos=prepos;
      
       //vinh ver4
       //if(g_userdata_currpos==pos1)
       //{
          g_userdata_currpos=g_userdata_firstpos;
       //}
    }
    else{
       nextpos=g_userdata.p_data[pos+1];
       prepos=g_userdata.p_data[pos+2];
       g_userdata.p_data[pos]=0x00; //mark as free block
       g_userdata.p_data[pos+1]=g_userdata.p_data[pos+2]=0xFF;//NULL

       g_userdata.p_data[nextpos*MAX_USERDATA_BUFFER_BLOCKSIZE+2]=prepos;
       g_userdata.p_data[prepos*MAX_USERDATA_BUFFER_BLOCKSIZE+1]=nextpos;
      
       //vinh ver4if(g_userdata_currpos==pos1)
       //{
          g_userdata_currpos=nextpos;
       //}

    }
    g_userdata.size--;



  }
    uart_printf("delete block %d, new curr pos:%d, new size: %d \n\r",pos1,g_userdata_currpos,g_userdata.size);
 
  if(cond==true)
  {// add ids to history buffer
      vf_add_buff_adv_hist3(ids);
  }
}





/*---------------------
@Brief: advertising a packet in buffer by pasting it to user data field of adv struct
g_userdata_currpos: packet to be sent
g_userdata_firstpos: first packet block in buffer
g_userdata_lastpos: last packet block in buffer
g_userdata.size: number of used block in buffer
*/

void relay_adv_data2(void)
{

    ret_code_t err_code;
    uint8_t i,j=0;

    uint8_t advlen=org_adv_data_size;
    static uint8_t relay_data[32];
     uint16_t pos=g_userdata_firstpos;
    uint8_t relay_size;

    while(g_userdata.size>0) //check nubmer of used block
    {
          pos=g_userdata_currpos;
          if(g_userdata.p_data[pos*MAX_USERDATA_BUFFER_BLOCKSIZE+4]==0)
          {//size of data =0 ->delete block
            vf_delete_block_buffer();
          }
          else if(g_userdata.p_data[pos*MAX_USERDATA_BUFFER_BLOCKSIZE+4+4]>=4) //max TTL=4
          {//TTL expired ->delete block
            vf_delete_block_buffer();
          }
          else
          {
            if(g_userdata_currpos==g_userdata_lastpos) //reach lastpos -> first pos 
                g_userdata_currpos=g_userdata_firstpos;
            else
              g_userdata_currpos=g_userdata.p_data[pos*MAX_USERDATA_BUFFER_BLOCKSIZE+1];

            relay_size= g_userdata.p_data[pos*MAX_USERDATA_BUFFER_BLOCKSIZE+4];
            g_userdata.p_data[pos*MAX_USERDATA_BUFFER_BLOCKSIZE+4+4]++; //increase TTL

            relay_data[0]=relay_size;
            relay_data[1]=0xff; //type: MANUFACTURER  
            memcpy(&relay_data[2],&g_userdata.p_data[pos*MAX_USERDATA_BUFFER_BLOCKSIZE+5],relay_size);
            memcpy(&adv_packet.adv_data.p_data[advlen],relay_data,relay_size+1);
            adv_packet.adv_data.len=org_adv_data_size+relay_size+1;

             uart_printf("adv relay data *%d (len)%d @%d",j,adv_packet.adv_data.len,pos );
              j++;
             for(int i=0;i<adv_packet.adv_data.len;i++)
              {
                uart_printf("%d  ",adv_packet.adv_data.p_data[i] );
              }
              uart_printf("\n\r"); 
            break;
          }

    }//while
   
}


/*---------------------
@Brief: advertising a packet in buffer by pasting it to user data field of adv struct
g_userdata_currpos: packet to be sent
g_userdata_firstpos: first packet block in buffer
g_userdata_lastpos: last packet block in buffer
g_userdata.size: number of used block in buffer
*/

void vf_relay_adv_data3(void* p_indata)
{

    ret_code_t err_code;
    uint8_t i,j=0;

    uint8_t advlen=org_adv_data_size;
    static uint8_t relay_data[32];
    uint8_t relay_size;
    size_t pos;
//TODO: size=0 -> stop broadcast

 
    while(g_userdata.size>0) //check nubmer of used block
    {
          if(g_userdata.p_data[g_userdata_currpos*MAX_USERDATA_BUFFER_BLOCKSIZE+4]==0)
          {//size of broadcast data =0 (invalid block) ->delete block, not add to history buffer
            vf_delete_block_buffer3(false);
          }
          else
          {
          //vinh ver4
            pos=g_userdata_currpos;   //get position of data block to be transfered 
            relay_size= g_userdata.p_data[pos*MAX_USERDATA_BUFFER_BLOCKSIZE+4];
            relay_data[0]=relay_size;
            relay_data[1]=0xff; //type: MANUFACTURER  
            memcpy(&relay_data[2],&g_userdata.p_data[pos*MAX_USERDATA_BUFFER_BLOCKSIZE+5],relay_size-1);
            relay_data[5]++; //increase hop counts

            memcpy(&adv_packet.adv_data.p_data[advlen],relay_data,relay_size+1); //copy data to broadcast
            adv_packet.adv_data.len=org_adv_data_size+relay_size+1;

            uart_printf("adv relay data (len)%d @%d, data: ",adv_packet.adv_data.len,pos );
            j++;
            for(int i=0;i<adv_packet.adv_data.len;i++)
            {
                uart_printf("%d  ",adv_packet.adv_data.p_data[i] );
            }
            uart_printf("\n\r");

            if(--g_userdata.p_data[pos*MAX_USERDATA_BUFFER_BLOCKSIZE+3]==0)
            { // advertised more than 2 times, then move this block to history buffer
                vf_delete_block_buffer3(true);
                uart_printf("buffer state: new currpos=%d, data:",g_userdata_currpos);
                for(i=0;i<MAX_USERDATA_BUFFER_BLOCK;i++)
                {
                  uart_printf("%d ",g_userdata.p_data[i*MAX_USERDATA_BUFFER_BLOCKSIZE]);
                }
                uart_printf("\n\r");
            }
            else
            {
              //vinh ver4
              if(g_userdata_currpos==g_userdata_lastpos) //reach lastpos -> first pos 
                g_userdata_currpos=g_userdata_firstpos;
              else
                g_userdata_currpos=g_userdata.p_data[g_userdata_currpos*MAX_USERDATA_BUFFER_BLOCKSIZE+1];
              uart_printf("buffer state: new currpos=%d \n\r",g_userdata_currpos);
            }

            break;
          }

        if(g_userdata.size==0) vf_stop_broadcast_data();

    }//while
   
}

/*
@brief: modify relay data by increasing TTL, byte[3] of input
*/
static void vf_modify_relay_data(uint8_array_t *checkdata)
{
  checkdata->p_data[3]++;
}

/*
@brief: validate relay data, looking up in the buffer to find an element match with new data:
1. source addr (byte 0), destination addr(byte 1), packet id(byte2) 
2. if (1.) match, check time to live (byte 3) and update the new to old if the new is higher
 
3.  if (1.) not match, return 0xFFFF -> new packet
*/
static uint16_t vf_validate_relay_packet(uint8_array_t *checkdata)
{

  uint8_t src_id=0;
  uint8_array_t userdata;
  uint8_t cmpdata[4];
  uint8_t i,size;
  size_t temp_pos=g_userdata_firstpos;
  size_t pos=g_userdata_firstpos;
  while(pos!=g_userdata_lastpos)
  {
      temp_pos=pos+1;
      for(i=0;i<4;i++)
      {
        if(temp_pos>=MAX_USERDATA_BUFFER)
        {
          temp_pos=0;
        }
        cmpdata[i]=garr_userdata[temp_pos++];
      }
      if(memcmp(checkdata->p_data,cmpdata,3)==0)
      {//matched
        if(checkdata->p_data[3]>cmpdata[3])
        {//update TTL if new is higher than old
          garr_userdata[temp_pos-1]=checkdata->p_data[3];
        }

        return pos;
      }
      else
      {//unmatched, check next packet in buffer

        size=g_userdata.p_data[pos];
        if(size+pos<=MAX_USERDATA_BUFFER)
        {
          pos+=size;
        }
        else
        {
          pos=0+size-(MAX_USERDATA_BUFFER-pos);
        }
      }
  }
  return 0xFFFF;
}


/*
@brief: validate relay data, looking up in the buffer to find an element match with new data:
1. source id (byte 0), destination id(byte 1), packet id(byte2) 
2. if (1.) match, check time to live (byte 3) and update the new to old if the new is higher
 return the position of match item
3.  if (1.) not match, return 0xFFFF -> new packet
*/
static uint16_t vf_validate_relay_packet2(uint8_array_t *checkdata)
{

  uint8_t src_id=0;
  uint8_array_t userdata;
  uint8_t cmpdata[4];
  uint8_t i,size;
  size_t temp_pos=g_userdata_firstpos;
  size_t pos=g_userdata_firstpos;

  if(g_userdata.size==0)
    return 0xFFFF;
  else if(g_userdata.size==1)
  {
      temp_pos=pos*MAX_USERDATA_BUFFER_BLOCKSIZE+5;    
      for(i=0;i<4;i++)
      {
        cmpdata[i]=g_userdata.p_data[temp_pos++];
      }
      if(memcmp(checkdata->p_data,cmpdata,3)==0)
      {
     /*check TTL
        if(checkdata->p_data[3]>cmpdata[3])
        {//update TTL if new is higher than old
          g_userdata.p_data[temp_pos-1]=checkdata->p_data[3];
        }*/

        return pos;
      }
      else
      {
        return 0xFFFF;
      }

  }
  else
  {
    while(pos!=g_userdata_lastpos)
   {
      temp_pos=pos*MAX_USERDATA_BUFFER_BLOCKSIZE+5;    
      for(i=0;i<4;i++)
      {
        cmpdata[i]=g_userdata.p_data[temp_pos++];
      }
      if(memcmp(checkdata->p_data,cmpdata,3)==0)
      {
      /*check TTL
        if(checkdata->p_data[3]>cmpdata[3])
        {//update TTL if new is higher than old
          g_userdata.p_data[temp_pos-1]=checkdata->p_data[3];
        }*/

        return pos;
      }
      else
      {//unmatched, check next packet in buffer
        pos=g_userdata.p_data[pos*MAX_USERDATA_BUFFER_BLOCKSIZE+1];
      }
    }
  }
  return 0xFFFF;
}



/*--------------------
@brief: modify input data to preparing send back to the source
*/
void vf_process_adv_command(uint8_array_t *data)
{//send to phone
  //if(g_is_sink==true) vinh doing


//vinh doing
    vf_app_adv_data_send_to_phone(data);
}


/*-----------------------------------------------
@brief: validate incomming data, check whether this message was recevice before.
@input: *checkdata:  source addr (byte 0), destination addr(byte 1), packet id(byte2)
   -> merge into 32 bits var ids= 0x00AABBCC, with AA is source id, BB is destination id, CC is packet id
@operation:
  compare ids with:
    - history buffer which contains ids of old messages
        which was either removed from advertising buffer(g_userdata) or processed.
    - if no matched item in history buffer, compare with current advertising buffer(g_userdata)

@return
   if matched found, return the position of the packet
      in history buffer (=0x8000+pos) or advertising buffer (pos)
    else return 0xFFFF -> new packet
*/
static uint16_t vf_validate_relay_packet3(uint8_array_t *checkdata)
{
  uint8_t *cmpdata_pos;
  uint16_t i;
  uint32_t ids=0,cmp_data;
  size_t temp_pos;
  size_t pos=g_userdata_firstpos;

  ids=((uint32_t)checkdata->p_data[0]<<16)+((uint32_t)checkdata->p_data[1]<<8)+(uint32_t)(checkdata->p_data[2]);

  //check history buffer
  if (vf_find_id_buff_adv_hist3(ids)!=0xFFFF)
  {
    uart_printf("find in history buffer: 0x%x \n\r", i);
    return i+8000; //already in history buffer

  }
  //check current buffer
  if(g_userdata.size==0)
    return 0xFFFF;
  else
  {
    uart_printf("ids in operating buffer: ");
    do
    {
      uart_printf("0x%x ,",i);
      temp_pos=pos*MAX_USERDATA_BUFFER_BLOCKSIZE+5; 
      cmpdata_pos=&g_userdata.p_data[temp_pos];
      cmp_data=((*cmpdata_pos)<<16)+((*(cmpdata_pos+1))<<8)+(*(cmpdata_pos+2));
      if(cmp_data==ids)
      {
        uart_printf("find in operating buffer: 0x%x \n\r", i);
        return pos;
      }
      else
      {
        return 0xFFFF;
      }
      pos=g_userdata.p_data[pos*MAX_USERDATA_BUFFER_BLOCKSIZE+1];
    }
    while (pos!=g_userdata_lastpos);
  }

  return 0xFFFF;
}



/*----------
@brief: add an id value to buffer of adverting history
@input: id value (0x00AABBCC, AA: source id, BB: dest id, CC:packet id)
@ouput: position in the array g_buff_adv_hist,
    0xFFFF if this id has already been in the buffer
----------------------*/
uint16_t vf_add_buff_adv_hist3(uint32_t ids)
{
  uint16_t i;

    //check current id has already been in history buffer?
    if((i=vf_find_id_buff_adv_hist3(ids))!=0xFFFF) return 0xFFFF; //yes -> quit

    g_buff_adv_hist[g_buff_adv_hist_lastpos].id=ids;
    g_buff_adv_hist[g_buff_adv_hist_lastpos].TTL=10; //timeout 10 sec
    if(g_buff_adv_hist_size!=MAX_HIST_ADV_BUFF_SIZE)
    {
      if(++g_buff_adv_hist_lastpos==MAX_HIST_ADV_BUFF_SIZE)
          g_buff_adv_hist_lastpos=0;
    }
    else
    {
      if(++g_buff_adv_hist_lastpos==MAX_HIST_ADV_BUFF_SIZE)
          g_buff_adv_hist_lastpos=0;
      g_buff_adv_hist_firstpos=g_buff_adv_hist_lastpos;
    }

    if(g_buff_adv_hist_size<MAX_HIST_ADV_BUFF_SIZE) g_buff_adv_hist_size++;
      

    uart_printf("add to history 0x%x, lastpos:%d, fisrtpos:%d, size:%d \n\r", \
                ids,g_buff_adv_hist_lastpos,g_buff_adv_hist_firstpos,g_buff_adv_hist_size);
  return g_buff_adv_hist_size;
}

/*----------
@brief: delete an element in buffer of advertising history
@input: position
*/
void vf_delete_buff_adv_hist3(uint16_t pos)
{
  uint16_t i,size;

    if(g_buff_adv_hist_size==0)   return;
    g_buff_adv_hist_size--;
    if(++g_buff_adv_hist_firstpos==MAX_HIST_ADV_BUFF_SIZE)
      g_buff_adv_hist_firstpos=0;
  return ;
}

/*----------
@brief: search for position of id value (0x00AABBCC, AA: source id, BB: dest id, CC:packet id)
 in buffer advertising history
@input: id value
@ouput: position in the array g_buff_adv_hist, 0xFFFF if no match position
*/
uint16_t vf_find_id_buff_adv_hist3(uint32_t id)
{
  uint16_t i;

    if(g_buff_adv_hist_size==0) return 0xFFFF;
    if(g_buff_adv_hist_firstpos<g_buff_adv_hist_lastpos)
    {
        for(i=g_buff_adv_hist_firstpos;i<g_buff_adv_hist_lastpos;i++)
        {
          if(id==g_buff_adv_hist[i].id)
          {
            return i ;
          }
        }
    }
    else
    {
        for(i=g_buff_adv_hist_firstpos;i<MAX_HIST_ADV_BUFF_SIZE;i++)
        {
          if(id==g_buff_adv_hist[i].id)
          {
            return i ;
          }
        }
        for(i=0;i<g_buff_adv_hist_lastpos;i++)
        {
          if(id==g_buff_adv_hist[i].id)
          {
            return i ;
          }
        }
    }

  return 0xFFFF;
}

//vinh ver4
void vf_add_edata_adv_buff_callback(void * p_context)
{

  int i;
  thingy_data_t thingy_data;
  int16_t temperature;
  int32_t pressure;
  int16_t humidity;
  uint8_t button_state;
  uint8_t arr_data[32];
  uint8_array_t idata;
  for(i=0;i<NRF_SDH_BLE_CENTRAL_LINK_COUNT;i++)
  {

    if(m_thingy_tes_c[i].conn_handle!=BLE_CONN_HANDLE_INVALID)
    {
      temperature=vf_ble_tes_average_temperature(&g_thingy_edata[i]);
      pressure=vf_ble_tes_average_pressure(&g_thingy_edata[i]);
      humidity=vf_ble_tes_average_humidity(&g_thingy_edata[i]);
      button_state=g_thingy_edata[i].button;
      if(g_is_sink==false)
      {
         //send button state to sink
         thingy_data.local_id=m_thingy_tes_c[i].conn_handle;
         thingy_data.link_state=AGG_NODE_LINK_DATA_UPDATE;
         thingy_data.button=button_state;
         thingy_data.temperature=temperature;
         thingy_data.pressure=pressure;
         thingy_data.humidity=humidity;
         vf_adv_thingy_data(&thingy_data);  //add data to buffer
         uart_printf("Thingy ENV handle:%d, i:%d \n\r", m_thingy_tes_c[i].conn_handle,i);

      }
      else
      {

// //       idata.p_data=arr_data;
// //       //send local thingy data to phone
//        idata.p_data[0]=CLUSTER_ID; //cluster source id
//        idata.p_data[1]=SINK_ID; //cluster destination id
//        idata.p_data[2]=g_packetID++; //packet No
//        idata.p_data[3]=0; //hop counts  
//        idata.p_data[4]=AGG_NODE_LINK_DATA_UPDATE; //link state 
//        idata.p_data[5]=i; //local id
//        idata.p_data[6]=(uint8_t)(temperature>>8);//temp1
//        idata.p_data[7]=(uint8_t)(temperature&0x00FF);//temp1
//        idata.p_data[8]=(uint8_t)(pressure>>8);//pressure1
//        idata.p_data[9]=(uint8_t)(pressure&0x00FF);//pressure2         
//        idata.p_data[10]=(uint8_t)(humidity>>8);//hummi 1
//        idata.p_data[11]=(uint8_t)(humidity&0x00FF);//hummi 2
//        idata.p_data[12]=0x0;//button state
//        idata.size=13;
//        vf_app_adv_data_send_to_phone(&idata);
//        
        //vinhstop
      }
    }
  }
  

}

/*----------
@brief: search for position of id value (0x00AABBCC, AA: source id, BB: dest id, CC:packet id)
 in buffer advertising history
@input: id value
@ouput: position in the array g_buff_adv_hist, 0xFFFF if no match position
*/
void vf_refresh_history_buff_callback(void * p_context)
{
  uint16_t i,j,count=0;
  uint8_t str1[432]="delete hist buff:",str2[10];

    if(g_buff_adv_hist_size==0) return;


    if(g_buff_adv_hist_firstpos<g_buff_adv_hist_lastpos)
    {
        for(i=g_buff_adv_hist_firstpos;i<g_buff_adv_hist_lastpos;i++)
        {
          if(g_buff_adv_hist[i].TTL>0)
          {
            g_buff_adv_hist[i].TTL--; ;
          }
        }
    }
    else
    {
        for(i=g_buff_adv_hist_firstpos;i<MAX_HIST_ADV_BUFF_SIZE;i++)
        {
          if(g_buff_adv_hist[i].TTL>0)
          {
            g_buff_adv_hist[i].TTL--; ;
          }
        }
        for(i=0;i<g_buff_adv_hist_lastpos;i++)
        {
          if(g_buff_adv_hist[i].TTL>0)
          {
            g_buff_adv_hist[i].TTL--; ;
          }
        }
    }

    if(g_buff_adv_hist_firstpos<g_buff_adv_hist_lastpos)
    {
      for(i=g_buff_adv_hist_firstpos;i<g_buff_adv_hist_lastpos;i++)
      {
          if( g_buff_adv_hist[i].TTL==0) break;

      }
      if(i!=g_buff_adv_hist_lastpos)
      {
        count=1;

        sprintf(str2,"%d ",i);
        strcat(str1,str2);

        for(j=i+1;j<g_buff_adv_hist_lastpos;j++)
        {
            if(g_buff_adv_hist[j].TTL!=0)
            {
              g_buff_adv_hist[i++]=g_buff_adv_hist[j];

              sprintf(str2,"%d ",j);
              strcat(str1,str2);

            }
            else
            {
              count++;
            }

        }
        g_buff_adv_hist_lastpos=i;
        g_buff_adv_hist_size-=count;

        strcat(str1,"\n\r");
        uart_printf(str1);
      }

    }
    else
    {
      for(i=g_buff_adv_hist_firstpos;i<MAX_HIST_ADV_BUFF_SIZE;i++)
      {
          if( g_buff_adv_hist[i].TTL==0) break;

      }
      if(i!=MAX_HIST_ADV_BUFF_SIZE)
      {
        count=1;

        sprintf(str2,"%d ",i);
        strcat(str1,str2);

        for(j=i+1;j<MAX_HIST_ADV_BUFF_SIZE;j++)
        {
            if(g_buff_adv_hist[j].TTL!=0)
            {
              g_buff_adv_hist[i++]=g_buff_adv_hist[j];
              sprintf(str2,"%d ",j);
              strcat(str1,str2);

            }
            else
            {
              count++;
            }

        }
        for(j=0;j<g_buff_adv_hist_lastpos;j++)
        {
            if(g_buff_adv_hist[j].TTL!=0)
            {
              g_buff_adv_hist[i++]=g_buff_adv_hist[j];
              sprintf(str2,"%d ",j);
              strcat(str1,str2);

            }
            else
            {
              count++;
            }
            if(i>=MAX_HIST_ADV_BUFF_SIZE) i=0;

        }

        g_buff_adv_hist_lastpos=i;
        g_buff_adv_hist_size-=count;

        strcat(str1,"\n\r");
        uart_printf(str1);
      }
      else
      {
          for(i=0;i<g_buff_adv_hist_lastpos;i++)
          {
              if( g_buff_adv_hist[j].TTL==0) break;

          }
          if(i!=g_buff_adv_hist_lastpos)
          {
            count=1;
              sprintf(str2,"%d ",i);
              strcat(str1,str2);
            for(j=i+1;j<g_buff_adv_hist_lastpos;j++)
            {
                if(g_buff_adv_hist[j].TTL!=0)
                {
                  g_buff_adv_hist[i++]=g_buff_adv_hist[j];
                  sprintf(str2,"%d ",j);
                  strcat(str1,str2);
                }
                else
                {
                  count++;
                }

            }
            g_buff_adv_hist_lastpos=i;
            g_buff_adv_hist_size-=count;

            strcat(str1,"\n\r");
            uart_printf(str1);
          }
      }

    }


  


  return;


}

/*--------------------
@brief: process command from the source
    1. compare new data has been processed before by buffer of advertising history
    2. If not, process command, else quit
@input: broadcast data
*/
void vf_process_adv_command3(uint8_array_t *br_data)
{//send to phone
  //if(g_is_sink==true) vinh doing
  uint32_t ids;
  ids=(((uint32_t)br_data->p_data[0])<<16)+(((uint32_t)br_data->p_data[1])<<8)+((uint32_t)br_data->p_data[2]);
  uart_printf("ids:0x%x \n\r", ids);

  if(vf_validate_relay_packet3(br_data)==0xFFFF)
  {//new data
    vf_app_adv_data_send_to_phone(br_data); //process -> send data to phone
    vf_add_buff_adv_hist3(ids); //add this id to history buffer
  }
  else 
    uart_printf("duplicate command \n\r");
}


/*------------
start advertising dummy data using buffer
---------*/
static void vf_start_broadcast_data(void)
{


    uint8_t buff[32];
    uint8_t i;
    uint8_array_t buffdata;
    ret_code_t err_code;
    uart_printf("add start broadcast data\n\r");
    err_code=sd_ble_gap_adv_stop(m_adv_handle);
    uart_printf("$r1 %d ",err_code);
 /*
   static bool k1=false;   
    if(k1==true)
      err_code=sd_ble_gap_adv_stop(m_adv_handle);
    else
      err_code=sd_ble_gap_adv_stop(m_adv_handle);

    uart_printf("$r1 %d ",err_code);
  nrf_delay_ms(500);
*/



    buff[0]=19; //length
    buff[1]=0xff; //type
    buff[2]=CLUSTER_ID; //source
    buff[3]=SINK_ID; //destination
    buff[4]=0x3; //Packet_no
    buff[5]=0x0; //TTL
    buff[6]=0x1; 
    buff[7]=0x2;
    buff[8]=0x3;
    buff[9]=0x4;   
    buff[10]=5;             
   buff[11]=6;
  buff[12]=7;
  buff[13]=8;
  buff[14]=9;
  buff[15]=10;
  buff[16]=11;
  buff[17]=12;
  buff[18]=13;
  buff[19]=14;
  buffdata.p_data=&buff[2];
  buffdata.size=18;
  vf_add_packet_to_buffer3(&buffdata);
  //memcpy(&adv_packet.adv_data.p_data[org_adv_data_size],buff,20);


  err_code = sd_ble_gap_adv_start(m_adv_handle, APP_BLE_CONN_CFG_TAG);
    //APP_ERROR_CHECK(err_code);
  uart_printf("$r3 %d ",err_code);
}


/*------------
start advertising dummy data directly
---------*/
static void vf_start_broadcast_data2(void)
{

    uint8_t buff[32];
    uint8_t i;
    uint8_array_t buffdata;
    ret_code_t err_code;
    uart_printf("add start broadcast data\n\r");
    err_code=sd_ble_gap_adv_stop(m_adv_handle); //stop advertising
    uart_printf("$err1 %d ",err_code); 
 /*
   static bool k1=false;   
    if(k1==true)
      err_code=sd_ble_gap_adv_stop(m_adv_handle);
    else
      err_code=sd_ble_gap_adv_stop(m_adv_handle);

    uart_printf("$r1 %d ",err_code);
  nrf_delay_ms(500);
*/



    buff[0]=19; //length
    buff[1]=0xff; //type
    buff[2]=CLUSTER_ID; //source
    buff[3]=SINK_ID; //destination
    buff[4]=0x3; //Packet_no
    buff[5]=0x0; //TTL
    buff[6]=0x1; 
    buff[7]=0x2;
    buff[8]=0x3;
    buff[9]=0x4;   
    buff[10]=5;             
   buff[11]=6;
  buff[12]=7;
  buff[13]=8;
  buff[14]=9;
  buff[15]=10;
  buff[16]=11;
  buff[17]=12;
  buff[18]=13;
  buff[19]=14;
  buffdata.p_data=&buff[0];
  buffdata.size=18;
  //vf_add_packet_to_buffer2(&buffdata);
  memcpy(&adv_packet.adv_data.p_data[org_adv_data_size],buff,20);


  err_code = sd_ble_gap_adv_start(m_adv_handle, APP_BLE_CONN_CFG_TAG); //start advertising
    //APP_ERROR_CHECK(err_code);
  uart_printf("$err2 %d ",err_code);
}


static void vf_stop_broadcast_data(void)
{

    uint8_t buff[32];
    uint8_t i;
    ret_code_t err_code;
    uart_printf(" stop broadcast userdata \n\r");

    err_code=sd_ble_gap_adv_stop(m_adv_handle);
    //uart_printf("$r1 %d ",err_code);

    memset(buff,0,32);
    memcpy(&adv_packet.adv_data.p_data[org_adv_data_size],buff,20);   

       err_code = sd_ble_gap_adv_start(m_adv_handle, APP_BLE_CONN_CFG_TAG);
    //APP_ERROR_CHECK(err_code);
    //uart_printf("$r3 %d ",err_code);
}

bool vf_check_source(uint8_array_t *data)
{
  if((data->p_data[0])==CLUSTER_ID) return true;
  else return false;
}

bool vf_check_destination(uint8_array_t *data)
{
  if((data->p_data[1])==CLUSTER_ID) return true;
  else return false;
}


void vf_adv_thingy_data(thingy_data_t *data)
{


  uint8_t data_arr[32],str1[20],i;
  uint8_array_t idata;
//vinh doing

  idata.p_data=data_arr;
  idata.p_data[0]=CLUSTER_ID; //cluster source id
  idata.p_data[1]=SINK_ID; //cluster destination id
  idata.p_data[2]=g_packetID++; //packet No
  idata.p_data[3]=0; //hop counts  
  idata.p_data[4]=data->link_state; //link state 
  idata.p_data[5]=data->local_id; //local id
  switch(data->link_state)
  {
    case AGG_NODE_LINK_CONNECTED:

        idata.p_data[6]=0x12;//temp1
        idata.p_data[7]=0x12;//temp1
        idata.p_data[8]=0x00;//pressure1
        idata.p_data[9]=0x01;//pressure2 
        idata.p_data[10]=0x02;//pressure3
        idata.p_data[11]=0x03;//pressure4                
        idata.p_data[12]=0x00;//hummi 1
        idata.p_data[13]=0x78;//hummi 2
        idata.p_data[14]=0x0;//button state
        idata.size=15;
        break;

    case AGG_NODE_LINK_DISCONNECTED:
        idata.size=6;
        break;

    case AGG_NODE_LINK_DATA_UPDATE:
        idata.p_data[6]=data->temperature>>8;//temp1
        idata.p_data[7]=(uint8_t)(data->temperature&0x00FF);//temp1
        idata.p_data[8]=data->pressure>>24;//pressure1 MSB
        idata.p_data[9]=data->pressure>>16;//pressure2
        idata.p_data[10]=data->pressure>>8;//pressure3
        idata.p_data[11]=(uint8_t)(data->pressure&0x00FF);//pressure4  LSB                   

        idata.p_data[12]=data->humidity>>8;//hummi 1
        idata.p_data[13]=(uint8_t)(data->humidity&0x00FF);//hummi 2
        idata.p_data[14]=data->button;//button state
        idata.size=15;
        break;
  }


  uart_printf(" Add Thingy data to buffer: ");
  for(i=0;i<idata.size;i++)
  {
    sprintf(str1,"%d ",idata.p_data[i]);
    uart_printf(str1);
  }
  uart_printf("\r\n");
  vf_add_packet_to_buffer3(&idata);
  
  //vinh ver3
 // relay_adv_data2();
}


uint8_t vf_add_packet_to_buffer(uint8_array_t *data)
{
  uint8_array_t *userdata=data;
  uint8_t err_code=0;
  size_t trim_pos,i;
  if((g_userdata.size+userdata->size+1)<=MAX_USERDATA_BUFFER)
  {//buffer not overflow
      g_userdata.size+=userdata->size+1;
      g_userdata.p_data[g_userdata_lastpos++]=userdata->size+1;

      if((g_userdata_lastpos+userdata->size)>=MAX_USERDATA_BUFFER)
      {
          memcpy(&g_userdata.p_data[g_userdata_lastpos],userdata->p_data,MAX_USERDATA_BUFFER-g_userdata_lastpos);
          trim_pos=MAX_USERDATA_BUFFER-g_userdata_lastpos;
          g_userdata_lastpos=userdata->size-(MAX_USERDATA_BUFFER-g_userdata_lastpos);
          memcpy(&g_userdata.p_data[0],&userdata->p_data[trim_pos],g_userdata_lastpos);
      }
      else
      {
         memcpy(&g_userdata.p_data[g_userdata_lastpos],userdata->p_data,userdata->size);
         g_userdata_lastpos+=userdata->size;
      }
      uart_printf("add packet to buffer");
      uart_printf("@%d $%d $%d *",g_userdata_lastpos,userdata->size,g_userdata.size);
      if(g_userdata_firstpos+g_userdata.size<MAX_USERDATA_BUFFER)
      {
          for(i=g_userdata_firstpos;i<g_userdata_firstpos+g_userdata.size;i++)
          {
           uart_printf("%d ", g_userdata.p_data[i]);
            //sprintf(str_uart, "%d ",userdata.p_data[i]);
          }
      }
      else
      {
            for(i=g_userdata_firstpos;i<MAX_USERDATA_BUFFER;i++)
            {
             uart_printf("%d ", g_userdata.p_data[i]);
            //sprintf(str_uart, "%d ",userdata.p_data[i]);
            }
            for(i=0;i<g_userdata_lastpos;i++)
            {
             uart_printf("%d ", g_userdata.p_data[i]);
            //sprintf(str_uart, "%d ",userdata.p_data[i]);
            }
      }
      uart_printf("\n\r");
  }//end buffer not full
  else
  {
    uart_printf("Buffer full");
    err_code=1;
  }
  return err_code;
}


uint8_t vf_add_packet_to_buffer2(uint8_array_t *data)
{
//each block include 32 bytes
//byte 0: block property: 0x0: free, 0xFF: used
//byte 1: pointer to next available block,0xFF is NULL
//byte 2: pointer to previous available block,0xFF is NULL
//byte 4..n:size + data
//g_userdata_lastpos: position of last block in buffer
//g_userdata_firstpos: position of last block in buffer
  uint8_array_t *userdata=data;
  uint8_t err_code=1;
  size_t pos,i;
  if((g_userdata.size)<MAX_USERDATA_BUFFER_BLOCK)
  {//buffer not overflow

      for(i=0;i<MAX_USERDATA_BUFFER_BLOCK;i++)
      {
        if(g_userdata.p_data[i*MAX_USERDATA_BUFFER_BLOCKSIZE]==0) //found free block
        {
          err_code=0;
          break;
        }
      }
      
      if(err_code==0)
      {  
      //found free block
        pos=i;
        if( g_userdata.size==0)
        {
          g_userdata.p_data[pos*32+1]=g_userdata.p_data[pos*32+2]=0xFF; 
          g_userdata_currpos=g_userdata_lastpos=g_userdata_firstpos=pos;
        }
        else
        {
          g_userdata.p_data[g_userdata_lastpos*32+1]=pos; //update previous "next block pointer"
          g_userdata.p_data[pos*32+2]=g_userdata_lastpos; //update current "pre block pointer"
          g_userdata_lastpos=pos; //update last position
        }
        g_userdata.p_data[pos*MAX_USERDATA_BUFFER_BLOCKSIZE]=0xFF;  //mark as used block
        g_userdata.p_data[pos*MAX_USERDATA_BUFFER_BLOCKSIZE+4]=userdata->size+1; //number of data byte in this block, include this byte
        memcpy(&g_userdata.p_data[pos*MAX_USERDATA_BUFFER_BLOCKSIZE+5],userdata->p_data,userdata->size);
        g_userdata.size++; 
      }
  

  }//end buffer not full

  if (err_code==0)
  {
       uart_printf("@%d $%d $%d *",g_userdata.size,g_userdata_lastpos,userdata->size); 
       for(i=0;i<userdata->size+1+4;i++)
       {
          uart_printf("%d ", g_userdata.p_data[g_userdata_lastpos*MAX_USERDATA_BUFFER_BLOCKSIZE+i]);
       }
      uart_printf("\n\r");
  }
  else
  {
    uart_printf("Buffer full");
  }
  return err_code;
}


/*--------------------------------
@brief: add broadcast data to buffer for advertising
@input: user data in *data
@output: return value 0: success, 1: buffer full

data is coupled into buffer at the posiont pointed by g_userdata_lastpos

Structure of a block (32 bytes)
  byte 0: block property: 0x0: free, 0xFF: used
  byte 1: pointer to next available block,0xFF is NULL
  byte 2: pointer to previous available block,0xFF is NULL
  byte 3: number of advertsing times before removing
  byte 4: size (=(size of broadcase data)+1)
  byte 5..n: broadcast data
          broadcast data (br_data)
            byte 0: source id
            byte 1: destination id
            byte 2: packet id
            byte 3: hop counts
            byte 4..18: user data 

//g_userdata_lastpos: position of last block in buffer
//g_userdata_firstpos: position of last block in buffer
//g_userdata_currpos: position of current block which is adverting
//g_userdata : buffer
---------*/

uint8_t vf_add_packet_to_buffer3(uint8_array_t *br_data)
{

  uint8_array_t *userdata=br_data;
  uint8_t err_code=1;
  size_t pos,i;
  if((g_userdata.size)<MAX_USERDATA_BUFFER_BLOCK)
  {//buffer not overflow

      for(i=0;i<MAX_USERDATA_BUFFER_BLOCK;i++)
      {
        if(g_userdata.p_data[i*MAX_USERDATA_BUFFER_BLOCKSIZE]==0) //found free block
        {
          err_code=0;
          break;
        }
      }
      
      if(err_code==0)
      {  
      //found free block
        pos=i;
        if( g_userdata.size==0)
        {
          g_userdata.p_data[pos*MAX_USERDATA_BUFFER_BLOCKSIZE+1]=g_userdata.p_data[pos*MAX_USERDATA_BUFFER_BLOCKSIZE+2]=0xFF; //pointer to next and previous block
          g_userdata_currpos=g_userdata_lastpos=g_userdata_firstpos=pos;
        }
        else
        {
          g_userdata.p_data[g_userdata_lastpos*MAX_USERDATA_BUFFER_BLOCKSIZE+1]=pos; //update previous "next block pointer"
          g_userdata.p_data[pos*MAX_USERDATA_BUFFER_BLOCKSIZE+2]=g_userdata_lastpos; //update current "pre block pointer"
          g_userdata.p_data[pos*MAX_USERDATA_BUFFER_BLOCKSIZE+1]=0xFF; //pointer to next block is NULL
          g_userdata_lastpos=pos; //update last position
        }
        g_userdata.p_data[pos*MAX_USERDATA_BUFFER_BLOCKSIZE]=0xFF;  //mark as used block
        g_userdata.p_data[pos*MAX_USERDATA_BUFFER_BLOCKSIZE+3]=2;  //advertise 2 times before remove this block
        g_userdata.p_data[pos*MAX_USERDATA_BUFFER_BLOCKSIZE+4]=userdata->size+1; //number of data byte in this block, include this byte
        memcpy(&g_userdata.p_data[pos*MAX_USERDATA_BUFFER_BLOCKSIZE+5],userdata->p_data,userdata->size);
        g_userdata.size++;
      } 

  }//end buffer not overflow

  if (err_code==0)
  {
       uart_printf("add data to buffer firstpos:%d, currpos:%d, lastpos:$%d buffsize:$%d *",g_userdata_firstpos,g_userdata_currpos,g_userdata_lastpos,g_userdata.size); 
       for(i=0;i<userdata->size+1+4;i++)
       {
          uart_printf("%d ", g_userdata.p_data[g_userdata_lastpos*MAX_USERDATA_BUFFER_BLOCKSIZE+i]);
       }
      uart_printf("\n\r");
  }
  else
  {
    uart_printf("Buffer full");
  }
  return err_code;
}

/**@brief Handles events coming from the Thingy Environment central module.
 */
static void vf_tes_c_evt_handler(ble_tes_c_t * p_tes_c, ble_tes_c_evt_t * p_tes_c_evt)
{
    uint16_t connection_handle=p_tes_c_evt->conn_handle;
    switch (p_tes_c_evt->evt_type)
    {
        case BLE_TES_C_EVT_DISCOVERY_COMPLETE:
        {
            ret_code_t err_code;

           err_code = ble_tes_c_handles_assign(&m_thingy_tes_c[p_tes_c_evt->conn_handle],
                                               p_tes_c_evt->conn_handle,
                                               &p_tes_c_evt->params.peer_db);
            uart_printf("Thingy Environment service discovered on conn_handle 0x%x.", p_tes_c_evt->conn_handle);

            // Thingy Environment service discovered. Enable notification of sensor data.
            err_code = ble_tes_c_temperature_notif_enable(p_tes_c);
            APP_ERROR_CHECK(err_code);
            err_code = ble_tes_c_pressure_notif_enable(p_tes_c);
            APP_ERROR_CHECK(err_code);
            err_code = ble_tes_c_humidity_notif_enable(p_tes_c);
            APP_ERROR_CHECK(err_code);
            //err_code = ble_tes_c_gas_notif_enable(p_tes_c);
            //APP_ERROR_CHECK(err_code);
            //err_code = ble_tes_c_color_notif_enable(p_tes_c);
            //APP_ERROR_CHECK(err_code);
//            err_code = ble_tes_c_config_notif_enable(p_tes_c);
//            APP_ERROR_CHECK(err_code);
            
        } break; // BLE_TES_C_EVT_DISCOVERY_COMPLETE

        case BLE_TES_C_EVT_TEMPERATURE_NOTIFICATION:
        {
            ble_tes_temperature_t temperature = p_tes_c_evt->params.value.temperature_data;
            vf_ble_tes_add_sum_temperature(&g_thingy_edata[connection_handle],temperature);         

            uart_printf("Got Thingy @%d temperature: %d,%d, sum:%d \n\r",p_tes_c_evt->conn_handle,\
                    temperature.integer, temperature.decimal,g_thingy_edata[connection_handle].temperature);
        } break; // BLE_TES_C_EVT_TEMPERATURE_NOTIFICATION
        case BLE_TES_C_EVT_PRESSURE_NOTIFICATION:
        {
            ble_tes_pressure_t pressure = p_tes_c_evt->params.value.pressure_data;
            vf_ble_tes_add_sum_pressure(&g_thingy_edata[connection_handle],pressure); 

            uart_printf("Got Thingy @%d pressure: %d,%d \n\r",connection_handle, pressure.integer, pressure.decimal);
        } break; // BLE_TES_C_EVT_PRESSURE_NOTIFICATION
        case BLE_TES_C_EVT_HUMIDITY_NOTIFICATION:
        {
            ble_tes_humidity_t humidity = p_tes_c_evt->params.value.humidity_data;
            vf_ble_tes_add_sum_humidity(&g_thingy_edata[connection_handle],humidity); 

            uart_printf("Got Thingy @%d humidity: %d \n\r",connection_handle,humidity);
        } break; // BLE_TES_C_EVT_HUMIDITY_NOTIFICATION
        case BLE_TES_C_EVT_GAS_NOTIFICATION:
        {
            ble_tes_gas_t gas = p_tes_c_evt->params.value.gas_data;
            uart_printf("Got C02: %d \n\r", gas.eco2_ppm);
            uart_printf("Got organic components: %d \n\r", gas.tvoc_ppb);
        } break; // BLE_TES_C_EVT_GAS_NOTIFICATION
        case BLE_TES_C_EVT_COLOR_NOTIFICATION:
        {
            ble_tes_color_t color = p_tes_c_evt->params.value.color_data;
            uart_printf("Got color. R%d, G%d, B%d, C%d \n\r", color.red, color.green, color.blue, color.clear);
        } break; // BLE_TES_C_EVT_COLOR_NOTIFICATION
        case BLE_TES_C_EVT_CONFIG_NOTIFICATION:
        {
            // No implementation. 
        } break; // BLE_TES_C_EVT_CONFIG_NOTIFICATION

        default:
            // No implementation needed.
            break;
    }
}





/**@brief Function for setting up advertising data. */
static void advertising_data_set(void)
{
    ret_code_t err_code;

    
    adv_data.name_type          = BLE_ADVDATA_FULL_NAME;
    adv_data.flags              = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    adv_data.include_appearance = false;


    sr_data.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    sr_data.uuids_complete.p_uuids = m_adv_uuids;
 
 
    adv_packet.adv_data.p_data = &adv_data_buf[0];
    adv_packet.adv_data.len = ADV_MAX_LENGTH;
    adv_packet.scan_rsp_data.p_data = sr_data_buf;
    adv_packet.scan_rsp_data.len = ADV_MAX_LENGTH;

    


    err_code = ble_advdata_encode(&adv_data, adv_packet.adv_data.p_data, &adv_packet.adv_data.len);
    APP_ERROR_CHECK(err_code);
    
    err_code = ble_advdata_encode(&sr_data, adv_packet.scan_rsp_data.p_data, &adv_packet.scan_rsp_data.len);
    APP_ERROR_CHECK(err_code);

   //vinh
    org_adv_data_size=adv_packet.adv_data.len;
    uart_printf("%d  ",adv_packet.adv_data.len );
    

    //student: initial advertising data
    org_adv_data_size=adv_packet.adv_data.len;  //mark the start position of datafield 


    memset(&adv_packet.adv_data.p_data[org_adv_data_size],0,20);
    adv_packet.adv_data.len=org_adv_data_size+20; //length

    
    //adv_params.properties.connectable = 1;
    //adv_params.properties.scannable = 1;
    //adv_params.properties.legacy_pdu = 1;
    adv_params.properties.type = BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED;
    adv_params.p_peer_addr   = NULL;
    adv_params.filter_policy = BLE_GAP_ADV_FP_ANY;
    adv_params.interval      = PERIPHERAL_ADV_INTERVAL;
    adv_params.duration      = PERIPHERAL_ADV_TIMEOUT_IN_SECONDS * 100;
    adv_params.primary_phy   = BLE_GAP_PHY_1MBPS;
    adv_params.secondary_phy = BLE_GAP_PHY_1MBPS;

    memcpy((void*)&adv_params2,(void*)&adv_params,sizeof(adv_params));

    err_code = sd_ble_gap_adv_set_configure(&m_adv_handle, &adv_packet, &adv_params);
    APP_ERROR_CHECK(err_code);
    
 
}


/**@brief Function for starting advertising. */
static void advertising_start(void)
{
    NRF_LOG_INFO("Starting advertising.");
    
    ret_code_t err_code = sd_ble_gap_adv_start(m_adv_handle, APP_BLE_CONN_CFG_TAG);
    APP_ERROR_CHECK(err_code);
    
    err_code = app_timer_start(m_adv_led_blink_timer_id, APP_TIMER_TICKS(500), 0);
    APP_ERROR_CHECK(err_code);

    //vinh, start advertising timer with 200ms interval to change advertising packet in buffer
    err_code = app_timer_start(m_adv_timer_id, APP_TIMER_TICKS(200), 0);
    APP_ERROR_CHECK(err_code);

    //timer for delete a history id if history buffer
    err_code = app_timer_start(m_hist_refresh_timer_id, APP_TIMER_TICKS(1000), 0);
    //APP_ERROR_CHECK(err_code);

}


/**@brief Function for writing to the LED characteristic of all connected clients.
 *
 * @details Based on if the button is pressed or released, this function writes a high or low
 *          LED status to the server.
 *
 * @param[in] button_action The button action (press/release).
 *            Determines if the LEDs of the servers will be ON or OFF.
 *
 * @return If successful NRF_SUCCESS is returned. Otherwise, the error code from @ref ble_lbs_led_status_send.
 */
static ret_code_t led_status_send_to_all(uint8_t button_action)
{
    ret_code_t err_code;

    for (uint32_t i = 0; i< NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++)
    {
        err_code = ble_lbs_led_status_send(&m_lbs_c[i], button_action);

        if(err_code != NRF_SUCCESS)
        {
            err_code = ble_thingy_uis_led_set_on_off(&m_thingy_uis_c[i], button_action);
            //err_code = ble_thingy_uis_led_set_constant(&m_thingy_uis_c[i], button_action ? 255 : 0, button_action ? 255 : 0, button_action ? 255 : 0);
            if (err_code != NRF_SUCCESS &&
                err_code != BLE_ERROR_INVALID_CONN_HANDLE &&
                err_code != NRF_ERROR_INVALID_STATE)
            {
                return err_code;
            }
        }
    }
    return NRF_SUCCESS;
}
 
 
static ret_code_t led_status_send_by_mask(uint8_t button_action, uint8_t r, uint8_t g, uint8_t b, uint32_t mask)
{
    ret_code_t err_code;
    uint8_t colors[3] = {r, g, b};
    
    app_aggregator_on_led_color_set(r, g, b, mask);

    for (uint32_t i = 0; i < NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++)
    {
        if((mask & (1 << i)) != 0)
        {
            // First, try to access the devices as a Blinky device
            err_code = ble_lbs_led_color_send(&m_lbs_c[i], colors);
            if(err_code != NRF_SUCCESS)
            {
                // If the blinky call fails, assume this is a Thingy device
                err_code = ble_thingy_uis_led_set_constant(&m_thingy_uis_c[i], button_action ? r : 0, button_action ? g : 0, button_action ? b : 0);
                if (err_code != NRF_SUCCESS &&
                    err_code != BLE_ERROR_INVALID_CONN_HANDLE &&
                    err_code != NRF_ERROR_INVALID_STATE)
                {
                    return err_code;
                }
            }
        }
    }
    return NRF_SUCCESS;    
}

ret_code_t led_status_on_off_send_by_mask(bool on, uint32_t mask)
{
    ret_code_t err_code;

    app_aggregator_on_led_update(on, mask);
    
    for (uint32_t i = 0; i < NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++)
    {
        if((mask & (1 << i)) != 0)
        {
            err_code = ble_lbs_led_status_send(&m_lbs_c[i], on);
            if(err_code != NRF_SUCCESS)
            {
                err_code = ble_thingy_uis_led_set_on_off(&m_thingy_uis_c[i], on);
                if (err_code != NRF_SUCCESS &&
                    err_code != BLE_ERROR_INVALID_CONN_HANDLE &&
                    err_code != NRF_ERROR_INVALID_STATE)
                {
                    return err_code;
                }
            }
        }
    }
    return NRF_SUCCESS;      
}

ret_code_t post_connect_message(uint8_t conn_handle)
{
    ret_code_t err_code = NRF_SUCCESS;
    for(int i = 0; i < NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++)
    {
        if(m_lbs_c[i].conn_handle == conn_handle)
        {
            err_code = ble_thingy_uis_led_set_on_off(&m_thingy_uis_c[i], true);
        }
        
        if(m_thingy_uis_c[i].conn_handle == conn_handle)
        {
            err_code = ble_thingy_uis_led_set_constant(&m_thingy_uis_c[i], 255, 255, 255);  
        }
    }
    return err_code;
}

ret_code_t disconnect_all_peripherals()
{
    ret_code_t err_code;
    for(int i = 0; i < NRF_SDH_BLE_CENTRAL_LINK_COUNT; i++)
    {
        if(m_lbs_c[i].conn_handle != BLE_CONN_HANDLE_INVALID)
        {
            err_code = sd_ble_gap_disconnect(m_lbs_c[i].conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if(err_code != NRF_SUCCESS)
            {
                //return err_code;
            }
        }
        
        else if(m_thingy_uis_c[i].conn_handle != BLE_CONN_HANDLE_INVALID)
        {
            err_code = sd_ble_gap_disconnect(m_thingy_uis_c[i].conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if(err_code != NRF_SUCCESS)
            {
                //return err_code;
            }
        }
    }    
    return NRF_SUCCESS;
}

/**@brief Function for handling events from the button handler module.
 *
 * @param[in] pin_no        The pin that the event applies to.
 * @param[in] button_action The button action (press/release).
 */


static void button_event_handler(uint8_t pin_no, uint8_t button_action)
{
    ret_code_t err_code;
    uint8_array_t userdata;
    static bool k=true;
    switch (pin_no)
    {
        case CENTRAL_DISCONNECT_BUTTON:

/*
            if(m_per_con_handle != BLE_CONN_HANDLE_INVALID)
            {
                sd_ble_gap_disconnect(m_per_con_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            }
            break;
  */
        //vinh
        if(k==true)
        {
                    vf_stop_broadcast_data();
            k=false;
        }
        else
          k=true;
                    break;

        case SCAN_START_STOP_BUTTON:
            if(button_action == APP_BUTTON_PUSH)
            {
                m_scanning_enabled = !m_scanning_enabled;
                if(m_scanning_enabled)
                {
                    scan_start(false);
                }
                else
                {
                    scan_stop();
                }
            }
            break;

        case LEDBUTTON_BUTTON:
            err_code = led_status_send_to_all(button_action);
            if (err_code == NRF_SUCCESS)
            {
                NRF_LOG_INFO("LBS write LED state %d", button_action);
            }
            //vinh
          if(k==true)
          {
          vf_start_broadcast_data();
          k=false;
          }
          else
           k=true;

        //vinh
        case TEST_BUTTON:
         

          break;
        default:
            APP_ERROR_HANDLER(pin_no);
            break;
    }
}


/**@brief Function for initializing the button handler module.
 */
static void buttons_init(void)
{
    ret_code_t err_code;

   // The array must be static because a pointer to it will be saved in the button handler module.
    static app_button_cfg_t buttons[] =
    {
        {CENTRAL_DISCONNECT_BUTTON, false, BUTTON_PULL, button_event_handler},
        {SCAN_START_STOP_BUTTON,    false, BUTTON_PULL, button_event_handler},
        {LEDBUTTON_BUTTON,          false, BUTTON_PULL, button_event_handler}
    };

    err_code = app_button_init(buttons, ARRAY_SIZE(buttons), BUTTON_DETECTION_DELAY);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling database discovery events.
 *
 * @details This function is callback function to handle events from the database discovery module.
 *          Depending on the UUIDs that are discovered, this function should forward the events
 *          to their respective services.
 *
 * @param[in] p_event  Pointer to the database discovery event.
 */
static void db_disc_handler(ble_db_discovery_evt_t * p_evt)
{
    NRF_LOG_DEBUG("call to ble_lbs_on_db_disc_evt for instance %d and link 0x%x!",
                  p_evt->conn_handle,
                  p_evt->conn_handle);

    ble_lbs_on_db_disc_evt(&m_lbs_c[p_evt->conn_handle], p_evt);
    ble_thingy_uis_on_db_disc_evt(&m_thingy_uis_c[p_evt->conn_handle], p_evt);
    ble_tes_on_db_disc_evt(&m_thingy_tes_c[p_evt->conn_handle], p_evt);

}


/** @brief Database discovery initialization.
 */
static void db_discovery_init(void)
{
    ret_code_t err_code = ble_db_discovery_init(db_disc_handler);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling an event from the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module
 *          which are passed to the application.
 *
 * @note All this function does is to disconnect. This could have been done by simply setting
 *       the disconnect_on_fail config parameter, but instead we use the event handler
 *       mechanism to demonstrate its use.
 *
 * @param[in] p_evt  Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    uint32_t err_code;

    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_per_con_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for handling errors from the Connection Parameters module.
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


/**@brief Function for initializing power management.
 */
static void power_management_init(void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}


/** @brief Function to sleep until a BLE event is received by the application.
 */
static void power_manage(void)
{
    ret_code_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the idle state (main loop).
 *
 * @details Handle any pending log operation(s), then sleep until the next event occurs.
 */
static void idle_state_handle(void)
{
    if (NRF_LOG_PROCESS() == false)
    {
        nrf_pwr_mgmt_run();
    }
}


/** @brief Function for initializing the log module.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();    
}


void uart_error_handle(app_uart_evt_t * p_event)
{
    if (p_event->evt_type == APP_UART_COMMUNICATION_ERROR)
    {
        APP_ERROR_HANDLER(p_event->data.error_communication);
    }
    else if (p_event->evt_type == APP_UART_FIFO_ERROR)
    {
        APP_ERROR_HANDLER(p_event->data.error_code);
    }
}


static void uart_init()
{
    uint32_t err_code;
    const app_uart_comm_params_t comm_params =
    {
        RX_PIN_NUMBER,
        TX_PIN_NUMBER,
        RTS_PIN_NUMBER,
        CTS_PIN_NUMBER,
        HWFC,
        false,
        UART_BAUDRATE_BAUDRATE_Baud460800
    };

    APP_UART_FIFO_INIT(&comm_params,
                       16,
                       1024,
                       uart_error_handle,
                       APP_IRQ_PRIORITY_LOWEST,
                       err_code);

    APP_ERROR_CHECK(err_code);
}

void post_message_connect_callback(void *p)
{
    app_aggregator_update_link_status();
}

/** @brief Function for initializing the timer.
 */
static void timer_init(void)
{
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_adv_led_blink_timer_id, APP_TIMER_MODE_REPEATED, adv_led_blink_callback);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_scan_led_blink_timer_id, APP_TIMER_MODE_REPEATED, scan_led_blink_callback);
    APP_ERROR_CHECK(err_code);
    
    err_code = app_timer_create(&m_post_message_delay_timer_id, APP_TIMER_MODE_SINGLE_SHOT, post_message_connect_callback);

    //vinh ver3
    err_code = app_timer_create(&m_adv_timer_id, APP_TIMER_MODE_REPEATED, vf_relay_adv_data3);
    err_code = app_timer_create(&m_hist_refresh_timer_id, APP_TIMER_MODE_REPEATED, vf_refresh_history_buff_callback);

    //vinh ver4
    err_code = app_timer_create(&m_add_edata_adv_buff_timer_id, APP_TIMER_MODE_REPEATED, vf_add_edata_adv_buff_callback);

}


/**@brief Function for initializing the GATT module.
 */
static void gatt_init(void)
{
    ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, NULL);
    APP_ERROR_CHECK(err_code);
    
    err_code = nrf_ble_gatt_att_mtu_periph_set(&m_gatt, 64);
    APP_ERROR_CHECK(err_code);
}


static void process_app_commands()
{//process command from phone           
    if(agg_cmd_received != 0)
    {          
        uint32_t mask;
        NRF_LOG_INFO("APP COMMAND %d",agg_cmd_received);
        switch(agg_cmd_received)
        {
            case APPCMD_SET_LED_ALL: //change LED colors of all
                for(int i = 2; i >= 0; i--)
                {
                    mask = mask << 8 | agg_cmd[4 + i];
                }
                led_status_send_by_mask(agg_cmd[0], agg_cmd[1], agg_cmd[2], agg_cmd[3], mask);
                break;
                
            case APPCMD_SET_LED_ON_OFF_ALL: //on/of LED
                for(int i = 2; i >= 0; i--)
                {
                    mask = mask << 8 | agg_cmd[1 + i];
                }
                led_status_on_off_send_by_mask(agg_cmd[0], mask);
                break;
                
            case APPCMD_POST_CONNECT_MESSAGE:
                post_connect_message(agg_cmd[0]);
                break;
            
            case APPCMD_DISCONNECT_PERIPHERALS:
                disconnect_all_peripherals();
                break;

            case APPCMD_DISCONNECT_CENTRAL://disconnect to phone event
                sd_ble_gap_disconnect(m_per_con_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
                break;
            
            default:
                break;
        }
        agg_cmd_received = 0;
    }
}





int main(void)
{
    uint32_t err_code;

    //vinh
    //name clusterhead and string to detect blinky group
    char str1[4],s;
    sprintf(str1,"%d",CLUSTER_ID);
 
    strcat(m_target_clusterhead_name,(const char*)str1);
    /*s='A'+CLUSTER_ID-1;
    m_target_blinky_name[0]=m_target_blinky_name[1]=s;
    m_target_blinky_name[2]=0;*/
    g_userdata_currpos=g_userdata_lastpos=g_userdata_firstpos;
    g_userdata.size=0;  
    memset(garr_userdata,0,sizeof(garr_userdata));
    memset(&g_buff_adv_hist,0,sizeof(g_buff_adv_hist));
    g_buff_adv_hist_firstpos=g_buff_adv_hist_lastpos=0;
    g_buff_adv_hist_size=0;

    memset(g_thingy_edata,0,sizeof(g_thingy_edata));

    if(CLUSTER_ID==SINK_ID)
      g_is_sink=true;
    else
      g_is_sink=false;


    log_init();
    timer_init();
    uart_init();
    leds_init();
    buttons_init();
    ble_stack_init();
    gap_params_init();
    gatt_init();
    services_init();
    app_aggregator_init(&m_agg_cfg_service);
    db_discovery_init();
    lbs_c_init();
    thingy_uis_c_init();
    thingy_tes_c_init(); //vinh ver4, thingy enviroment sevice initial
    ble_conn_state_init();
    advertising_data_set();
    conn_params_init();
    
    enable_gpio_debug();

    NRF_LOG_INFO("Multilink example started");
    //uart_printf("Multilink example started. Group name \"%s\"\r\n", m_target_periph_name); //vinh, change group name
    uart_printf("Multilink example started. Device Name \"%s\"\r\n", m_target_clusterhead_name); //vinh, change group name
    uart_printf("Multilink example started. Thingy Group \"%s\"\r\n", m_target_blinky_name); //vinh, change group name

    

    for (int i = 0; i < NRF_SDH_BLE_TOTAL_LINK_COUNT; ++i){
        m_coded_phy_conn_handle[i] = BLE_CONN_HANDLE_INVALID; //vinh, init the handle
    }
    // Start scanning for peripherals and initiate connection to devices which  advertise.
    scan_start(false);
    err_code = app_timer_start(m_add_edata_adv_buff_timer_id, APP_TIMER_TICKS(10000), 0); //timer for perodically updating Thingy data to phone
    APP_ERROR_CHECK(err_code);

    // Start advertising
    advertising_start();
    
    err_code = app_button_enable();
    APP_ERROR_CHECK(err_code);
    
    for (;;)
    {
        if(m_per_con_handle != BLE_CONN_HANDLE_INVALID) //vinh BLE is ongoing
        {
            while(app_aggregator_flush_ble_commands()); //flush all command in buffer
        }

        process_app_commands();

        device_list_print();
        
        while(NRF_LOG_PROCESS());

        power_manage();
    }
}