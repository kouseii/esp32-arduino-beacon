#include "Beacon.h"

Beacon beacon;

// 受信フィルター.
static AdvertisingDataFilter beaconFilter = {
  0x4C, 0x00, // Company identifier code (0x004C == Apple)
  0x02,       // ID
  0x15,       // length of the remaining payload
  0x00, 0x00, 0x00, 0x00, 0xDF, 0xF2, 0x10, 0x01, // location UUID
  0xB0, 0x00, 0x00, 0x1C, 0x4D, 0xF2, 0x6B, 0x36,
  0x00, 0x00, // the major value to differentiate a location
  0x00, 0x00, // the minor value to differentiate a location
  0x00,       // 2's complement of the Tx power (-56dB)
  0x00        // remain
};
// 受信フィルターのマスク.
static AdvertisingDataFilter beaconFilterMask = {
  0xFF, 0xFF, // Company identifier code (0x004C == Apple)
  0xFF,       // ID
  0xFF,       // length of the remaining payload
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // location UUID
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0x00, 0x00, // the major value to differentiate a location
  0x00, 0x00, // the minor value to differentiate a location
  0x00,       // 2's complement of the Tx power (-56dB)
  0x00        // remain
};

void setup() {
  Serial.begin(115200);

  beacon.begin();
  beacon.setScanFilter(beaconFilter, beaconFilterMask);
}

void loop() {
  // 無限受信する.
  if (!beacon.isScanning()) {
    beacon.startScan(60, 0x0010, 0x0010); // 60秒間区切りで受信.
  }

  const ScanData *sd = beacon.getScanResult();
  if (sd) {
    const uint8_t *ble_adv = sd->ble_adv;
    for (uint8_t i = 0; i < ESP_BLE_ADV_DATA_LEN_MAX; i++) {
      Serial.printf("%02x", ble_adv[i]);
    }
    Serial.println();
  }
}

