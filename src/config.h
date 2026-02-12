/**
 * ============================================================================
 * Configuration Header - mDNS Service Discovery
 * ============================================================================
 * Local configuration and data structures
 * All CONFIG_* constants are centralized in include/arduino_configs.h
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>
#include "arduino_configs.h"

// ============================================================================
// DATA STRUCTURES
// ============================================================================

/**
 * Discovered Service Configuration
 * Stores extracted mDNS data for the config service
 */
typedef struct {
  char hostname[CONFIG_HOSTNAME_MAX_LEN];   // Target hostname (e.g., "myserver.local")
  uint16_t port;                             // Service port (from SRV record)
  char path[CONFIG_PATH_MAX_LEN];            // HTTP path (from TXT record, e.g., "/config")
  char version[CONFIG_VERSION_MAX_LEN];      // API version (from TXT record, e.g., "1.0")
  uint32_t ipAddress;                        // IPv4 address (from A record)
  char ipStr[CONFIG_IP_STR_MAX_LEN];         // IP as dotted decimal (e.g., "192.168.1.1")
  bool valid;                                // All required fields populated
} DiscoveredConfig;

#endif  // CONFIG_H
