#ifndef BLE_SERVER_HPP_
#define BLE_SERVER_HPP_
extern "C" {
#include <esp_err.h>

#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_common_api.h"
#include "esp_gatts_api.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/queue.h"
}
#include <map>
#include <string>
#include <vector>

struct gatts_profile_inst {
  esp_gatts_cb_t gatts_cb;
  uint16_t gatts_if;
  uint16_t app_id;
  uint16_t conn_id;
  uint16_t service_handle;
  esp_gatt_srvc_id_t service_id;
  uint16_t char_handle;
  esp_bt_uuid_t char_uuid;
  esp_gatt_perm_t perm;
  esp_gatt_char_prop_t property;
  uint16_t descr_handle;
  esp_bt_uuid_t descr_uuid;
  gatts_profile_inst() = default;
};

/* class BLEChar { */
/*    public: */
/*     BLEChar(esp_gatt_char_prop_t char_prop); */

/*    private: */
/*     esp_gatt_char_prop_t m_char_prop; */
/* }; */

class BLEService {
public:
  BLEService(const std::string &service_name);
  /* [[nodiscard]] esp_err_t addChar(BLEChar* chars); */
  void onRead(uint8_t value);
  // Need to implement
  void onWrite(uint8_t value);
  void gatts_profile_a_event_handler(esp_gatts_cb_event_t event,
                                     esp_gatt_if_t gatts_if,
                                     esp_ble_gatts_cb_param_t *param);
  xQueueHandle getQueueHandle();

private:
  std::string m_service_name;
  xQueueHandle m_queue_handle;
  // TODO: Find out why this var works only when it is static
  // static gatts_profile_inst m_profile;
  //   static esp_gatt_char_prop_t m_char_prop;
  gatts_profile_inst m_profile;
  //   esp_gatt_char_prop_t m_char_prop;
};

//TODO: Change this into a singleton class
class BLEServer {
public:
  BLEServer() = default;
  [[nodiscard]] esp_err_t init();
  [[nodiscard]] static esp_err_t addService(BLEService *service);
  [[nodiscard]] esp_err_t startService();

private:
  static void gatts_event_handler_dispatcher(esp_gatts_cb_event_t event,
                                             esp_gatt_if_t gatts_if,
                                             esp_ble_gatts_cb_param_t *param);
  static void gap_event_handler(esp_gap_ble_cb_event_t event,
                                esp_ble_gap_cb_param_t *param);
  BLEService *m_service;
  static std::vector<BLEService *> m_services;
  static uint8_t adv_config_done;
};

#endif // !BLE_SERVER_HPP_
