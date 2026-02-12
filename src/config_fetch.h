#ifndef CONFIG_FETCH_H
#define CONFIG_FETCH_H

#include <Arduino.h>
#include "device_id.h"

/**
 * HTTP GET request to fetch configuration
 * Uses the discovered mDNS service details + device ID
 */

typedef struct {
  int http_code;           // HTTP response code (200, 404, 500, etc.)
  char error_msg[256];     // Error message if failed
  bool success;            // true if 200 OK and config retrieved
  char config_json[2048];  // Raw JSON response
} ConfigResponse;

/**
 * Fetch configuration from discovered server
 *
 * Parameters:
 *   - host: Hostname or IP address (from mDNS discovery)
 *   - port: Server port (from mDNS discovery)
 *   - device_id: Device identification structure
 *   - response: Output buffer for response data
 *
 * Returns: ConfigResponse with HTTP code and data
 *
 * Example:
 *   DeviceID my_device = initializeDeviceID();
 *   ConfigResponse resp = fetchConfigFromServer(
 *     "192.168.1.100", 5050, &my_device
 *   );
 *   if (resp.success) {
 *     Serial.println(resp.config_json);
 *   }
 */
ConfigResponse fetchConfigFromServer(
  const char* host,
  uint16_t port,
  const DeviceID* device_id
);

/**
 * Parse retrieved JSON config and extract MQTT settings
 * Supports:
 *   - mqtt_broker, mqtt_port, mqtt_topic
 *   - poll_frequency_sec, heartbeat_frequency_sec
 *   - template
 */
typedef struct {
  char mqtt_broker[128];
  uint16_t mqtt_port;
  char mqtt_topic[256];
  uint16_t poll_frequency_sec;
  uint16_t heartbeat_frequency_sec;
  char template_name[32];
} MQTTConfig;

/**
 * Parse MQTT configuration from JSON response
 */
MQTTConfig parseConfigJSON(const char* json_response);

#endif
