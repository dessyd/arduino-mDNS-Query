/**
 * ============================================================================
 * Arduino Configuration Header
 * ============================================================================
 * Centralized configuration for mDNS Service Discovery
 * Naming Convention: CONFIG_xxxx for all configuration constants
 */

#ifndef ARDUINO_CONFIGS_H
#define ARDUINO_CONFIGS_H

// ============================================================================
// DEBUG CONFIGURATION
// ============================================================================
#ifndef DEBUG
#define DEBUG true  // Set to false in production (saves memory & CPU)
#endif

#if DEBUG
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINTF(fmt, val) \
    do { Serial.print(fmt); Serial.println(val); } while(0)
  #define DEBUG_HEX(label, val, width) \
    do { Serial.print(label); Serial.println((val), HEX); } while(0)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(fmt, val)
  #define DEBUG_HEX(label, val, width)
#endif

// ============================================================================
// NETWORK CONFIGURATION
// ============================================================================
#ifndef CONFIG_LOCAL_UDP_PORT
#define CONFIG_LOCAL_UDP_PORT 5354
#endif

#ifndef CONFIG_MDNS_PORT
#define CONFIG_MDNS_PORT 5353
#endif

#ifndef CONFIG_WIFI_TIMEOUT_MS
#define CONFIG_WIFI_TIMEOUT_MS 30000
#endif

// ============================================================================
// mDNS SERVICE DISCOVERY CONFIGURATION
// ============================================================================
#ifndef CONFIG_MDNS_SERVICE_TYPE
#define CONFIG_MDNS_SERVICE_TYPE "config"
#endif

#ifndef CONFIG_MDNS_PROTOCOL
#define CONFIG_MDNS_PROTOCOL "tcp"
#endif

#ifndef CONFIG_MDNS_DOMAIN
#define CONFIG_MDNS_DOMAIN "local"
#endif

#ifndef CONFIG_QUERY_INTERVAL_MS
#define CONFIG_QUERY_INTERVAL_MS 10000
#endif

// ============================================================================
// DNS PROTOCOL CONSTANTS
// ============================================================================
#define CONFIG_DNS_TYPE_PTR   12
#define CONFIG_DNS_CLASS_IN   1

// ============================================================================
// BUFFER AND SIZE LIMITS
// ============================================================================
#define CONFIG_PACKET_BUFFER_SIZE 256
#define CONFIG_SERVICE_NAME_MAX_LEN 128
#define CONFIG_HOSTNAME_MAX_LEN 128
#define CONFIG_PATH_MAX_LEN 64
#define CONFIG_VERSION_MAX_LEN 16
#define CONFIG_IP_STR_MAX_LEN 16
#define CONFIG_URL_MAX_LEN 256

// ============================================================================
// SERIAL CONFIGURATION
// ============================================================================
#ifndef CONFIG_SERIAL_WAIT_TIMEOUT
#define CONFIG_SERIAL_WAIT_TIMEOUT 5000
#endif

// ============================================================================
// RTC (REAL-TIME CLOCK) CONFIGURATION
// ============================================================================

// RTC synchronization interval (milliseconds)
// Non-blocking sync with WiFiNINA.getTime() every 60 seconds
#ifndef CONFIG_RTC_SYNC_INTERVAL_MS
#define CONFIG_RTC_SYNC_INTERVAL_MS 60000
#endif

// Bootstrap timestamp for RTC initialization before network sync
// Default: 2026-02-13 00:00:00 UTC (reasonable starting point)
// Unix timestamp: 1739404800
#ifndef CONFIG_RTC_BOOTSTRAP_TIMESTAMP
#define CONFIG_RTC_BOOTSTRAP_TIMESTAMP 1739404800UL
#endif

// RTC staleness threshold (milliseconds)
// Mark RTC as "stale" if last sync was >5 minutes ago
#ifndef CONFIG_RTC_STALE_THRESHOLD_MS
#define CONFIG_RTC_STALE_THRESHOLD_MS 300000
#endif

// ============================================================================
// SENSOR TELEMETRY CONFIGURATION
// ============================================================================

// Sensor change detection thresholds (based on hardware accuracy specs)
// Used to determine if a sensor reading has changed significantly enough to publish

// Temperature: HTS221 accuracy is ±0.5°C (threshold in Celsius)
#ifndef CONFIG_TEMP_THRESHOLD_CELSIUS
#define CONFIG_TEMP_THRESHOLD_CELSIUS 0.5f
#endif

// Humidity: HTS221 accuracy is ±3.5% (threshold in percentage points)
#ifndef CONFIG_HUMIDITY_THRESHOLD_PERCENT
#define CONFIG_HUMIDITY_THRESHOLD_PERCENT 3.5f
#endif

// Pressure: LPS22HB accuracy is ±1 hPa (threshold in millibars/hPa)
#ifndef CONFIG_PRESSURE_THRESHOLD_HPA
#define CONFIG_PRESSURE_THRESHOLD_HPA 1.0f
#endif

// Illuminance: TEMT6000 accuracy is ±5% relative
// Threshold as percentage of current reading (for relative change)
#ifndef CONFIG_ILLUMINANCE_THRESHOLD_PERCENT
#define CONFIG_ILLUMINANCE_THRESHOLD_PERCENT 5.0f
#endif

// Illuminance: Absolute threshold in lux (noise floor for very low light)
// Used when absolute difference is more meaningful than relative
#ifndef CONFIG_ILLUMINANCE_THRESHOLD_ABS_LUX
#define CONFIG_ILLUMINANCE_THRESHOLD_ABS_LUX 50.0f
#endif

// UV Index: ML8511 accuracy is ±0.5 (threshold in index units)
#ifndef CONFIG_UV_THRESHOLD_INDEX
#define CONFIG_UV_THRESHOLD_INDEX 0.5f
#endif

#endif  // ARDUINO_CONFIGS_H
