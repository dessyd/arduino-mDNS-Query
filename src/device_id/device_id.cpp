#include <Arduino.h>
#include "device_id/device_id.h"
#include "arduino_configs.h"
#include <WiFiNINA.h>

#include <ArduinoECCX08.h>

/**
 * Get device serial from ATECC608A crypto chip
 * The serial number is 9 bytes and stored in permanent read-only slots
 *
 * ATECC608A Serial Layout:
 *   Bytes 0-3:  Device SN[0:3]  (manufacturer specific)
 *   Bytes 4-8:  Device SN[4:8]  (unique per chip)
 * Total: 9 bytes → 18 hex characters
 */
char* getDeviceSerial(char* buffer, size_t max_len)
{
  if (!buffer || max_len < 19) {
    DEBUG_PRINTLN(F("✗ Buffer too small for serial number"));
    return NULL;
  }

  // Initialize ECCX08 crypto chip
  if (!ECCX08.begin()) {
    DEBUG_PRINTLN(F("✗ ECCX08 initialization failed - check I2C connection (SDA/SCL)"));
    return NULL;
  }

  // Read serial number from crypto chip
  // The serial is permanently stored in the chip and cannot be modified
  uint8_t serial[9];

  if (!ECCX08.readSlot(0, serial, 9)) {
    DEBUG_PRINTLN(F("✗ Failed to read serial from ATECC608A"));
    ECCX08.end();
    return NULL;
  }

  // Convert 9 bytes to 18-character uppercase hex string
  size_t pos = 0;
  for (size_t i = 0; i < 9 && pos < max_len - 1; i++) {
    pos += snprintf(
      buffer + pos,
      max_len - pos,
      "%02X",
      serial[i]
    );
  }

  buffer[pos] = '\0';
  ECCX08.end();

  DEBUG_PRINT(F("✓ Device Serial (ATECC608A): "));
  DEBUG_PRINTLN(buffer);
  DEBUG_PRINT(F("  Raw bytes: "));
  for (size_t i = 0; i < 9; i++) {
    if (serial[i] < 0x10) DEBUG_PRINT(F("0"));
    Serial.print(serial[i], HEX);
    if (i < 8) DEBUG_PRINT(F(" "));
  }
  DEBUG_PRINTLN();

  return buffer;
}

/**
 * Get WiFi MAC address
 * Format: XX:XX:XX:XX:XX:XX
 */
char* getWiFiMAC(char* buffer, size_t max_len)
{
  if (!buffer || max_len < 18) {
    DEBUG_PRINTLN(F("✗ Buffer too small for MAC address"));
    return NULL;
  }

  // Get MAC address from WiFi module
  uint8_t mac[6];
  WiFi.macAddress(mac);

  // Format as XX:XX:XX:XX:XX:XX
  snprintf(
    buffer,
    max_len,
    "%02X:%02X:%02X:%02X:%02X:%02X",
    mac[0], mac[1], mac[2],
    mac[3], mac[4], mac[5]
  );

  DEBUG_PRINT(F("✓ WiFi MAC: "));
  DEBUG_PRINTLN(buffer);

  return buffer;
}

/**
 * Initialize device identification
 * Reads and validates both serial and MAC
 */
DeviceID initializeDeviceID()
{
  DeviceID device;
  device.valid = false;

  // Read device serial
  if (!getDeviceSerial(device.device_id, sizeof(device.device_id))) {
    DEBUG_PRINTLN(F("✗ Failed to read device serial"));
    return device;
  }

  // Read WiFi MAC (must be done after WiFi.begin())
  if (!getWiFiMAC(device.mac_address, sizeof(device.mac_address))) {
    DEBUG_PRINTLN(F("✗ Failed to read WiFi MAC"));
    return device;
  }

  // Validate formats
  if (strlen(device.device_id) >= 8 && strlen(device.mac_address) >= 17) {
    device.valid = true;
    DEBUG_PRINTLN(F("✓ Device ID initialized successfully"));
  }

  return device;
}

/**
 * Build complete GET /config URL
 * Format: /config?device_id=<serial>&mac=<mac>
 */
char* buildConfigURL(const DeviceID* device_id, char* buffer, size_t max_len)
{
  if (!device_id || !device_id->valid || !buffer) {
    DEBUG_PRINTLN(F("✗ Invalid DeviceID for URL building"));
    return NULL;
  }

  snprintf(
    buffer,
    max_len,
    "/config?device_id=%s&mac=%s",
    device_id->device_id,
    device_id->mac_address
  );

  DEBUG_PRINT(F("✓ Config URL: "));
  DEBUG_PRINTLN(buffer);

  return buffer;
}
