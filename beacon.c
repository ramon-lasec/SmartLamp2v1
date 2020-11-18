
/** @file
*
* Beacon sample
*
* This app demonstrates use of Google Eddystone and Apple iBeacons via the
* beacon library. It also demonstrates uses of multi-advertisement feature.
*
* During initialization the app configures advertisement packets for Eddystone and iBeacon
* and starts advertisements via multi-advertisement APIs.
* It also sets up a 1 sec timer to update Eddystone TLM advertisement data
*
* Features demonstrated
*  - configuring Apple iBeacon & Google Eddystone advertisements
*  - Apple iBeacon & Google Eddystone adv message will be advertised simultaneously.
*  - OTA Firmware Upgrade
*
* To demonstrate the app, work through the following steps.
*
* 1. Plug the WICED eval board into your computer
* 2. Build and download the application to the WICED board
* 3. Monitor advertisement packets -
*        - on Android, download  app such as 'Beacon Scanner' by Nicholas Briduox
*        - on iOS, download app such as 'Locate Beacon'. Add UUID for iBeacon (see below UUID_IBEACON)
*        - or use over the air sniffer
* 4. Run BTSpy app, capture protocol and snoop traces and view the view viewer such as Fronline
*
*/
#include "wiced_hal_wdog.h"
#include "wiced_hal_rand.h"
#include "wiced_hal_nvram.h"

#include "wiced_bt_trace.h"
#include "wiced_bt_cfg.h"
#if ( defined(CYW20706A2) || defined(CYW20719B1) || defined(CYW20719B0) || defined(CYW20721B1) || defined(CYW20735B0) || defined(CYW43012C0) )
#include "wiced_bt_app_common.h"
#endif
#include "wiced_bt_stack.h"
#include "wiced_timer.h"
#include "wiced_bt_beacon.h"
#include "string.h"
#include "sparcommon.h"
//#include "GeneratedSource/cycfg_gatt_db.h"
#ifndef CYW43012C0
#include "wiced_bt_ota_firmware_upgrade.h"
#endif
#include "wiced_hal_puart.h"
#include "wiced_platform.h"
#include "wiced_transport.h"

#include <malloc.h>

#include "wiced_bt_mesh_models.h"
#include "wiced_bt_mesh_app.h"
#include "wiced_bt_mesh_core.h"


unsigned char data_save_flash[20];
uint8_t data_uuid[16];
wiced_result_t  status;
uint16_t        numbytes;


/******************************************************************************
*                                Constants
******************************************************************************/
/* Multi advertisement instance ID */
#define BEACON_EDDYSTONE_UID 1
#define BEACON_EDDYSTONE_URL 2
#define BEACON_EDDYSTONE_EID 3
#define BEACON_EDDYSTONE_TLM 4
#define BEACON_IBEACON       5
#define NVRAM_ID_LOCAL_UUID         (NVRAM_ID_NODE_INFO + 2)

/* User defined UUID for iBeacon */
#define UUID_IBEACON     0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f

#ifdef OTA_SECURE_FIRMWARE_UPGRADE
#include "bt_types.h"
#include "p_256_multprecision.h"
#include "p_256_ecc_pp.h"

// If secure version of the OTA firmware upgrade is used, the app should be linked with the ecdsa256_pub.c
// which exports the public key
extern Point    ecdsa256_public_key;
#endif
/******************************************************************************
 *                                Structures
 ******************************************************************************/

#if defined(CYW20735B1) || defined(CYW20819A1) || defined(CYW20719B2) || defined(CYW20721B2) || defined (WICEDX)
/* Adv parameter used for multi-adv*/
wiced_bt_ble_multi_adv_params_t adv_param =
#else
wiced_bt_beacon_multi_advert_data_t adv_param =
#endif
{
    .adv_int_min = BTM_BLE_ADVERT_INTERVAL_MIN,
    .adv_int_max = BTM_BLE_ADVERT_INTERVAL_MAX,
    .adv_type = MULTI_ADVERT_NONCONNECTABLE_EVENT,
    .channel_map = BTM_BLE_ADVERT_CHNL_37 | BTM_BLE_ADVERT_CHNL_38 | BTM_BLE_ADVERT_CHNL_39,
    .adv_filter_policy = BTM_BLE_ADVERT_FILTER_WHITELIST_CONNECTION_REQ_WHITELIST_SCAN_REQ,
    .adv_tx_power = MULTI_ADV_TX_POWER_MAX,
    .peer_bd_addr = {0},
    .peer_addr_type = BLE_ADDR_PUBLIC,
    .own_bd_addr = {0},
    .own_addr_type = BLE_ADDR_PUBLIC
};

/******************************************************************************
 *                              Variables Definitions
 ******************************************************************************/
/* Beacon timer */
static wiced_timer_t beacon_timer;
uint16_t      beacon_conn_id = 0;

extern  wiced_bt_cfg_settings_t app_cfg_settings;
extern  wiced_bt_cfg_buf_pool_t app_buf_pools[];

/* transport configuration, needed for WICED HCI traces */
const wiced_transport_cfg_t transport_cfg =
{
    .type = WICED_TRANSPORT_UART,
    .cfg =
    {
        .uart_cfg =
        {
            .mode = WICED_TRANSPORT_UART_HCI_MODE,
            .baud_rate =  HCI_UART_DEFAULT_BAUD
        },
    },
    .rx_buff_pool_cfg =
    {
        .buffer_size = 0,
        .buffer_count = 0
    },
    .p_status_handler = NULL,
    .p_data_handler = NULL,
    .p_tx_complete_cback = NULL
};

/******************************************************************************
 *                             Local Function Definitions
 ******************************************************************************/
static void                     beacon_init(void);
static wiced_result_t           beacon_management_callback(wiced_bt_management_evt_t event, wiced_bt_management_evt_data_t *p_event_data);
static void                     beacon_advertisement_stopped(void);




static void beacon_data_update(uint32_t arg);
static void beacon_set_app_advertisement_data();

static void beacon_set_eddystone_tlm_advertisement_data(void);

/******************************************************************************
 *                          Function Definitions
 ******************************************************************************/

/*
 *  Entry point to the application. Set device configuration and start BT
 *  stack initialization.  The actual application initialization will happen
 *  when stack reports that BT device is ready.
 */

static uint32_t mesh_nvram_access(wiced_bool_t write, int inx, uint8_t* node_info, uint16_t len, wiced_result_t *p_result)
{
    uint32_t len_res = len;


    if (!write)
        len_res = wiced_hal_read_nvram(inx, len, node_info, p_result);
    // 0 len means delete
    else if (len != 0)
        len_res = wiced_hal_write_nvram(inx, len, node_info, p_result);
    else
        wiced_hal_delete_nvram(inx, p_result);

    //WICED_BT_TRACE("mesh_nvram_access: inx:%x write:%d result:%x len:%d/%d\n", inx, write, *p_result, len, len_res);
    return len_res;
}


APPLICATION_START()
{

    wiced_bt_gatt_status_t gatt_status;

    wiced_transport_init( &transport_cfg );

#ifdef WICED_BT_TRACE_ENABLE
    /*
     * Set the debug uart to enable the debug traces
     */

    /*
     * Sets the UART type as WICED_ROUTE_DEBUG_TO_PUART,
     * For disabling the traces set the UART type as WICED_ROUTE_DEBUG_NONE, and
     * For sending debug strings over the WICED debug interface set the UART type as WICED_ROUTE_DEBUG_TO_WICED_UART
     */
#ifdef NO_PUART_SUPPORT
    wiced_set_debug_uart( WICED_ROUTE_DEBUG_TO_WICED_UART );
#else
    wiced_set_debug_uart( WICED_ROUTE_DEBUG_TO_PUART );
#ifdef CYW20706A2
    wiced_hal_puart_select_uart_pads( WICED_PUART_RXD, WICED_PUART_TXD, 0, 0);
#endif
#endif

    // To set to HCI to see traces on HCI uart -
    // wiced_set_debug_uart( WICED_ROUTE_DEBUG_TO_HCI_UART );

#endif

	// Register call back and configuration with stack
	uint8_t buff_name[30];
	   uint8_t BT_LOCAL_NAME[] = { 'L', '4', 'S', 'E', 'C', ' ', 'B', 'B', 'B', ' ', ' ', ' ', ' ', '\0' };

	data_uuid[0]=0x45;
	data_uuid[1]=0x45;
	data_uuid[2]=0xF5;
	data_uuid[3]=0;
	data_uuid[4]=0;
	data_uuid[5]=0xF5;

	const char bn[30];
	memcpy(bn,BT_LOCAL_NAME,1);

	memcpy(data_save_flash,data_uuid ,16);
	data_save_flash[16]=0x2c;
	data_save_flash[17]=0x31;
	data_save_flash[18]=0x0A;
	data_save_flash[19]=0x0D;
	numbytes = wiced_hal_write_nvram( WICED_NVRAM_VSID_START, sizeof(data_save_flash), &data_save_flash, &status );

	numbytes = wiced_hal_read_nvram( WICED_NVRAM_VSID_START, sizeof(data_save_flash), &data_save_flash, &status );

	//sprintf(buff_name,"L4SEC %02x%02x\0",data_save_flash[5],data_save_flash[6]);
	//sprintf(&buff_name,"L4SEC");
	//app_cfg_settings.device_name = (uint8_t*)bn;



	wiced_bt_stack_init(beacon_management_callback, &app_cfg_settings, app_buf_pools);
}

/*
 * This function is executed in the BTM_ENABLED_EVT management callback.
 */
void beacon_init(void)
{
    wiced_bt_gatt_status_t gatt_status;
    wiced_result_t         result;

#ifdef CYW20706A2
#if defined(USE_256K_SECTOR_SIZE)
    wiced_hal_sflash_use_erase_sector_size_256K(1);
    wiced_hal_sflash_use_4_byte_address(1);
#endif
#endif

    /* Register with stack to receive GATT callback */

    /*  Tell stack to use our GATT database */

    wiced_bt_dev_register_hci_trace(NULL);
    /* Allow peer to pair */
    wiced_bt_set_pairable_mode(WICED_TRUE, 0);

#if OTA_FW_UPGRADE
    /* OTA Firmware upgrade Initialization */
#ifdef OTA_SECURE_FIRMWARE_UPGRADE
    if (!wiced_ota_fw_upgrade_init(&ecdsa256_public_key, NULL, NULL))
#else
    if (!wiced_ota_fw_upgrade_init(NULL, NULL, NULL))
#endif
    {
          WICED_BT_TRACE("OTA upgrade Init failure !!! \n");
    }
#endif
    /* Set the advertising params and make the device discoverable */
    beacon_set_app_advertisement_data();


    numbytes = wiced_hal_read_nvram( WICED_NVRAM_VSID_START, sizeof(data_save_flash), &data_save_flash, &status );

    wiced_bt_device_address_t bda;
    bda[0]=data_save_flash[0];
    bda[1]=data_save_flash[1];
    bda[2]=data_save_flash[2];
    bda[3]=data_save_flash[3];
    bda[4]=data_save_flash[4];
    bda[5]=data_save_flash[5];


	wiced_bt_ble_address_type_t  macc=BLE_ADDR_PUBLIC;

    wiced_bt_set_local_bdaddr(bda,macc);



    result =  wiced_bt_start_advertisements(BTM_BLE_ADVERT_UNDIRECTED_HIGH, 0, NULL);
    WICED_BT_TRACE("wiced_bt_start_advertisements %d\n", result);
}


static uint32_t adv_cnt = 0;
static uint32_t sec_cnt = 0;
static void beacon_set_eddystone_tlm_advertisement_data(void)
{
    uint8_t adv_data_tlm[31];
    uint8_t adv_len_tlm = 0;

    /* Set sample values for Eddystone TLM */
    uint16_t vbatt = 10;
    uint16_t temp =  15;

    /* For each invocation of API, update Advertising PDU count and Time since power-on or reboot*/
    adv_cnt++;
    sec_cnt++;

    memset(adv_data_tlm, 0, 31);

    /* Call Eddystone TLM api to prepare adv data*/
    wiced_bt_eddystone_set_data_for_tlm_unencrypted(vbatt, temp, adv_cnt, sec_cnt, adv_data_tlm, &adv_len_tlm);

    /* Sets adv data for multi adv instance*/
    wiced_set_multi_advertisement_data(adv_data_tlm, adv_len_tlm, BEACON_EDDYSTONE_TLM);
}


/* Function called on timer */
void beacon_data_update(uint32_t arg)
{
    /* Stops Eddystone TLM advertisements */
    wiced_start_multi_advertisements(MULTI_ADVERT_STOP, BEACON_EDDYSTONE_TLM);

    // Set Eddystone TLM adv data
    beacon_set_eddystone_tlm_advertisement_data();

    /* Sets adv data for multi adv instance*/
    adv_param.adv_int_min = 1280; // 800 ms
    adv_param.adv_int_max = 1280; // 800 ms
#if defined(CYW20735B1) || defined(CYW20819A1) || defined(CYW20719B2) || defined(CYW20721B2) || defined (WICEDX)
    wiced_set_multi_advertisement_params(BEACON_EDDYSTONE_TLM, &adv_param);
#else
    wiced_set_multi_advertisement_params(adv_param.adv_int_min, adv_param.adv_int_max, adv_param.adv_type,
            adv_param.own_addr_type, adv_param.own_bd_addr, adv_param.peer_addr_type, adv_param.peer_bd_addr,
            adv_param.channel_map, adv_param.adv_filter_policy,
            BEACON_EDDYSTONE_TLM, adv_param.adv_tx_power);
#endif
    /* Starts Eddystone TLM advertisements */
    wiced_start_multi_advertisements(MULTI_ADVERT_START, BEACON_EDDYSTONE_TLM);
}

/*
 * Setup advertisement data with 16 byte UUID and device name
 */
void beacon_set_app_advertisement_data(void)
{
    wiced_bt_ble_advert_elem_t adv_elem[2];
    uint8_t num_elem = 0;
    uint8_t flag = BTM_BLE_GENERAL_DISCOVERABLE_FLAG | BTM_BLE_BREDR_NOT_SUPPORTED;

    adv_elem[num_elem].advert_type  = BTM_BLE_ADVERT_TYPE_FLAG;
    adv_elem[num_elem].len          = sizeof(uint8_t);
    adv_elem[num_elem].p_data       = &flag;
    num_elem++;

    adv_elem[num_elem].advert_type  = BTM_BLE_ADVERT_TYPE_NAME_COMPLETE;
    adv_elem[num_elem].len          = strlen((const char *)app_cfg_settings.device_name);
    adv_elem[num_elem].p_data       = (uint8_t*)app_cfg_settings.device_name;
    num_elem++;

    wiced_bt_ble_set_raw_advertisement_data(num_elem, adv_elem);
}

/*
 * This function is invoked when advertisements stop.  Continue advertising if there
 * are no active connections
 */
void beacon_advertisement_stopped(void)
{
    wiced_result_t result;

    // while we are not connected
    if (beacon_conn_id == 0)
    {
        result =  wiced_bt_start_advertisements(BTM_BLE_ADVERT_UNDIRECTED_LOW, 0, NULL);
        WICED_BT_TRACE("wiced_bt_start_advertisements: %d\n", result);
    }
    else
    {
        WICED_BT_TRACE("ADV stop\n");
    }
}

/*
 * Application management callback.  Stack passes various events to the function that may
 * be of interest to the application.
 */
wiced_result_t beacon_management_callback(wiced_bt_management_evt_t event, wiced_bt_management_evt_data_t *p_event_data)
{
    wiced_result_t                    result = WICED_BT_SUCCESS;
    uint8_t                          *p_keys;
    wiced_bt_ble_advert_mode_t       *p_mode;

    WICED_BT_TRACE("beacon_management_callback: %x\n", event);

    switch(event)
    {
    /* Bluetooth  stack enabled */
    case BTM_ENABLED_EVT:
        beacon_init();


        break;

    case BTM_DISABLED_EVT:
        break;

    case BTM_PAIRING_IO_CAPABILITIES_BLE_REQUEST_EVT:
        p_event_data->pairing_io_capabilities_ble_request.local_io_cap  = BTM_IO_CAPABILITIES_NONE;
        p_event_data->pairing_io_capabilities_ble_request.oob_data      = BTM_OOB_NONE;
        p_event_data->pairing_io_capabilities_ble_request.auth_req      = BTM_LE_AUTH_REQ_BOND | BTM_LE_AUTH_REQ_MITM;
        p_event_data->pairing_io_capabilities_ble_request.max_key_size  = 0x10;
        p_event_data->pairing_io_capabilities_ble_request.init_keys     = BTM_LE_KEY_PENC | BTM_LE_KEY_PID;
        p_event_data->pairing_io_capabilities_ble_request.resp_keys     = BTM_LE_KEY_PENC | BTM_LE_KEY_PID;
        break;

    case BTM_SECURITY_REQUEST_EVT:
        wiced_bt_ble_security_grant( p_event_data->security_request.bd_addr, WICED_BT_SUCCESS );
        break;

    case BTM_PAIRED_DEVICE_LINK_KEYS_UPDATE_EVT:
        break;

     case BTM_LOCAL_IDENTITY_KEYS_UPDATE_EVT:
         break;


     case BTM_LOCAL_IDENTITY_KEYS_REQUEST_EVT:
        break;

    case BTM_BLE_ADVERT_STATE_CHANGED_EVT:
        p_mode = &p_event_data->ble_advert_state_changed;
        WICED_BT_TRACE("Advertisement State Change: %d\n", *p_mode);
        if (*p_mode == BTM_BLE_ADVERT_OFF)
        {
            beacon_advertisement_stopped();
        }
        break;

    default:
        break;
    }

    return result;
}

