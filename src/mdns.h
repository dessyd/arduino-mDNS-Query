/**
 * ============================================================================
 * mDNS Module Header
 * ============================================================================
 * mDNS query sending and response handling
 */

#ifndef MDNS_H
#define MDNS_H

#include "config.h"

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
