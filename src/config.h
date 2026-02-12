/**
 * ============================================================================
 * Configuration Header - mDNS Service Discovery
 * ============================================================================
 * Centralized configuration for network, mDNS, and feature flags
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>

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

// ============================================================================
// PROTOCOL CONSTANTS
// ============================================================================
#define DNS_TYPE_PTR   12
#define DNS_CLASS_IN   1

// ============================================================================
// BUFFER AND TIMING CONSTANTS
// ============================================================================
#define PACKET_BUFFER_SIZE 256
#define SERVICE_NAME_MAX_LEN 128
#define HOSTNAME_MAX_LEN 128
#define PATH_MAX_LEN 64
#define VERSION_MAX_LEN 16
#define IP_STR_MAX_LEN 16
#define URL_MAX_LEN 256

#define WIFI_TIMEOUT_MS 30000
#define QUERY_INTERVAL_MS 10000
#define SERIAL_WAIT_TIMEOUT 5000

// ============================================================================
// DATA STRUCTURES
// ============================================================================

/**
 * Discovered Service Configuration
 * Stores extracted mDNS data for the config service
 */
typedef struct {
  char hostname[HOSTNAME_MAX_LEN];   // Target hostname (e.g., "myserver.local")
  uint16_t port;                      // Service port (from SRV record)
  char path[PATH_MAX_LEN];            // HTTP path (from TXT record, e.g., "/config")
  char version[VERSION_MAX_LEN];      // API version (from TXT record, e.g., "1.0")
  uint32_t ipAddress;                 // IPv4 address (from A record)
  char ipStr[IP_STR_MAX_LEN];         // IP as dotted decimal (e.g., "192.168.1.1")
  bool valid;                         // All required fields populated
} DiscoveredConfig;

#endif  // CONFIG_H
