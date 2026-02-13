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

#endif  // ARDUINO_CONFIGS_H
