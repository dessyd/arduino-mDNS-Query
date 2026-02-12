/**
 * ============================================================================
 * mDNS Module Header
 * ============================================================================
 * mDNS query sending and response handling
 */

#ifndef MDNS_H
#define MDNS_H

#include <Arduino.h>
#include "arduino_configs.h"

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

/**
 * Send mDNS service discovery PTR query
 *
 * Builds and sends a PTR query to the mDNS multicast group (224.0.0.251:5353)
 * for the configured service type.
 *
 * RETURNS:
 *   true  - Query sent successfully
 *   false - Failed to build or send query
 */
bool sendMDNSQuery(void);

/**
 * Handle incoming mDNS response packet
 *
 * Processes received UDP packet:
 *   - Validates packet size and header
 *   - Validates response matches requested service
 *   - Extracts answer records (SRV, TXT, A)
 *   - Builds configuration URL
 *
 * PARAMETERS:
 *   packetSize - Size of received UDP packet in bytes
 */
void handleMDNSResponse(int packetSize);

/**
 * Get the last discovered configuration
 *
 * RETURNS:
 *   Pointer to DiscoveredConfig struct
 */
const DiscoveredConfig* getDiscoveredConfig(void);

#endif  // MDNS_H
