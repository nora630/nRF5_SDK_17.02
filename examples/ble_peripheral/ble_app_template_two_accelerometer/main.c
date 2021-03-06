/**
 * Copyright (c) 2014 - 2020, Nordic Semiconductor ASA
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
/** @file
 *
 * @defgroup ble_sdk_app_template_main main.c
 * @{
 * @ingroup ble_sdk_app_template
 * @brief Template project main file.
 *
 * This file contains a template for creating a new application. It has the code necessary to wakeup
 * from button, advertise, get a connection restart advertising on disconnect and if no new
 * connection created go back to system-off mode.
 * It can easily be used as a starting point for creating a new application, the comments identified
 * with 'YOUR_JOB' indicates where and how you can customize.
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "ble.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
#include "app_timer.h"
#include "fds.h"
#include "peer_manager.h"
#include "peer_manager_handler.h"
#include "bsp_btn_ble.h"
#include "sensorsim.h"
#include "ble_conn_state.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "nrf_pwr_mgmt.h"

#include "ble_acs.h"

#include "boards.h"
#include "app_util_platform.h"
#include "nrf_drv_ppi.h"
//#include "nrf_drv_clock.h"
#include "nrf_drv_timer.h"
#include "nrf_drv_twi.h"
#include "nrf_delay.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "adpcm.h"


#define DEVICE_NAME                     "Nordic_Template"                       /**< Name of device. Will be included in the advertising data. */
#define MANUFACTURER_NAME               "NordicSemiconductor"                   /**< Manufacturer. Will be passed to Device Information Service. */
#define APP_ADV_INTERVAL                300                                     /**< The advertising interval (in units of 0.625 ms. This value corresponds to 187.5 ms). */

#define APP_ADV_DURATION                18000                                   /**< The advertising duration (180 seconds) in units of 10 milliseconds. */
#define APP_BLE_OBSERVER_PRIO           3                                       /**< Application's BLE observer priority. You shouldn't need to modify this value. */
#define APP_BLE_CONN_CFG_TAG            1                                       /**< A tag identifying the SoftDevice BLE configuration. */

#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(40, UNIT_1_25_MS)        /**< Minimum acceptable connection interval (10 milliseconds). */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(40, UNIT_1_25_MS)        /**< Maximum acceptable connection interval (10 millisecond). */
#define SLAVE_LATENCY                   0                                       /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)         /**< Connection supervisory timeout (4 seconds). */
//#define APP_ADV_TIMEOUT_IN_SECONDS      180                                     /**< The advertising timeout in units of seconds. */
#define ACS_SERVICE_UUID_TYPE           BLE_UUID_TYPE_VENDOR_BEGIN

#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)                   /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000)                  /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                       /**< Number of attempts before giving up the connection parameter negotiation. */

#define SEC_PARAM_BOND                  1                                       /**< Perform bonding. */
#define SEC_PARAM_MITM                  0                                       /**< Man In The Middle protection not required. */
#define SEC_PARAM_LESC                  0                                       /**< LE Secure Connections not enabled. */
#define SEC_PARAM_KEYPRESS              0                                       /**< Keypress notifications not enabled. */
#define SEC_PARAM_IO_CAPABILITIES       BLE_GAP_IO_CAPS_NONE                    /**< No I/O capabilities. */
#define SEC_PARAM_OOB                   0                                       /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE          7                                       /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE          16                                      /**< Maximum encryption key size. */

#define DEAD_BEEF                       0xDEADBEEF                              /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#define ACCEL_SEND_INTERVAL             APP_TIMER_TICKS(1)                   // ble accel send interval

NRF_BLE_GATT_DEF(m_gatt);                                                       /**< GATT module instance. */
NRF_BLE_QWR_DEF(m_qwr);                                                         /**< Context for the Queued Write module.*/
BLE_ADVERTISING_DEF(m_advertising);                                             /**< Advertising module instance. */

BLE_ACS_DEF(m_acs);                                                             // Accelerometer Service instance
APP_TIMER_DEF(m_accel_timer_id);                                                // Accelerometer timer

static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;                        /**< Handle of the current connection. */

static uint8_t aData[20];
static int32_t aData1[40];
static int32_t aData2[40];
static uint16_t alen = 40;
static uint16_t asendlen = 20;


//*****************************************************//
// twi communication
//*****************************************************//
/* SCL SDA PIN */
#define SCL1_PIN    9
#define SDA1_PIN    2
#define SCL2_PIN    29
#define SDA2_PIN    10


/* TWI instance ID. */
#define TWI_INSTANCE_ID1     0
#define TWI_INSTANCE_ID2     1

/* Common address for LIS2DH */
#define LIS2DH_ADDR      0x18U

#define LIS2DH_CTRL_REG   0xA0U
#define LIS2DH_DATA_REG   0xA8U
#define WHO_AM_I          0x0FU
#define LIS2DH_CTRL_REG1  0x20U
#define LIS2DH_CTRL_REG2  0x21U
#define LIS2DH_CTRL_REG4  0x23U
#define LIS2DH_CTRL_REG5  0x24U
#define FIFO_CTRL_REG     0x2EU

/* range */
#define LIS2DH_RANGE_2GA    0x00U
#define LIS2DH_RANGE_4GA    0x10U

/* mode */
#define LOW_POWER_MODE    0x2FU // Low Power and ODR = 10Hz
#define LOW_POWER_MODE1   0x7FU // Low Power and ODR = 400Hz
#define LOW_POWER_MODE2   0x8FU // Low Power and ODR = 1620Hz
#define HR_NORMAL_MODE    0x97U // HR normal and ODR = 1620Hz
#define HR_NORMAL_MODE2   0x77U // HR normal and ODR = 400Hz
#define HR_NORMAL_MODE3   0x27U // HR normal and ODR = 10Hz

#define HR_MODE           0x38U

#define HIGH_PASS_MODE    0xB8U

#define FIFO_ENABLE       0x40U
#define STREAM_MODE       0x80U

/* PPI */
#define PPI_TIMER1_INTERVAL   (1) // Timer interval in milliseconds, this is twi sampling rate. 

/* buffer size */
#define TWIM_RX_BUF_WIDTH    6
#define TWIM_RX_BUF_LENGTH  40

static struct ADPCMstate state;

/* Indicates if operation on TWI has ended. */
static volatile bool m_xfer_done1 = false;
static volatile bool m_xfer_done2 = false;

static volatile bool m_send_done1 = false;
static volatile bool m_send_done2 = false;

/* TWI instance. */
static const nrf_drv_twi_t m_twi1 = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID1);
static const nrf_drv_twi_t m_twi2 = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID2);


/* Buffer for samples read from accelerometer */
typedef struct ArrayList
{
    uint8_t buffer[TWIM_RX_BUF_WIDTH];
} array_list_t;

static array_list_t p_rx_buffer1[TWIM_RX_BUF_LENGTH];
static array_list_t p_rx_buffer2[TWIM_RX_BUF_LENGTH];
//static int16_t x;
//static int16_t y;
//static int16_t z;
//static int32_t sum;

static uint8_t m_sensorData[6];
static uint8_t m_dataReg[1] = {LIS2DH_DATA_REG};

/* ppi setting */
static const nrf_drv_timer_t m_timer1 = NRF_DRV_TIMER_INSTANCE(1);
static const nrf_drv_timer_t m_timer2 = NRF_DRV_TIMER_INSTANCE(2);
static const nrf_drv_timer_t m_timer3 = NRF_DRV_TIMER_INSTANCE(3);
static const nrf_drv_timer_t m_timer4 = NRF_DRV_TIMER_INSTANCE(4);


static nrf_ppi_channel_t     m_ppi_channel1;
static nrf_ppi_channel_t     m_ppi_channel2;
static nrf_ppi_channel_t     m_ppi_channel3;
static nrf_ppi_channel_t     m_ppi_channel4;


/* etc.. */
static uint32_t              m_evt_counter;

/* filter setting */
static  float in1, in2, out1, out2;
static  float a0, a1, a2, b0, b1, b2;

void high_filter_set(void)
{
    in1 = 0;
    in2 = 0;
    out1 = 0;
    out2 = 0;
    //omega = 2.0f * 3.14159265f * freq / samplerate;
    //alpha = sin(omega) / (2.0f * q);

    // cut f = 0.001 * 500 Hz
    a0 =   1;
    a1 =   -0.9969;
    //a2 =   1.0f - alpha;
    b0 =  0.9984;
    b1 = -0.9984;
    //b2 =  (1.0f + cos(omega)) / 2.0f;

  
    /*
    // cut f = 0.01 * 500 Hz
    a0 =   1;
    a1 =   -0.9691;
    //a2 =   1.0f - alpha;
    b0 =  0.9845;
    b1 = -0.9845;
    //b2 =  (1.0f + cos(omega)) / 2.0f;
    */

    /*
    // cut f = 0.005 * 500 Hz
    a0 = 1;
    a1 = -0.9844;
    b0 = 0.9922;
    b1 = -0.9922;
    */

    /*
    // cut f = 0.1 * 500 Hz
    a0 = 1;
    a1 = -0.7265;
    b0 = 0.8633;
    b1 = -0.8633;
    */
}

int32_t filter1(int32_t input)
{
    float output;
    output = b0/a0 * (float)input + b1/a0 * in1 - a1/a0 * out1;
    in1 = input;
    out1 = output;
  
    return (int32_t)output;
}

int32_t filter2(int32_t input)
{
    float output;
    output = b0/a0 * (float)input + b1/a0 * in2 - a1/a0 * out2;
    in2 = input;
    out2 = output;
  
    return (int32_t)output;
}



// sqrt function
int32_t isqrt(int32_t num) {
    //assert(("sqrt input should be non-negative", num > 0));
    int32_t res = 0;
    int32_t bit = 1 << 30; // The second-to-top bit is set.
                           // Same as ((unsigned) INT32_MAX + 1) / 2.

    // "bit" starts at the highest power of four <= the argument.
    while (bit > num)
        bit >>= 2;

    while (bit != 0) {
        if (num >= res + bit) {
            num -= res + bit;
            res = (res >> 1) + bit;
        } else
            res >>= 1;
        bit >>= 2;
    }
    return res;
}


/* Function for setting LIS2DH accelerometer */
void LIS2DH_set_mode(void)
{
    ret_code_t err_code;
    
    /* Writing to LIS2DH_CTR_REG set range and Low Power mode(ODR=10Hz) */
    //uint8_t reg[7] = {LIS2DH_CTRL_REG, LOW_POWER_MODE1, 0x00, 0x00, LIS2DH_RANGE_2GA, 0x00, 0x00};
    //uint8_t reg[2] = {LIS2DH_CTRL_REG1, LOW_POWER_MODE1};
    uint8_t reg[2] = {LIS2DH_CTRL_REG1, HR_NORMAL_MODE};
    uint8_t reg2[2] = {LIS2DH_CTRL_REG4, HR_MODE};
    uint8_t reg3[2] = {LIS2DH_CTRL_REG2, HIGH_PASS_MODE};
    uint8_t reg4[2] = {LIS2DH_CTRL_REG5, FIFO_ENABLE};
    uint8_t reg5[2] = {FIFO_CTRL_REG, STREAM_MODE};
    m_xfer_done1 = false;
    err_code = nrf_drv_twi_tx(&m_twi1, LIS2DH_ADDR, reg, sizeof(reg), false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done1 == false);
    m_xfer_done1 = false;
    
/*
    err_code = nrf_drv_twi_tx(&m_twi1, LIS2DH_ADDR, reg3, sizeof(reg3), false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done1 == false);
    m_xfer_done1 = false;
  */  

    err_code = nrf_drv_twi_tx(&m_twi1, LIS2DH_ADDR, reg2, sizeof(reg2), false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done1 == false);
    m_xfer_done1 = false;
    /*
    //err_code = nrf_drv_twi_tx(&m_twi1, LIS2DH_ADDR, reg3, sizeof(reg3), false);
    //APP_ERROR_CHECK(err_code);
    //while (m_xfer_done1 == false);
    //m_xfer_done1 = false;
    err_code = nrf_drv_twi_tx(&m_twi1, LIS2DH_ADDR, reg4, sizeof(reg4), false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done1 == false);
    m_xfer_done1 = false;
    err_code = nrf_drv_twi_tx(&m_twi1, LIS2DH_ADDR, reg5, sizeof(reg5), false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done1 == false);
    m_xfer_done1 = false;
    */
    

    m_xfer_done2 = false;
    err_code = nrf_drv_twi_tx(&m_twi2, LIS2DH_ADDR, reg, sizeof(reg), false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done2 == false);
    m_xfer_done2 = false;

    /*
    err_code = nrf_drv_twi_tx(&m_twi2, LIS2DH_ADDR, reg3, sizeof(reg3), false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done2 == false);
    m_xfer_done2 = false;
*/

    err_code = nrf_drv_twi_tx(&m_twi2, LIS2DH_ADDR, reg2, sizeof(reg2), false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done2 == false);
    m_xfer_done2 = false;
    /*
    //err_code = nrf_drv_twi_tx(&m_twi2, LIS2DH_ADDR, reg3, sizeof(reg3), false);
    //APP_ERROR_CHECK(err_code);
    //while (m_xfer_done2 == false);
    //m_xfer_done2 = false;
    err_code = nrf_drv_twi_tx(&m_twi2, LIS2DH_ADDR, reg4, sizeof(reg4), false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done2 == false);
    m_xfer_done2 = false;
    err_code = nrf_drv_twi_tx(&m_twi2, LIS2DH_ADDR, reg5, sizeof(reg5), false);
    APP_ERROR_CHECK(err_code);
    while (m_xfer_done2 == false);
    m_xfer_done2 = false;
    */
}

/**
 * @brief TWI events handler.
 */
void twi_handler1(nrf_drv_twi_evt_t const * p_event, void * p_context)
{
    switch (p_event->type)
    {
        case NRF_DRV_TWI_EVT_DONE:
            m_xfer_done1 = true;
            break;
        default:
            break;
    }
}

void twi_handler2(nrf_drv_twi_evt_t const * p_event, void * p_context)
{
    switch (p_event->type)
    {
        case NRF_DRV_TWI_EVT_DONE:
            m_xfer_done2 = true;
            break;
        default:
            break;
    }
}

/**
 * @brief UART initialization.
 */
void twi_init (void)
{
    ret_code_t err_code;

    const nrf_drv_twi_config_t twi_lis2dh_config1 = {
       .scl                = SCL1_PIN,
       .sda                = SDA1_PIN,
       .frequency          = NRF_TWI_FREQ_400K,
       .interrupt_priority = APP_IRQ_PRIORITY_LOW,
       .clear_bus_init     = true
    };

    err_code = nrf_drv_twi_init(&m_twi1, &twi_lis2dh_config1, twi_handler1, NULL);
    APP_ERROR_CHECK(err_code);

    const nrf_drv_twi_config_t twi_lis2dh_config2 = {
       .scl                = SCL2_PIN,
       .sda                = SDA2_PIN,
       .frequency          = NRF_TWI_FREQ_400K,
       .interrupt_priority = APP_IRQ_PRIORITY_LOW,
       .clear_bus_init     = true
    };

    err_code = nrf_drv_twi_init(&m_twi2, &twi_lis2dh_config2, twi_handler2, NULL);
    APP_ERROR_CHECK(err_code);

    nrf_drv_twi_enable(&m_twi1);
    nrf_drv_twi_enable(&m_twi2);
}

/*

*/
void twi_check_connection(void)
{
    ret_code_t err_code;
    uint8_t samp;

    err_code = nrf_drv_twi_rx(&m_twi1, LIS2DH_ADDR, &samp, sizeof(samp));
    APP_ERROR_CHECK(err_code);
    if (err_code == NRF_SUCCESS)
    {
        NRF_LOG_INFO("TWI device detected at address 0x%x.", LIS2DH_ADDR);
    }
    while (m_xfer_done1 == false);
    //NRF_LOG_FLUSH();
}

/* empty function */
static void timer1_handler(nrf_timer_event_t event_type, void * p_context)
{

}


/* TWIM counter handler */
static void timer2_handler(nrf_timer_event_t event_type, void * p_context)
{
    m_xfer_done1 = true;

    int16_t x, y, z;
    int32_t sum;

    for(int16_t i=0; i<alen; i++)
    {
        //x = ((int8_t)p_rx_buffer1[i].buffer[1] << 4) & 0x0ff0;//+ ((int8_t)p_rx_buffer1[i].buffer[0]); // change 0 to i
        //x |= ((int8_t)p_rx_buffer1[i].buffer[0] >> 4) & 0x0f;
        //y = ((int8_t)p_rx_buffer1[i].buffer[3] << 4) & 0x0ff0;//+ ((int8_t)p_rx_buffer1[i].buffer[2]);
        //y |= ((int8_t)p_rx_buffer1[i].buffer[2] >> 4) & 0x0f;
        //z = ((int8_t)p_rx_buffer1[i].buffer[5] << 4) & 0x0ff0;//+ ((int8_t)p_rx_buffer1[i].buffer[4]);
        //z |= ((int8_t)p_rx_buffer1[i].buffer[4] >> 4) & 0x0f;
        //x = (int16_t)p_rx_buffer1[i].buffer[1] << 4;
        //y = (int16_t)p_rx_buffer1[i].buffer[3] << 4;
        //z = (int16_t)p_rx_buffer1[i].buffer[5] << 4;
        //sum = x * x + y * y + z * z;
        x = ((int8_t)p_rx_buffer1[i].buffer[1] << 8) & 0xff00;//+ ((int8_t)p_rx_buffer1[i].buffer[0]); // change 0 to i
        x |= ((uint8_t)p_rx_buffer1[i].buffer[0]) & 0xf0;
        y = ((int8_t)p_rx_buffer1[i].buffer[3] << 8) & 0xff00;//+ ((int8_t)p_rx_buffer1[i].buffer[2]);
        y |= ((uint8_t)p_rx_buffer1[i].buffer[2]) & 0xf0;
        z = ((int8_t)p_rx_buffer1[i].buffer[5] << 8) & 0xff00;//+ ((int8_t)p_rx_buffer1[i].buffer[4]);
        z |= ((uint8_t)p_rx_buffer1[i].buffer[4]) & 0xf0;
        sum = x + y + z;
        //sum = isqrt(sum);
        //sum *= 3;
        aData1[i] = sum;//(int16_t)sum;
        //aData[i] = sum & 0x0f;
        //printf("%d\n", aData1[i]);
        //aData[2*i+1] = (sum >> 8) & 0x0f;
    }
    //printf("111  %d\n", sum);

    //accel_data_send();
    m_send_done1 = true;

    nrf_drv_twi_xfer_desc_t xfer = NRF_DRV_TWI_XFER_DESC_TXRX(LIS2DH_ADDR, m_dataReg, 
                                      sizeof(m_dataReg), (uint8_t*)p_rx_buffer1, sizeof(p_rx_buffer1) / TWIM_RX_BUF_LENGTH);

    uint32_t flags = NRF_DRV_TWI_FLAG_HOLD_XFER             |
                     NRF_DRV_TWI_FLAG_RX_POSTINC            |
                     NRF_DRV_TWI_FLAG_NO_XFER_EVT_HANDLER   |
                     NRF_DRV_TWI_FLAG_REPEATED_XFER;

    ret_code_t err_code = nrf_drv_twi_xfer(&m_twi1, &xfer, flags);
    APP_ERROR_CHECK(err_code);

}

/* TWIM counter handler */
static void timer3_handler(nrf_timer_event_t event_type, void * p_context)
{
    m_xfer_done2 = true;
    int16_t x, y, z;
    int32_t sum;

    for(int16_t i=0; i<alen; i++)
    {
        //x = (int16_t)p_rx_buffer2[i].buffer[1] << 4; // change 0 to i
        //y = (int16_t)p_rx_buffer2[i].buffer[3] << 4;
        //z = (int16_t)p_rx_buffer2[i].buffer[5] << 4;
        
        //x = ((int8_t)p_rx_buffer2[i].buffer[1] << 4) & 0x0ff0;//+ ((int8_t)p_rx_buffer2[i].buffer[0] >> 4); // change 0 to i
        //x |= ((int8_t)p_rx_buffer2[i].buffer[0] >> 4) & 0x0f;
        //y = ((int8_t)p_rx_buffer2[i].buffer[3] << 4) & 0x0ff0;//+ ((int8_t)p_rx_buffer2[i].buffer[2] >> 4);
        //y |= ((int8_t)p_rx_buffer2[i].buffer[2] >> 4) & 0x0f;
        //z = ((int8_t)p_rx_buffer2[i].buffer[5] << 4) & 0x0ff0;//+ ((int8_t)p_rx_buffer2[i].buffer[4] >> 4);
        //z |= ((int8_t)p_rx_buffer2[i].buffer[4] >> 4) & 0x0f;
        x = ((int8_t)p_rx_buffer2[i].buffer[1] << 8) & 0xff00;//+ ((int8_t)p_rx_buffer1[i].buffer[0]); // change 0 to i
        x |= ((uint8_t)p_rx_buffer2[i].buffer[0]) & 0xf0;
        y = ((int8_t)p_rx_buffer2[i].buffer[3] << 8) & 0xff00;//+ ((int8_t)p_rx_buffer1[i].buffer[2]);
        y |= ((uint8_t)p_rx_buffer2[i].buffer[2]) & 0xf0;
        z = ((int8_t)p_rx_buffer2[i].buffer[5] << 8) & 0xff00;//+ ((int8_t)p_rx_buffer1[i].buffer[4]);
        z |= ((uint8_t)p_rx_buffer2[i].buffer[4]) & 0xf0;
        //sum = x * x + y * y + z * z;
        //sum = isqrt(sum);
        sum = x + y + z;
        aData2[i] = sum;//(int16_t)sum;
        //aData[i] = sum & 0x0f;
        //printf("%d\n", aData[i]);
        //aData[2*i+1] = (sum >> 8) & 0x0f;
    }
    m_send_done2 = true;
    //accel_data_send();
    //printf("222  %d\n", aData2[0]);


    nrf_drv_twi_xfer_desc_t xfer = NRF_DRV_TWI_XFER_DESC_TXRX(LIS2DH_ADDR, m_dataReg, 
                                      sizeof(m_dataReg), (uint8_t*)p_rx_buffer2, sizeof(p_rx_buffer2) / TWIM_RX_BUF_LENGTH);

    uint32_t flags = NRF_DRV_TWI_FLAG_HOLD_XFER             |
                     NRF_DRV_TWI_FLAG_RX_POSTINC            |
                     NRF_DRV_TWI_FLAG_NO_XFER_EVT_HANDLER   |
                     NRF_DRV_TWI_FLAG_REPEATED_XFER;

    ret_code_t err_code = nrf_drv_twi_xfer(&m_twi2, &xfer, flags);
    APP_ERROR_CHECK(err_code);

}

static void timer4_handler(nrf_timer_event_t event_type, void * p_context)
{
    ret_code_t err_code;
    //uint8_t aData[2] = {0x4d, 0x3a};
    //uint16_t alen = 2;
    //for (int i=0; i<4; i++) aData[i] |= 0x11 ;
    //uint8_t aData[20];
    uint16_t s;
    if(!(m_send_done1&&m_send_done2)) 
    {
        //printf("no\n");
        //count++;
        return;
    }
    m_send_done1 = false;
    m_send_done2 = false;
    //printf("%d\n", count);
    adpcm_encoder();
    /*
    for(uint16_t i=0; i<alen; i++){
        s = aData1[i] + aData2[i];
        //printf("%d\n", s);
        aData[2*i] = (s >> 8) & 0xff;
        aData[2*i+1] = s & 0xff;
    } */

    //if(++count>120) count = 0;
    //aData[0] = count;

    err_code = ble_acs_accel_data_send(&m_acs, &aData, &asendlen);
    if((err_code != NRF_SUCCESS) &&
       (err_code != NRF_ERROR_INVALID_STATE) &&
       (err_code != NRF_ERROR_RESOURCES) &&
       (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
      )
    {
        APP_ERROR_HANDLER(err_code);
    }
}



// Function for Timer 1 initialization
static void timer1_init(void)
{
    nrf_drv_timer_config_t timer1_config = NRF_DRV_TIMER_DEFAULT_CONFIG;
    timer1_config.frequency = NRF_TIMER_FREQ_16MHz;
    ret_code_t err_code = nrf_drv_timer_init(&m_timer1, &timer1_config, timer1_handler);
    APP_ERROR_CHECK(err_code);

    nrf_drv_timer_extended_compare(&m_timer1,
                                   NRF_TIMER_CC_CHANNEL0,
                                   nrf_drv_timer_ms_to_ticks(&m_timer1,
                                                             PPI_TIMER1_INTERVAL),
                                   NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK,
                                   false);

}

// Function for Timer 2 initialization
static void timer2_init(void)
{
    nrf_drv_timer_config_t timer2_config = NRF_DRV_TIMER_DEFAULT_CONFIG;
    timer2_config.mode = NRF_TIMER_MODE_LOW_POWER_COUNTER;
    ret_code_t err_code = nrf_drv_timer_init(&m_timer2, &timer2_config, timer2_handler);
    APP_ERROR_CHECK(err_code);

    nrf_drv_timer_extended_compare(&m_timer2,
                                    NRF_TIMER_CC_CHANNEL0,
                                    TWIM_RX_BUF_LENGTH,
                                    NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK,
                                    true);
}

static void timer3_init(void)
{
    nrf_drv_timer_config_t timer3_config = NRF_DRV_TIMER_DEFAULT_CONFIG;
    timer3_config.mode = NRF_TIMER_MODE_LOW_POWER_COUNTER;
    ret_code_t err_code = nrf_drv_timer_init(&m_timer3, &timer3_config, timer3_handler);
    APP_ERROR_CHECK(err_code);

    nrf_drv_timer_extended_compare(&m_timer3,
                                    NRF_TIMER_CC_CHANNEL0,
                                    TWIM_RX_BUF_LENGTH,
                                    NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK,
                                    true);
}

static void timer4_init(void)
{
    nrf_drv_timer_config_t timer4_config = NRF_DRV_TIMER_DEFAULT_CONFIG;
    timer4_config.frequency = NRF_TIMER_FREQ_16MHz;
    ret_code_t err_code = nrf_drv_timer_init(&m_timer4, &timer4_config, timer4_handler);
    APP_ERROR_CHECK(err_code);

    nrf_drv_timer_extended_compare(&m_timer4,
                                   NRF_TIMER_CC_CHANNEL0,
                                   nrf_drv_timer_ms_to_ticks(&m_timer4,
                                                             PPI_TIMER1_INTERVAL),
                                   NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK,
                                   true);

}

/* initialize ppi channel */
static void twi_accel_ppi_init(void)
{
    ret_code_t err_code;
    
    err_code = nrf_drv_ppi_init();
    APP_ERROR_CHECK(err_code);
    
    // setup timer1
    timer1_init();

    //setup timer2
    timer2_init();

    //setup timer3
    timer3_init();

    nrf_drv_twi_xfer_desc_t xfer1 = NRF_DRV_TWI_XFER_DESC_TXRX(LIS2DH_ADDR, m_dataReg, 
                                      sizeof(m_dataReg), (uint8_t*)p_rx_buffer1, sizeof(p_rx_buffer1)  / TWIM_RX_BUF_LENGTH);

    uint32_t flags = NRF_DRV_TWI_FLAG_HOLD_XFER             |
                     NRF_DRV_TWI_FLAG_RX_POSTINC            |
                     NRF_DRV_TWI_FLAG_NO_XFER_EVT_HANDLER   |
                     NRF_DRV_TWI_FLAG_REPEATED_XFER;

    err_code = nrf_drv_twi_xfer(&m_twi1, &xfer1, flags);
    
    // TWIM is now configured and ready to be started.
    if (err_code == NRF_SUCCESS)
    {   
        // set up PPI to trigger the transfer
        uint32_t twi_start_task_addr1 = nrf_drv_twi_start_task_get(&m_twi1, xfer1.type);
        err_code = nrf_drv_ppi_channel_alloc(&m_ppi_channel1);
        APP_ERROR_CHECK(err_code);
        err_code = nrf_drv_ppi_channel_assign(m_ppi_channel1,
                                              nrf_drv_timer_event_address_get(&m_timer1,
                                                                              NRF_TIMER_EVENT_COMPARE0),
                                              twi_start_task_addr1);
        APP_ERROR_CHECK(err_code);
        
        // set up PPI to count the number of transfers 
        uint32_t twi_stopped_event_addr1 = nrf_drv_twi_stopped_event_get(&m_twi1);
        err_code = nrf_drv_ppi_channel_alloc(&m_ppi_channel2);
        APP_ERROR_CHECK(err_code);
        err_code = nrf_drv_ppi_channel_assign(m_ppi_channel2,
                                              twi_stopped_event_addr1,
                                              nrf_drv_timer_task_address_get(&m_timer2,
                                                                             NRF_TIMER_TASK_COUNT));
    }


    nrf_drv_twi_xfer_desc_t xfer2 = NRF_DRV_TWI_XFER_DESC_TXRX(LIS2DH_ADDR, m_dataReg, 
                                      sizeof(m_dataReg), (uint8_t*)p_rx_buffer2, sizeof(p_rx_buffer2)  / TWIM_RX_BUF_LENGTH);


    err_code = nrf_drv_twi_xfer(&m_twi2, &xfer2, flags);
    
    // TWIM is now configured and ready to be started.
    if (err_code == NRF_SUCCESS)
    {   
        // set up PPI to trigger the transfer
        uint32_t twi_start_task_addr2 = nrf_drv_twi_start_task_get(&m_twi2, xfer2.type);
        err_code = nrf_drv_ppi_channel_alloc(&m_ppi_channel3);
        APP_ERROR_CHECK(err_code);
        err_code = nrf_drv_ppi_channel_assign(m_ppi_channel3,
                                              nrf_drv_timer_event_address_get(&m_timer1,
                                                                              NRF_TIMER_EVENT_COMPARE0),
                                              twi_start_task_addr2);
        APP_ERROR_CHECK(err_code);
        
        // set up PPI to count the number of transfers 
        uint32_t twi_stopped_event_addr2 = nrf_drv_twi_stopped_event_get(&m_twi2);
        err_code = nrf_drv_ppi_channel_alloc(&m_ppi_channel4);
        APP_ERROR_CHECK(err_code);
        err_code = nrf_drv_ppi_channel_assign(m_ppi_channel4,
                                              twi_stopped_event_addr2,
                                              nrf_drv_timer_task_address_get(&m_timer3,
                                                                             NRF_TIMER_TASK_COUNT));
    }
}


static void twi_accel_ppi_enable(void)
{
    ret_code_t err_code;
    err_code = nrf_drv_ppi_channel_enable(m_ppi_channel1);
    APP_ERROR_CHECK(err_code);
    err_code = nrf_drv_ppi_channel_enable(m_ppi_channel2);
    APP_ERROR_CHECK(err_code);
    err_code = nrf_drv_ppi_channel_enable(m_ppi_channel3);
    APP_ERROR_CHECK(err_code);
    err_code = nrf_drv_ppi_channel_enable(m_ppi_channel4);
    APP_ERROR_CHECK(err_code);
}

void twi_start(void)
{
    // enable the counter counting
    nrf_drv_timer_enable(&m_timer2);
    nrf_drv_timer_enable(&m_timer3);
    
    //m_xfer_done1 = false;
    // enable timer triggering TWI transfer
    nrf_drv_timer_enable(&m_timer1);
}

void twi_stop(void)
{
    // enable the counter counting
    nrf_drv_timer_disable(&m_timer2);
    nrf_drv_timer_disable(&m_timer3);
    
    //m_xfer_done1 = false;
    // enable timer triggering TWI transfer
    nrf_drv_timer_disable(&m_timer1);
}

/*******************************************************************************************************/

static ble_uuid_t m_adv_uuids[] =                                               /**< Universally unique service identifiers. */
{
    //{BLE_UUID_DEVICE_INFORMATION_SERVICE, BLE_UUID_TYPE_BLE}
    {ACS_UUID_SERVICE, ACS_SERVICE_UUID_TYPE}
};



static void advertising_start(bool erase_bonds);


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


/**@brief Function for handling Peer Manager events.
 *
 * @param[in] p_evt  Peer Manager event.
 */
static void pm_evt_handler(pm_evt_t const * p_evt)
{
    pm_handler_on_pm_evt(p_evt);
    pm_handler_flash_clean(p_evt);

    switch (p_evt->evt_id)
    {
        case PM_EVT_PEERS_DELETE_SUCCEEDED:
            advertising_start(false);
            break;

        default:
            break;
    }
}

void adpcm_state_init(void)
{
    state.prevsample = 0;
    state.previndex = 0;
}

void adpcm_encoder(void)
{
    int32_t s;
    for(int16_t i=0; i<alen; i++)
    {
        //s = (int16_t)filter1(aData1[i]) + (int16_t)filter2(aData2[i]);
        //s = (int16_t)(aData1[i]) + (int16_t)(aData2[i]);
        s = aData1[i] + aData2[i];
        //s = filter1(s);
        //if(i==0) printf("%d\n", s);
        //s = (int16_t)filter(s);
        //printf("%d\n", s);
        if(i%2){
            aData[i/2] |= ADPCMEncoder(s, &state);
        } else {
            aData[i/2] = ADPCMEncoder(s, &state);
            aData[i/2] = (aData[i/2] << 4) & 0xf0;
        }
    }
    //printf("%d\n", s);
}

//static uint8_t count = 0;

/* Function for performing accel measurement and updating the tx accel characteristic in Accelerometer Service */
void accel_data_send(void)
{
    ret_code_t err_code;
    //uint8_t aData[2] = {0x4d, 0x3a};
    //uint16_t alen = 2;
    //for (int i=0; i<4; i++) aData[i] |= 0x11 ;
    //uint8_t aData[20];
    uint16_t s;
    if(!(m_send_done1&&m_send_done2)) 
    {
        //printf("no\n");
        //count++;
        return;
    }
    m_send_done1 = false;
    m_send_done2 = false;
    //printf("%d\n", count);
    adpcm_encoder();
    /*
    for(uint16_t i=0; i<alen; i++){
        s = aData1[i] + aData2[i];
        //printf("%d\n", s);
        aData[2*i] = (s >> 8) & 0xff;
        aData[2*i+1] = s & 0xff;
    } */

    //if(++count>120) count = 0;
    //aData[0] = count;

    err_code = ble_acs_accel_data_send(&m_acs, &aData, &asendlen);
    if((err_code != NRF_SUCCESS) &&
       (err_code != NRF_ERROR_INVALID_STATE) &&
       (err_code != NRF_ERROR_RESOURCES) &&
       (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
      )
    {
        APP_ERROR_HANDLER(err_code);
    }
}

/* Function for handling the Accelerometer measurement timer timeout  */
static void accel_meas_timeout_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    accel_data_send();

}

/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 */
static void timers_init(void)
{
    // Initialize timer module.
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);


    timer4_init();

    /*
    // Create timers.
     err_code = app_timer_create(&m_accel_timer_id,
                                 APP_TIMER_MODE_REPEATED,
                                 accel_meas_timeout_handler);
     APP_ERROR_CHECK(err_code);
     */
}


/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(void)
{
    ret_code_t              err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    char nameID[20];
    char name[] = "Sensor:";
    ble_gap_addr_t device_addr;
    err_code = sd_ble_gap_addr_get(&device_addr);
    VERIFY_SUCCESS(err_code);

    sprintf(nameID, "%s%x", name, device_addr.addr[0]);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)nameID,
                                          strlen(nameID));
    APP_ERROR_CHECK(err_code);

    err_code = sd_ble_gap_appearance_set(0);
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the GATT module.
 */
static void gatt_init(void)
{
    ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, NULL);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling Queued Write Module errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for handling the YYY Service events.
 * YOUR_JOB implement a service handler function depending on the event the service you are using can generate
 *
 * @details This function will be called for all YY Service events which are passed to
 *          the application.
 *
 * @param[in]   p_yy_service   YY Service structure.
 * @param[in]   p_evt          Event received from the YY Service.
 *
 *
static void on_yys_evt(ble_yy_service_t     * p_yy_service,
                       ble_yy_service_evt_t * p_evt)
{
    switch (p_evt->evt_type)
    {
        case BLE_YY_NAME_EVT_WRITE:
            APPL_LOG("[APPL]: charact written with value %s. ", p_evt->params.char_xx.value.p_str);
            break;

        default:
            // No implementation needed.
            break;
    }
}
*/

/**@brief Function for initializing services that will be used by the application.
 */
static void services_init(void)
{
    ret_code_t         err_code;
    nrf_ble_qwr_init_t qwr_init = {0};

    // Initialize Queued Write Module.
    qwr_init.error_handler = nrf_qwr_error_handler;

    err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init);
    APP_ERROR_CHECK(err_code);

    ble_acs_init_t acs_init;
    
    // Initialize Accelerometer Service
    memset(&acs_init, 0, sizeof(acs_init));

    acs_init.accel_write_handler  = NULL;
    acs_init.notification_enabled = true;

    err_code = ble_acs_init(&m_acs, &acs_init);
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
    ret_code_t err_code;

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
    ret_code_t             err_code;
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


/**@brief Function for starting timers.
 */
static void application_timers_start(void)
{
    ret_code_t err_code;
    err_code = app_timer_start(m_accel_timer_id, ACCEL_SEND_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);
}

static void application_timers_stop(void)
{
    ret_code_t err_code;
    err_code = app_timer_stop(m_accel_timer_id);
    APP_ERROR_CHECK(err_code);
    
}


/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
static void sleep_mode_enter(void)
{
    ret_code_t err_code;

    err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);

    // Prepare wakeup buttons.
    err_code = bsp_btn_ble_sleep_mode_prepare();
    APP_ERROR_CHECK(err_code);

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
    ret_code_t err_code;

    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            NRF_LOG_INFO("Fast advertising.");
            err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_ADV_EVT_IDLE:
            sleep_mode_enter();
            break;

        default:
            break;
    }
}


/**@brief Function for handling BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    ret_code_t err_code = NRF_SUCCESS;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_DISCONNECTED:
            NRF_LOG_INFO("Disconnected.");
            // LED indication will be changed when advertising starts.
            twi_stop();
            nrf_drv_timer_disable(&m_timer4);
            //application_timers_stop();
            //NRF_LOG_INFO("Disconnected.");
            break;

        case BLE_GAP_EVT_CONNECTED:
            NRF_LOG_INFO("Connected.");
            err_code = bsp_indication_set(BSP_INDICATE_CONNECTED);
            APP_ERROR_CHECK(err_code);
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
            APP_ERROR_CHECK(err_code);
            m_send_done1 = false;
            m_send_done2 = false;
            adpcm_state_init();
            twi_start();
            //application_timers_start();
            nrf_drv_timer_enable(&m_timer4);
            break;

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

        case BLE_GATTC_EVT_TIMEOUT:
            // Disconnect on GATT Client timeout event.
            NRF_LOG_DEBUG("GATT Client Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_TIMEOUT:
            // Disconnect on GATT Server timeout event.
            NRF_LOG_DEBUG("GATT Server Timeout.");
            err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
            break;

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
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


/**@brief Function for the Peer Manager initialization.
 */
static void peer_manager_init(void)
{
    ble_gap_sec_params_t sec_param;
    ret_code_t           err_code;

    err_code = pm_init();
    APP_ERROR_CHECK(err_code);

    memset(&sec_param, 0, sizeof(ble_gap_sec_params_t));

    // Security parameters to be used for all security procedures.
    sec_param.bond           = SEC_PARAM_BOND;
    sec_param.mitm           = SEC_PARAM_MITM;
    sec_param.lesc           = SEC_PARAM_LESC;
    sec_param.keypress       = SEC_PARAM_KEYPRESS;
    sec_param.io_caps        = SEC_PARAM_IO_CAPABILITIES;
    sec_param.oob            = SEC_PARAM_OOB;
    sec_param.min_key_size   = SEC_PARAM_MIN_KEY_SIZE;
    sec_param.max_key_size   = SEC_PARAM_MAX_KEY_SIZE;
    sec_param.kdist_own.enc  = 1;
    sec_param.kdist_own.id   = 1;
    sec_param.kdist_peer.enc = 1;
    sec_param.kdist_peer.id  = 1;

    err_code = pm_sec_params_set(&sec_param);
    APP_ERROR_CHECK(err_code);

    err_code = pm_register(pm_evt_handler);
    APP_ERROR_CHECK(err_code);
}


/**@brief Clear bond information from persistent storage.
 */
static void delete_bonds(void)
{
    ret_code_t err_code;

    NRF_LOG_INFO("Erase bonds!");

    err_code = pm_peers_delete();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling events from the BSP module.
 *
 * @param[in]   event   Event generated when button is pressed.
 */
static void bsp_event_handler(bsp_event_t event)
{
    ret_code_t err_code;

    switch (event)
    {
        case BSP_EVENT_SLEEP:
            sleep_mode_enter();
            break; // BSP_EVENT_SLEEP

        case BSP_EVENT_DISCONNECT:
            err_code = sd_ble_gap_disconnect(m_conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if (err_code != NRF_ERROR_INVALID_STATE)
            {
                APP_ERROR_CHECK(err_code);
            }
            break; // BSP_EVENT_DISCONNECT

        case BSP_EVENT_WHITELIST_OFF:
            if (m_conn_handle == BLE_CONN_HANDLE_INVALID)
            {
                err_code = ble_advertising_restart_without_whitelist(&m_advertising);
                if (err_code != NRF_ERROR_INVALID_STATE)
                {
                    APP_ERROR_CHECK(err_code);
                }
            }
            break; // BSP_EVENT_KEY_0

        default:
            break;
    }
}


/**@brief Function for initializing the Advertising functionality.
 */
static void advertising_init(void)
{
    ret_code_t             err_code;
    ble_advertising_init_t init;

    memset(&init, 0, sizeof(init));

    init.advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    init.advdata.include_appearance = false;
    init.advdata.flags              = BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE;

    init.srdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    init.srdata.uuids_complete.p_uuids  = m_adv_uuids;


    init.config.ble_adv_fast_enabled  = true;
    init.config.ble_adv_fast_interval = APP_ADV_INTERVAL;
    init.config.ble_adv_fast_timeout  = APP_ADV_DURATION;

    init.evt_handler = on_adv_evt;

    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);

    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}


/**@brief Function for initializing buttons and leds.
 *
 * @param[out] p_erase_bonds  Will be true if the clear bonding button was pressed to wake the application up.
 */
static void buttons_leds_init(bool * p_erase_bonds)
{
    ret_code_t err_code;
    bsp_event_t startup_event;

    err_code = bsp_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS, bsp_event_handler);
    APP_ERROR_CHECK(err_code);

    err_code = bsp_btn_ble_init(NULL, &startup_event);
    APP_ERROR_CHECK(err_code);

    *p_erase_bonds = (startup_event == BSP_EVENT_CLEAR_BONDING_DATA);
}


/**@brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}


/**@brief Function for initializing power management.
 */
static void power_management_init(void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the idle state (main loop).
 *
 * @details If there is no pending log operation, then sleep until next the next event occurs.
 */
static void idle_state_handle(void)
{
    if (NRF_LOG_PROCESS() == false)
    {
        nrf_pwr_mgmt_run();
    }
}


/**@brief Function for starting advertising.
 */
static void advertising_start(bool erase_bonds)
{
    if (erase_bonds == true)
    {
        delete_bonds();
        // Advertising is started by PM_EVT_PEERS_DELETED_SUCEEDED event
    }
    else
    {
        ret_code_t err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);

        APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for application main entry.
 */
int main(void)
{
    bool erase_bonds;

    //NRF_POWER->DCDCEN = 1;
    // Initialize.
    log_init();
    timers_init();
    buttons_leds_init(&erase_bonds);
    power_management_init();
    ble_stack_init();
    //sd_power_dcdc_mode_set( NRF_POWER_DCDC_ENABLE );
    gap_params_init();
    gatt_init();
    //advertising_init();
    services_init();
    advertising_init();
    conn_params_init();
    peer_manager_init();

    // Start execution.
    NRF_LOG_INFO("Template example started.");
    //NRF_LOG_FLUSH();
    //application_timers_start();
    high_filter_set();
    adpcm_state_init();
    advertising_start(erase_bonds);
    NRF_LOG_FLUSH();

    twi_init();
   
    LIS2DH_set_mode();
    twi_accel_ppi_init();
    twi_accel_ppi_enable();
    //twi_start();
    //application_timers_start();

    // Enter main loop.
    for (;;)
    {
        idle_state_handle();
    }
}


/**
 * @}
 */
