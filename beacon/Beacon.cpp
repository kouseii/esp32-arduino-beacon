#include "Beacon.h"

#define SCAN_DATA_BUFFER_SIZE 10
AdvertisingDataFilter Beacon::_advertisingDataFilter;
AdvertisingDataFilter Beacon::_advertisingDataFilterMask;
ScanData Beacon::_scanData[SCAN_DATA_BUFFER_SIZE];
volatile uint32_t gScanDataWriteIndex = 0;
volatile bool gIsScanning = false;

Beacon::Beacon(void)
{
}

Beacon::~Beacon(void)
{
}

void Beacon::begin(void)
{
  // バッファを初期化.
  memset(_advertisingDataFilter, 0, sizeof(AdvertisingDataFilter));
  memset(_advertisingDataFilterMask, 0, sizeof(AdvertisingDataFilter));
  memset(_scanData, NULL, sizeof(_scanData));

  // BTモジュールを初期化.
  btStart();

  // bluedroidを初期化.
  if (esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_UNINITIALIZED) {
    esp_bluedroid_init();
    while (esp_bluedroid_get_status() != ESP_BLUEDROID_STATUS_INITIALIZED) {
      ;
    }
  }
  if (esp_bluedroid_get_status() != ESP_BLUEDROID_STATUS_ENABLED) {
    esp_bluedroid_enable();
    while (esp_bluedroid_get_status() != ESP_BLUEDROID_STATUS_ENABLED) {
      ;
    }
  }

  // コールバック関数(割り込み)を登録.
  esp_ble_gap_register_callback(Beacon::_esp_gap_ble_cb);
}

void Beacon::end(void)
{
  if (esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_ENABLED) {
    esp_bluedroid_disable();
    while (esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_ENABLED) {
      ;
    }
  }
  if (esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_INITIALIZED) {
    esp_bluedroid_deinit();
    while (esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_INITIALIZED) {
      ;
    }
  }

  if (btStarted()) {
    btStop();
  }
}

void Beacon::setScanFilter(const AdvertisingDataFilter filter, const AdvertisingDataFilter mask)
{
  if (filter) {
    memcpy(_advertisingDataFilter, filter, sizeof(AdvertisingDataFilter));
  } else {
    memset(_advertisingDataFilter, NULL, sizeof(AdvertisingDataFilter));
  }
  if (mask) {
    memcpy(_advertisingDataFilterMask, mask, sizeof(AdvertisingDataFilter));
  } else {
    memset(_advertisingDataFilterMask, NULL, sizeof(AdvertisingDataFilter));
  }
}

void Beacon::startScan(uint32_t duration, uint16_t scanInterval, uint16_t scanWindow)
{
  // スキャンパラメータを設定.
  esp_ble_scan_params_t ble_scan_params;
  memset(&ble_scan_params, NULL, sizeof(esp_ble_scan_params_t));
  ble_scan_params.scan_type = BLE_SCAN_TYPE_PASSIVE;
  ble_scan_params.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
  ble_scan_params.scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL;
  ble_scan_params.scan_interval = scanInterval;
  ble_scan_params.scan_window = scanWindow;
  esp_ble_gap_set_scan_params(&ble_scan_params);

  // スキャニングしてみる(非ブロック関数).
  if (esp_ble_gap_start_scanning(duration) == ESP_OK) {
    gIsScanning = true;
  }
}

bool Beacon::isScanning(void)
{
  return gIsScanning;
}

const ScanData *Beacon::getScanResult(void)
{
  uint32_t i = gScanDataWriteIndex;
  for (int c = 0; c < SCAN_DATA_BUFFER_SIZE; c++) {
    ScanData *sd = &_scanData[i++ % SCAN_DATA_BUFFER_SIZE];
    if (sd->newly) {
      sd->newly = 0;
      return sd;
    }
  }
  return NULL;
}

void Beacon::_esp_gap_ble_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
  // 注意:割り込み関数のためデバッグ出力や外部とのコミュニケーションは禁止です.
  // リングバッファ等にスキャニングした結果を蓄えるなどの処理にしてください.
  if (event == ESP_GAP_BLE_SCAN_RESULT_EVT) {
    if (param->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_CMPL_EVT
        || param->scan_rst.search_evt == ESP_GAP_SEARCH_SEARCH_CANCEL_CMPL_EVT) {
      // 受信終了.
      gIsScanning = false;
    } else {
      // マニュファクチャデータであることをチェック.
      uint8_t *ble_adv = param->scan_rst.ble_adv;
      if (ble_adv[4] != 0xff) {
        return;
      }

      // Filter設定に従ってチェック.
      for (uint8_t i = 5, fi = 0; i < ESP_BLE_ADV_DATA_LEN_MAX; i++, fi++) {
        if ((ble_adv[i] & _advertisingDataFilterMask[fi]) != _advertisingDataFilter[fi]) {
          return;
        }
      }

      // リングバッファに詰め込む.
      ScanData& sd = _scanData[gScanDataWriteIndex++ % SCAN_DATA_BUFFER_SIZE];
      sd.rssi = param->scan_rst.rssi;
      memcpy(sd.ble_adv, param->scan_rst.ble_adv, ESP_BLE_ADV_DATA_LEN_MAX);
      sd.newly = 1;
    }
  }
}
