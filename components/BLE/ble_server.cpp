// System headers
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

// Other Component headers
extern "C" {
#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_common_api.h"
#include "esp_gatts_api.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
}
// Own component header
#include <ble_server.hpp>

// Static defines
static constexpr const char *FILE_TAG = "BLE_SERVER";
static constexpr int adv_config_flag = (1 << 0);
static constexpr int scan_rsp_config_flag = (1 << 1);
static constexpr int GATTS_SERVICE_UUID_TEST_A = 0x00FE;
static constexpr int GATTS_CHAR_UUID_TEST_A = 0xFF01;
static constexpr int GATTS_DESCR_UUID_TEST_A = 0x3333;
static constexpr int GATTS_NUM_HANDLE_TEST_A = 4;

static constexpr const char *TEST_DEVICE_NAME = "ESP32-CHIP8";
static constexpr int TEST_MANUFACTURER_DATA_LEN = 17;

// static std::map<uint8_t, BLEService *> m_service_ptr;

static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = false,
    .min_interval = 0x0006, // slave connection min interval, Time =
                            // min_interval * 1.25 msec
    .max_interval = 0x0010, // slave connection max interval, Time =
                            // max_interval * 1.25 msec
    .appearance = 0x00,
    .manufacturer_len = 0,       // TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data = NULL, //&test_manufacturer[0],
    .service_data_len = 0,
    .p_service_data = NULL,
    // .service_uuid_len = sizeof(adv_service_uuid128),
    // .p_service_uuid = adv_service_uuid128,
    .service_uuid_len = 0,
    .p_service_uuid = NULL,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};
// scan response data
static esp_ble_adv_data_t scan_rsp_data = {
    .set_scan_rsp = true,
    .include_name = true,
    .include_txpower = true,
    //.min_interval = 0x0006,
    //.max_interval = 0x0010,
    .appearance = 0x00,
    .manufacturer_len = 0,       // TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data = NULL, //&test_manufacturer[0],
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = 0,
    .p_service_uuid = NULL,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    //.peer_addr            =
    //.peer_addr_type       =
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static uint8_t char1_str[] = {0x11, 0x22, 0x33};
static constexpr int GATTS_DEMO_CHAR_VAL_LEN_MAX = 0x40;
static esp_attr_value_t gatts_demo_char1_val = {
    .attr_max_len = GATTS_DEMO_CHAR_VAL_LEN_MAX,
    .attr_len = sizeof(char1_str),
    .attr_value = char1_str,
};

// Class impl

std::vector<BLEService *> BLEServer::m_services = {};
std::map<uint8_t, BLEService *> BLEServer::m_service_ptr{};

[[nodiscard]] esp_err_t BLEServer::init() {
  esp_err_t ret = ESP_OK;

  // Initialize NVS.
  ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  ret = esp_bt_controller_init(&bt_cfg);
  if (ret) {
    ESP_LOGE(FILE_TAG, "%s initialize controller failed: %s\n", __func__,
             esp_err_to_name(ret));
    return ret;
  }

  ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
  if (ret) {
    ESP_LOGE(FILE_TAG, "%s enable controller failed: %s\n", __func__,
             esp_err_to_name(ret));
    return ret;
  }
  ret = esp_bluedroid_init();
  if (ret) {
    ESP_LOGE(FILE_TAG, "%s init bluetooth failed: %s\n", __func__,
             esp_err_to_name(ret));
    return ret;
  }
  ret = esp_bluedroid_enable();
  if (ret) {
    ESP_LOGE(FILE_TAG, "%s enable bluetooth failed: %s\n", __func__,
             esp_err_to_name(ret));
    return ret;
  }
  ret = esp_ble_gap_register_callback(gap_event_handler);
  if (ret) {
    ESP_LOGE(FILE_TAG, "gap register error, error code = %x", ret);
    return ret;
  }
  return ret;
}

void BLEServer::gatts_event_handler_dispatcher(
    esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
    esp_ble_gatts_cb_param_t *param) {

  if (event == ESP_GATTS_REG_EVT) {
    if (param->reg.status == ESP_GATT_OK) {
      // Check for nullptr?
      m_service_ptr.emplace(
          std::make_pair(gatts_if, m_services[param->reg.app_id]));
    } else {
      ESP_LOGI(FILE_TAG, "Reg app failed, app_id %04x, status %d\n",
               param->reg.app_id, param->reg.status);
      return;
    }
  }

  if (gatts_if == ESP_GATT_IF_NONE) {
    for (auto &ele : m_services) {
      ele->gatts_profile_a_event_handler(event, gatts_if, param);
    }
  } else {
    m_service_ptr[gatts_if]->gatts_profile_a_event_handler(event, gatts_if,
                                                           param);
  }
}

esp_err_t BLEServer::addService(BLEService *service) {
  esp_err_t ret = ESP_OK;

  m_services.emplace_back(service);
  ret = esp_ble_gatts_register_callback(gatts_event_handler_dispatcher);
  if (ret) {
    ESP_LOGE(FILE_TAG, "gatts register error, error code = %x", ret);
    return ret;
  }
  return ret;
}

[[nodiscard]] esp_err_t BLEServer::startService() {
  esp_err_t ret = ESP_OK;
  for (uint8_t app_ids = 0; app_ids < m_services.size(); ++app_ids) {
    ret = esp_ble_gatts_app_register(app_ids);
    if (ret) {
      ESP_LOGE(FILE_TAG, "gatts app register error, error code = %x", ret);
    }
  }

  esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
  if (local_mtu_ret) {
    ESP_LOGE(FILE_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
  }
  return ret;
}

BLEService::BLEService(const std::string &service_name)
    : m_service_name{service_name}, is_device_connected{false} {
  m_queue_handle = xQueueCreate(5, sizeof(uint8_t));
  if (m_queue_handle == NULL) {
    ESP_LOGE(FILE_TAG, "Queue creation failed!");
  }
  m_profile = {};
  // m_char_prop = 0;
}

bool BLEService::isDeviceConnected() { return is_device_connected; }

void BLEService::onRead(uint8_t value) {
  const char *str = m_service_name.c_str();
  // std::cout << "Hello" << "\n" ;
  ESP_LOGI(FILE_TAG, "On read executing! %s%d", str, value);
}

void BLEService::onWrite(uint8_t value) {
  if (xQueueSend(m_queue_handle, &value, 1 / portTICK_PERIOD_MS) != pdPASS) {
    ESP_LOGE(FILE_TAG, "Failed to send the message\n");
  }
  ESP_LOGD(FILE_TAG, "On write executing! Received %d", value);
}
xQueueHandle BLEService::getQueueHandle() { return m_queue_handle; }

// TODO:Later check if this var is really needed
uint8_t BLEServer::adv_config_done = 0;
void BLEServer::gap_event_handler(esp_gap_ble_cb_event_t event,
                                  esp_ble_gap_cb_param_t *param) {
  esp_ble_adv_params_t adv_params = {
      .adv_int_min = 0x20,
      .adv_int_max = 0x40,
      .adv_type = ADV_TYPE_IND,
      .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
      //.peer_addr            =
      //.peer_addr_type       =
      .channel_map = ADV_CHNL_ALL,
      .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
  };

  switch (event) {
  case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
    adv_config_done &= (~adv_config_flag);
    if (adv_config_done == 0) {
      esp_ble_gap_start_advertising(&adv_params);
    }
    break;
  case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
    adv_config_done &= (~scan_rsp_config_flag);
    if (adv_config_done == 0) {
      esp_ble_gap_start_advertising(&adv_params);
    }
    break;
  case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
    // advertising start complete event to indicate advertising start
    // successfully or failed
    if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
      ESP_LOGE(FILE_TAG, "Advertising start failed\n");
    }
    break;
  case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
    if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
      ESP_LOGE(FILE_TAG, "Advertising stop failed\n");
    } else {
      ESP_LOGI(FILE_TAG, "Stop adv successfully\n");
    }
    break;
  case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
    ESP_LOGI(
        FILE_TAG,
        "update connection params status = %d, min_int = %d, "
        "max_int = %d,conn_int = %d,latency = %d, timeout = %d",
        param->update_conn_params.status, param->update_conn_params.min_int,
        param->update_conn_params.max_int, param->update_conn_params.conn_int,
        param->update_conn_params.latency, param->update_conn_params.timeout);
    break;
  default:
    break;
  }
}

void BLEService::gatts_profile_a_event_handler(
    esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if,
    esp_ble_gatts_cb_param_t *param) {
  switch (event) {
  case ESP_GATTS_REG_EVT: {
    ESP_LOGI(FILE_TAG, "REGISTER_APP_EVT, status %d, app_id %d\n",
             param->reg.status, param->reg.app_id);
    m_profile.service_id.is_primary = true;
    m_profile.service_id.id.inst_id = 0x00;
    m_profile.service_id.id.uuid.len = ESP_UUID_LEN_16;
    m_profile.service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_TEST_A;

    esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(TEST_DEVICE_NAME);
    if (set_dev_name_ret) {
      ESP_LOGE(FILE_TAG, "set device name failed, error code = %x",
               set_dev_name_ret);
    }
    // config adv data
    esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
    if (ret) {
      ESP_LOGE(FILE_TAG, "config adv data failed, error code = %x", ret);
    }
    /* adv_config_done |= adv_config_flag; */
    // config scan response data
    ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
    if (ret) {
      ESP_LOGE(FILE_TAG, "config scan response data failed, error code = %x",
               ret);
    }
    /* adv_config_done |= scan_rsp_config_flag; */

    esp_ble_gatts_create_service(gatts_if, &m_profile.service_id,
                                 GATTS_NUM_HANDLE_TEST_A);
    break;
  }
  case ESP_GATTS_READ_EVT: {
    ESP_LOGI(FILE_TAG, "GATT_READ_EVT, conn_id %d, trans_id %d, handle %d\n",
             param->read.conn_id, param->read.trans_id, param->read.handle);
    esp_gatt_rsp_t rsp;
    memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
    rsp.attr_value.handle = param->read.handle;
    rsp.attr_value.len = 1;
    rsp.attr_value.value[0] = 0xfe;
    esp_ble_gatts_send_response(gatts_if, param->read.conn_id,
                                param->read.trans_id, ESP_GATT_OK, &rsp);
    break;
  }
  case ESP_GATTS_WRITE_EVT: {
    ESP_LOGI(FILE_TAG, "GATT_WRITE_EVT, conn_id %d, trans_id %d, handle %d",
             param->write.conn_id, param->write.trans_id, param->write.handle);
    if (!param->write.is_prep) {
      // This is where the circular buffer insert goes
      onWrite(*(param->write.value));
      ESP_LOGD(FILE_TAG, "GATT_WRITE_EVT, value len %d, value :%02x",
               param->write.len, *(param->write.value));
      esp_log_buffer_hex(FILE_TAG, param->write.value, param->write.len);
      ESP_LOGD(FILE_TAG, "Profile descr handle: %#x, Param handle: %#x",
               m_profile.descr_handle, param->write.handle);
      if (param->write.need_rsp) {
        esp_err_t send_rsp_err = esp_ble_gatts_send_response(
            gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK,
            NULL);
        if (send_rsp_err) {
          ESP_LOGE(FILE_TAG, "Send response failed!");
        }
      }
      break;
    }
  case ESP_GATTS_EXEC_WRITE_EVT:
    /* ESP_LOGI(FILE_TAG, "ESP_GATTS_EXEC_WRITE_EVT"); */
    /* esp_ble_gatts_send_response(gatts_if, param->write.conn_id,
     */
    /*                             param->write.trans_id,
     * ESP_GATT_OK,
     */
    /*                             NULL); */
    /* example_exec_write_event_env(&a_prepare_write_env, param); */
    break;
  case ESP_GATTS_MTU_EVT:
    ESP_LOGI(FILE_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
    break;
  case ESP_GATTS_UNREG_EVT:
    break;
  case ESP_GATTS_CREATE_EVT: {
    ESP_LOGI(FILE_TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d\n",
             param->create.status, param->create.service_handle);
    m_profile.service_handle = param->create.service_handle;
    m_profile.char_uuid.len = ESP_UUID_LEN_16;
    m_profile.char_uuid.uuid.uuid16 = GATTS_CHAR_UUID_TEST_A;

    esp_ble_gatts_start_service(m_profile.service_handle);
    esp_gatt_char_prop_t char_prop = ESP_GATT_CHAR_PROP_BIT_READ |
                                     ESP_GATT_CHAR_PROP_BIT_WRITE |
                                     ESP_GATT_CHAR_PROP_BIT_NOTIFY;
    esp_err_t add_char_ret =
        esp_ble_gatts_add_char(m_profile.service_handle, &m_profile.char_uuid,
                               ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                               char_prop, &gatts_demo_char1_val, NULL);
    if (add_char_ret) {
      ESP_LOGE(FILE_TAG, "add char failed, error code =%x", add_char_ret);
    }
    break;
  }
  case ESP_GATTS_ADD_INCL_SRVC_EVT:
    break;
  case ESP_GATTS_ADD_CHAR_EVT: {
    uint16_t length = 0;
    const uint8_t *prf_char;

    ESP_LOGI(FILE_TAG,
             "ADD_CHAR_EVT, status %d,  attr_handle %d, "
             "service_handle %d\n",
             param->add_char.status, param->add_char.attr_handle,
             param->add_char.service_handle);
    m_profile.char_handle = param->add_char.attr_handle;
    m_profile.descr_uuid.len = ESP_UUID_LEN_16;
    m_profile.descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
    esp_err_t get_attr_ret = esp_ble_gatts_get_attr_value(
        param->add_char.attr_handle, &length, &prf_char);
    if (get_attr_ret == ESP_FAIL) {
      ESP_LOGE(FILE_TAG, "ILLEGAL HANDLE");
    }

    ESP_LOGI(FILE_TAG, "the gatts demo char length = %x\n", length);
    for (int i = 0; i < length; i++) {
      ESP_LOGI(FILE_TAG, "prf_char[%x] =%x\n", i, prf_char[i]);
    }
    esp_err_t add_descr_ret = esp_ble_gatts_add_char_descr(
        m_profile.service_handle, &m_profile.descr_uuid,
        ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, NULL, NULL);
    if (add_descr_ret) {
      ESP_LOGE(FILE_TAG, "add char descr failed, error code =%x",
               add_descr_ret);
    }
    break;
  }
  case ESP_GATTS_ADD_CHAR_DESCR_EVT: {
    m_profile.descr_handle = param->add_char_descr.attr_handle;
    ESP_LOGI(FILE_TAG,
             "ADD_DESCR_EVT, status %d, attr_handle %d, "
             "service_handle %d\n",
             param->add_char_descr.status, param->add_char_descr.attr_handle,
             param->add_char_descr.service_handle);
    break;
  }
  case ESP_GATTS_DELETE_EVT:
    break;
  case ESP_GATTS_START_EVT:
    ESP_LOGI(FILE_TAG, "SERVICE_START_EVT, status %d, service_handle %d\n",
             param->start.status, param->start.service_handle);
    break;
  case ESP_GATTS_STOP_EVT:
    break;
  case ESP_GATTS_CONNECT_EVT: {
    is_device_connected = true;
    esp_ble_conn_update_params_t conn_params = {0};
    memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
    /* For the IOS system, please reference the apple official
     * documents about the ble connection parameters restrictions.
     */
    conn_params.latency = 0;
    conn_params.max_int = 0x20; // max_int = 0x20*1.25ms = 40ms
    conn_params.min_int = 0x10; // min_int = 0x10*1.25ms = 20ms
    conn_params.timeout = 400;  // timeout = 400*10ms = 4000ms
    ESP_LOGI(FILE_TAG,
             "ESP_GATTS_CONNECT_EVT, conn_id %d, remote "
             "%02x:%02x:%02x:%02x:%02x:%02x:",
             param->connect.conn_id, param->connect.remote_bda[0],
             param->connect.remote_bda[1], param->connect.remote_bda[2],
             param->connect.remote_bda[3], param->connect.remote_bda[4],
             param->connect.remote_bda[5]);
    m_profile.conn_id = param->connect.conn_id;
    // start sent the update connection parameters to the peer
    // device.
    esp_ble_gap_update_conn_params(&conn_params);
    break;
  }
  case ESP_GATTS_DISCONNECT_EVT:
    ESP_LOGI(FILE_TAG, "ESP_GATTS_DISCONNECT_EVT, disconnect reason 0x%x",
             param->disconnect.reason);
    esp_ble_gap_start_advertising(&adv_params);
    break;
  case ESP_GATTS_CONF_EVT:
    ESP_LOGI(FILE_TAG, "ESP_GATTS_CONF_EVT, status %d attr_handle %d",
             param->conf.status, param->conf.handle);
    if (param->conf.status != ESP_GATT_OK) {
      esp_log_buffer_hex(FILE_TAG, param->conf.value, param->conf.len);
    }
    break;
  case ESP_GATTS_OPEN_EVT:
  case ESP_GATTS_CANCEL_OPEN_EVT:
  case ESP_GATTS_CLOSE_EVT:
  case ESP_GATTS_LISTEN_EVT:
  case ESP_GATTS_CONGEST_EVT:
  default:
    break;
  }
  }
}
