/**
 * Copyright (c) 2012 - 2017, Nordic Semiconductor ASA
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
#include "sdk_common.h"

#include "ble_thingy_uis_c.h"
#include "ble_db_discovery.h"
#include "ble_types.h"
#include "ble_srv_common.h"
#include "ble_gattc.h"
#define NRF_LOG_MODULE_NAME ble_thingy_uis_c
#include "nrf_log.h"

//vinh
extern void uart_printf(const char *fmt, ...);

NRF_LOG_MODULE_REGISTER();

#define TX_BUFFER_MASK         0x07                  /**< TX Buffer mask, must be a mask of continuous zeroes, followed by continuous sequence of ones: 000...111. */
#define TX_BUFFER_SIZE         (TX_BUFFER_MASK + 1)  /**< Size of send buffer, which is 1 higher than the mask. */

#define TUIS_WRITE_MESSAGE_LENGTH   8                     /**< Length of the write message for CCCD. */

typedef enum
{
    READ_REQ,  /**< Type identifying that this tx_message is a read request. */
    WRITE_REQ  /**< Type identifying that this tx_message is a write request. */
} tx_request_t;

/**@brief Structure for writing a message to the peer, i.e. CCCD.
 */
typedef struct
{
    uint8_t                  gattc_value[TUIS_WRITE_MESSAGE_LENGTH];  /**< The message to write. */
    ble_gattc_write_params_t gattc_params;                       /**< GATTC parameters for this message. */
} write_params_t;

/**@brief Structure for holding data to be transmitted to the connected central.
 */
typedef struct
{
    uint16_t     conn_handle;  /**< Connection handle to be used when transmitting this message. */
    tx_request_t type;         /**< Type of this message, i.e. read or write message. */
    union
    {
        uint16_t       read_handle;  /**< Read request message. */
        write_params_t write_req;    /**< Write request message. */
    } req;
} tx_message_t;


static tx_message_t m_tx_buffer[TX_BUFFER_SIZE];  /**< Transmit buffer for messages to be transmitted to the central. */
static uint32_t     m_tx_insert_index = 0;        /**< Current index in the transmit buffer where the next message should be inserted. */
static uint32_t     m_tx_index = 0;               /**< Current index in the transmit buffer from where the next message to be transmitted resides. */


/**@brief Function for passing any pending request from the buffer to the stack.
 */
static void tx_buffer_process(void)
{
    if (m_tx_index != m_tx_insert_index)
    {
        uint32_t err_code;

        if (m_tx_buffer[m_tx_index].type == READ_REQ)
        {
            err_code = sd_ble_gattc_read(m_tx_buffer[m_tx_index].conn_handle,
                                         m_tx_buffer[m_tx_index].req.read_handle,
                                         0);
        }
        else
        {
            err_code = sd_ble_gattc_write(m_tx_buffer[m_tx_index].conn_handle,
                                          &m_tx_buffer[m_tx_index].req.write_req.gattc_params);
        }
        if (err_code == NRF_SUCCESS)
        {
            NRF_LOG_DEBUG("SD Read/Write API returns Success..");
            m_tx_index++;
            m_tx_index &= TX_BUFFER_MASK;
        }
        else
        {
            NRF_LOG_DEBUG("SD Read/Write API returns error. This message sending will be "
                "attempted again..");
        }
    }
}


/**@brief Function for handling write response events.
 *
 * @param[in] p_ble_thingy_uis_c Pointer to the Led Button Client structure.
 * @param[in] p_ble_evt   Pointer to the BLE event received.
 */
static void on_write_rsp(ble_thingy_uis_c_t * p_ble_thingy_uis_c, ble_evt_t const * p_ble_evt)
{
    // Check if the event if on the link for this instance
    if (p_ble_thingy_uis_c->conn_handle != p_ble_evt->evt.gattc_evt.conn_handle)
    {
        return;
    }
    // Check if there is any message to be sent across to the peer and send it.
    tx_buffer_process();
}


/**@brief Function for handling Handle Value Notification received from the SoftDevice.
 *
 * @details This function will uses the Handle Value Notification received from the SoftDevice
 *          and checks if it is a notification of Button state from the peer. If
 *          it is, this function will decode the state of the button and send it to the
 *          application.
 *
 * @param[in] p_ble_thingy_uis_c Pointer to the Led Button Client structure.
 * @param[in] p_ble_evt   Pointer to the BLE event received.
 */
static void on_hvx(ble_thingy_uis_c_t * p_ble_thingy_uis_c, ble_evt_t const * p_ble_evt)
{
    // Check if the event is on the link for this instance
    if (p_ble_thingy_uis_c->conn_handle != p_ble_evt->evt.gattc_evt.conn_handle)
    {
        return;
    }
    // Check if this is a Button notification.
    if (p_ble_evt->evt.gattc_evt.params.hvx.handle == p_ble_thingy_uis_c->peer_thingy_uis_db.button_handle)
    {
        if (p_ble_evt->evt.gattc_evt.params.hvx.len == 1)
        {
            ble_thingy_uis_c_evt_t ble_thingy_uis_c_evt;

            ble_thingy_uis_c_evt.evt_type                   = BLE_THINGY_UIS_C_EVT_BUTTON_NOTIFICATION;
            ble_thingy_uis_c_evt.conn_handle                = p_ble_thingy_uis_c->conn_handle;
            ble_thingy_uis_c_evt.params.button.button_state = p_ble_evt->evt.gattc_evt.params.hvx.data[0];
            p_ble_thingy_uis_c->evt_handler(p_ble_thingy_uis_c, &ble_thingy_uis_c_evt);
        }
    }
}


/**@brief Function for handling Disconnected event received from the SoftDevice.
 *
 * @details This function check if the disconnect event is happening on the link
 *          associated with the current instance of the module, if so it will set its
 *          conn_handle to invalid.
 *
 * @param[in] p_ble_thingy_uis_c Pointer to the Led Button Client structure.
 * @param[in] p_ble_evt   Pointer to the BLE event received.
 */
static void on_disconnected(ble_thingy_uis_c_t * p_ble_thingy_uis_c, ble_evt_t const * p_ble_evt)
{
    if (p_ble_thingy_uis_c->conn_handle == p_ble_evt->evt.gap_evt.conn_handle)
    {
//vinh
    uart_printf("disconnect in Thingy UIS, handle %d \n\r", p_ble_thingy_uis_c->conn_handle );
        p_ble_thingy_uis_c->conn_handle                    = BLE_CONN_HANDLE_INVALID;
        p_ble_thingy_uis_c->peer_thingy_uis_db.button_cccd_handle = BLE_GATT_HANDLE_INVALID;
        p_ble_thingy_uis_c->peer_thingy_uis_db.button_handle      = BLE_GATT_HANDLE_INVALID;
        p_ble_thingy_uis_c->peer_thingy_uis_db.led_handle         = BLE_GATT_HANDLE_INVALID;
    }
}


void ble_thingy_uis_on_db_disc_evt(ble_thingy_uis_c_t * p_ble_thingy_uis_c, ble_db_discovery_evt_t const * p_evt)
{
     

    // Check if the Led Button Service was discovered.

    //vinh ver2
    if (p_evt->evt_type == BLE_DB_DISCOVERY_COMPLETE &&
        ((p_evt->params.discovered_db.srv_uuid.uuid == THINGY_UIS_UUID_SERVICE)||
         (p_evt->params.discovered_db.srv_uuid.uuid == THINGY_SENSOR_UUID_SERVICE))&&
        p_evt->params.discovered_db.srv_uuid.type == p_ble_thingy_uis_c->uuid_type)
    {
        ble_thingy_uis_c_evt_t evt;
        evt.evt_type    = BLE_THINGY_UIS_C_EVT_DISCOVERY_COMPLETE;
        evt.conn_handle = p_evt->conn_handle;


            for (uint32_t i = 0; i < p_evt->params.discovered_db.char_count; i++)
            {
                const ble_gatt_db_char_t * p_char = &(p_evt->params.discovered_db.charateristics[i]);
                if(p_evt->params.discovered_db.srv_uuid.uuid == THINGY_UIS_UUID_SERVICE)
                {

                    switch (p_char->characteristic.uuid.uuid)
                    {
                        case THINGY_UIS_UUID_LED_CHAR:
                            evt.params.peer_db.led_handle = p_char->characteristic.handle_value;
                            break;
                        case THINGY_UIS_UUID_BUTTON_CHAR:
                            evt.params.peer_db.button_handle      = p_char->characteristic.handle_value;
                            evt.params.peer_db.button_cccd_handle = p_char->cccd_handle;
                            break;
                      NRF_LOG_DEBUG("Led Button Service discovered at peer.");
                    }
                }
            }
        //If the instance has been assigned prior to db_discovery, assign the db_handles
        if (p_ble_thingy_uis_c->conn_handle != BLE_CONN_HANDLE_INVALID)
        {
            if ((p_ble_thingy_uis_c->peer_thingy_uis_db.led_handle         == BLE_GATT_HANDLE_INVALID)&&
                (p_ble_thingy_uis_c->peer_thingy_uis_db.button_handle      == BLE_GATT_HANDLE_INVALID)&&
                (p_ble_thingy_uis_c->peer_thingy_uis_db.button_cccd_handle == BLE_GATT_HANDLE_INVALID))
            {
                p_ble_thingy_uis_c->peer_thingy_uis_db = evt.params.peer_db;
            }
        }

        p_ble_thingy_uis_c->evt_handler(p_ble_thingy_uis_c, &evt);

    }


}

uint32_t ble_thingy_uis_c_init(ble_thingy_uis_c_t * p_ble_thingy_uis_c, ble_thingy_uis_c_init_t * p_ble_thingy_uis_c_init)
{
    uint32_t      err_code;
    ble_uuid_t    thingy_uis_uuid;
    ble_uuid128_t thingy_uis_base_uuid = {THINGY_UIS_UUID_BASE};

    VERIFY_PARAM_NOT_NULL(p_ble_thingy_uis_c);
    VERIFY_PARAM_NOT_NULL(p_ble_thingy_uis_c_init);
    VERIFY_PARAM_NOT_NULL(p_ble_thingy_uis_c_init->evt_handler);

    p_ble_thingy_uis_c->peer_thingy_uis_db.button_cccd_handle = BLE_GATT_HANDLE_INVALID;
    p_ble_thingy_uis_c->peer_thingy_uis_db.button_handle      = BLE_GATT_HANDLE_INVALID;
    p_ble_thingy_uis_c->peer_thingy_uis_db.led_handle         = BLE_GATT_HANDLE_INVALID;
    p_ble_thingy_uis_c->conn_handle                    = BLE_CONN_HANDLE_INVALID;
    p_ble_thingy_uis_c->evt_handler                    = p_ble_thingy_uis_c_init->evt_handler;

    err_code = sd_ble_uuid_vs_add(&thingy_uis_base_uuid, &p_ble_thingy_uis_c->uuid_type);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    VERIFY_SUCCESS(err_code);

    thingy_uis_uuid.type = p_ble_thingy_uis_c->uuid_type;
    thingy_uis_uuid.uuid = THINGY_UIS_UUID_SERVICE;

    return ble_db_discovery_evt_register(&thingy_uis_uuid);
}




void ble_thingy_uis_c_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
    if ((p_context == NULL) || (p_ble_evt == NULL))
    {
        return;
    }

    ble_thingy_uis_c_t * p_ble_thingy_uis_c = (ble_thingy_uis_c_t *)p_context;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GATTC_EVT_HVX:
            on_hvx(p_ble_thingy_uis_c, p_ble_evt);
            break;

        case BLE_GATTC_EVT_WRITE_RSP:
            on_write_rsp(p_ble_thingy_uis_c, p_ble_evt);
            break;

        case BLE_GAP_EVT_DISCONNECTED:

            on_disconnected(p_ble_thingy_uis_c, p_ble_evt);
            break;

        default:
            break;
    }
}


/**@brief Function for configuring the CCCD.
 *
 * @param[in] conn_handle The connection handle on which to configure the CCCD.
 * @param[in] handle_cccd The handle of the CCCD to be configured.
 * @param[in] enable      Whether to enable or disable the CCCD.
 *
 * @return NRF_SUCCESS if the CCCD configure was successfully sent to the peer.
 */
static uint32_t cccd_configure(uint16_t conn_handle, uint16_t handle_cccd, bool enable)
{
    NRF_LOG_DEBUG("Configuring CCCD. CCCD Handle = %d, Connection Handle = %d",
        handle_cccd,conn_handle);

    tx_message_t * p_msg;
    uint16_t       cccd_val = enable ? BLE_GATT_HVX_NOTIFICATION : 0;

    p_msg              = &m_tx_buffer[m_tx_insert_index++];
    m_tx_insert_index &= TX_BUFFER_MASK;

    p_msg->req.write_req.gattc_params.handle   = handle_cccd;
    p_msg->req.write_req.gattc_params.len      = 2;
    p_msg->req.write_req.gattc_params.p_value  = p_msg->req.write_req.gattc_value;
    p_msg->req.write_req.gattc_params.offset   = 0;
    p_msg->req.write_req.gattc_params.write_op = BLE_GATT_OP_WRITE_REQ;
    p_msg->req.write_req.gattc_value[0]        = LSB_16(cccd_val);
    p_msg->req.write_req.gattc_value[1]        = MSB_16(cccd_val);
    p_msg->conn_handle                         = conn_handle;
    p_msg->type                                = WRITE_REQ;

    tx_buffer_process();
    return NRF_SUCCESS;
}


uint32_t ble_thingy_uis_c_button_notif_enable(ble_thingy_uis_c_t * p_ble_thingy_uis_c)
{
    VERIFY_PARAM_NOT_NULL(p_ble_thingy_uis_c);

    if (p_ble_thingy_uis_c->conn_handle == BLE_CONN_HANDLE_INVALID)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    return cccd_configure(p_ble_thingy_uis_c->conn_handle,
                          p_ble_thingy_uis_c->peer_thingy_uis_db.button_cccd_handle,
                          true);
}



uint32_t ble_thingy_uis_led_status_send(ble_thingy_uis_c_t * p_ble_thingy_uis_c, ble_thingy_uis_led_t * led_state, uint16_t length)
{
    VERIFY_PARAM_NOT_NULL(p_ble_thingy_uis_c);

    if (p_ble_thingy_uis_c->conn_handle == BLE_CONN_HANDLE_INVALID)
    {
        return NRF_ERROR_INVALID_STATE;
    }
    
    NRF_LOG_DEBUG("writing Thingy UI LED status: Mode %i, %i, %i, %i", (int)led_state->mode,
        (int)led_state->params.constant.r, (int)led_state->params.constant.g, (int)led_state->params.constant.b);
    tx_message_t * p_msg;

    p_msg              = &m_tx_buffer[m_tx_insert_index++];
    m_tx_insert_index &= TX_BUFFER_MASK;

    p_msg->req.write_req.gattc_params.handle   = p_ble_thingy_uis_c->peer_thingy_uis_db.led_handle;
    p_msg->req.write_req.gattc_params.len      = length;
    p_msg->req.write_req.gattc_params.p_value  = p_msg->req.write_req.gattc_value;
    p_msg->req.write_req.gattc_params.offset   = 0;
    p_msg->req.write_req.gattc_params.write_op = BLE_GATT_OP_WRITE_REQ;
    memcpy(p_msg->req.write_req.gattc_value, (void *)led_state, length);
    p_msg->conn_handle                         = p_ble_thingy_uis_c->conn_handle;
    p_msg->type                                = WRITE_REQ;

    tx_buffer_process();

    return NRF_SUCCESS;
}

uint32_t ble_thingy_uis_led_set_off(ble_thingy_uis_c_t * p_ble_thingy_uis_c)
{
    ble_thingy_uis_led_t led_state;
    led_state.mode = THINGY_UIS_LED_MODE_OFF;
    return ble_thingy_uis_led_status_send(p_ble_thingy_uis_c, &led_state, 1);
}

uint32_t ble_thingy_uis_led_set_constant(ble_thingy_uis_c_t * p_ble_thingy_uis_c, uint8_t r, uint8_t g, uint8_t b)
{
    ble_thingy_uis_led_t led_state;
    led_state.mode = THINGY_UIS_LED_MODE_CONSTANT;
    led_state.params.constant.r = r;
    led_state.params.constant.g = g;
    led_state.params.constant.b = b;
    p_ble_thingy_uis_c->colors[0] = r;
    p_ble_thingy_uis_c->colors[1] = g;
    p_ble_thingy_uis_c->colors[2] = b;
    return ble_thingy_uis_led_status_send(p_ble_thingy_uis_c, &led_state, 4);    
}

uint32_t ble_thingy_uis_led_set_on_off(ble_thingy_uis_c_t * p_ble_thingy_uis_c, bool on)
{
    ble_thingy_uis_led_t led_state;
    led_state.mode = THINGY_UIS_LED_MODE_CONSTANT;
    led_state.params.constant.r = on ? p_ble_thingy_uis_c->colors[0] : 0;
    led_state.params.constant.g = on ? p_ble_thingy_uis_c->colors[1] : 0;
    led_state.params.constant.b = on ? p_ble_thingy_uis_c->colors[2] : 0;    
    return ble_thingy_uis_led_status_send(p_ble_thingy_uis_c, &led_state, 4);    
}

uint32_t ble_thingy_uis_led_set_breathe(ble_thingy_uis_c_t * p_ble_thingy_uis_c, uint8_t color, uint8_t intensity, uint16_t delay)
{
    ble_thingy_uis_led_t led_state;
    led_state.mode = THINGY_UIS_LED_MODE_BREATHE;
    led_state.params.breathe.color = color;
    led_state.params.breathe.intensity = intensity;
    led_state.params.breathe.delay = delay;
    return ble_thingy_uis_led_status_send(p_ble_thingy_uis_c, &led_state, 5);    
}

uint32_t ble_thingy_uis_led_set_one_shot(ble_thingy_uis_c_t * p_ble_thingy_uis_c, uint8_t color, uint8_t intensity)
{
    ble_thingy_uis_led_t led_state;
    led_state.mode = THINGY_UIS_LED_MODE_ONESHOT;
    led_state.params.one_shot.color = color;
    led_state.params.one_shot.intensity = intensity;
    return ble_thingy_uis_led_status_send(p_ble_thingy_uis_c, &led_state, 3);    
}

uint32_t ble_thingy_uis_c_handles_assign(ble_thingy_uis_c_t    * p_ble_thingy_uis_c,
                                  uint16_t         conn_handle,
                                  const thingy_uis_db_t * p_peer_handles)
{
    VERIFY_PARAM_NOT_NULL(p_ble_thingy_uis_c);

    p_ble_thingy_uis_c->conn_handle = conn_handle;
    if (p_peer_handles != NULL)
    {
        p_ble_thingy_uis_c->peer_thingy_uis_db = *p_peer_handles;
    }
    return NRF_SUCCESS;
}
