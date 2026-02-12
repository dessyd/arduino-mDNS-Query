#ifndef DEVICE_ID_H
#define DEVICE_ID_H

#include <Arduino.h>

/**
 * Device ID Management
 * Reads device serial from ATECC608A crypto chip and WiFi MAC address
 */

// Structure to hold device identification
typedef struct {
  char device_id[65];      // Hex serial from crypto chip (8-64 chars)
  char mac_address[18];    // WiFi MAC: XX:XX:XX:XX:XX:XX (17 chars + null)
  bool valid;              // Both IDs successfully read
} DeviceID;

/**
 * Read device serial number from ATECC608A crypto chip
 * Converts to uppercase hexadecimal string
 *
 * Returns: pointer to hex string (65 bytes max), or NULL on failure
 */
char* getDeviceSerial(char* buffer, size_t max_len);

/**
 * Read WiFi MAC address from the MKR WiFi 1010
 * Formats as XX:XX:XX:XX:XX:XX
 *
 * Returns: pointer to MAC string (18 bytes), or NULL on failure
 */
char* getWiFiMAC(char* buffer, size_t max_len);

/**
 * Initialize device identification
 * Reads both serial and MAC, validates format
 *
 * Returns: populated DeviceID structure
 */
DeviceID initializeDeviceID();

/**
 * Build GET /config URL with parameters
 * Format: /config?device_id=<serial>&mac=<mac>
 *
 * Returns: pointer to URL string
 */
char* buildConfigURL(const DeviceID* device_id, char* buffer, size_t max_len);

#endif
