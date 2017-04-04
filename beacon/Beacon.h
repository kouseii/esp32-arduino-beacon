#ifndef _BEACON_H_
#define _BEACON_H_

#include "Arduino.h"

#include "bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"

typedef struct {
  int8_t rssi;
  uint8_t ble_adv[ESP_BLE_ADV_DATA_LEN_MAX];
  uint8_t newly;
} ScanData;

/**
 * 受信データのフィルタ定義の型.
 */
typedef uint8_t AdvertisingDataFilter[ESP_BLE_ADV_DATA_LEN_MAX - 5];

class Beacon {
  public:
    Beacon(void);
    ~Beacon(void);
  public:
    void begin(void);
    void end(void);
  public:
    /**
     * 受信フィルタ条件を設定する.
     *
     * @param filter [in] フィルタ定義.
     * @param mask [in] マスク.
     */
    void setScanFilter(const AdvertisingDataFilter filter, const AdvertisingDataFilter mask);
    /**
     * @duration スキャン時間(秒). 無限不可.
     * @scanInterval 0.625ミリ秒単位の値(0x0004～0x4000).
     * @scanWindow 0.625ミリ秒単位の値(0x0004～0x4000).
     */
    void startScan(uint32_t duration, uint16_t scanInterval, uint16_t scanWindow);
    bool isScanning(void);
    /**
     * @return スキャンデータが無ければNULLを返却.
     */
    const ScanData *getScanResult(void);
  private:
    static void _esp_gap_ble_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
  private:
    /**
     * 受信データのフィルタ設定.
     */
    static AdvertisingDataFilter _advertisingDataFilter;
    /**
     * 受信データのフィルタ設定マスク.
     */
    static AdvertisingDataFilter _advertisingDataFilterMask;
    static ScanData _scanData[];
};

#endif
