/**
 * ============================================================================
 * Network Module Header
 * ============================================================================
 * WiFi connection and mDNS UDP socket initialization
 */

#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <WiFiUdp.h>

/**
 * Establish WiFi connection with timeout protection
 *
 * FEATURES:
 *   - Timeout protection (30 seconds max)
 *   - Detailed logging of connection progress
 *   - Returns success/failure status
 *
 * RETURNS:
 *   true  - WiFi successfully connected
 *   false - Connection failed or timed out
 */
bool connectToWiFi(void);

/**
 * Initialize mDNS UDP socket and prepare for service discovery
 *
 * SEQUENCE:
 *   1. Create UDP socket on port 5353
 *   2. Join multicast group 224.0.0.251:5353
 *   3. Begin listening for mDNS responses
 *
 * RETURNS:
 *   true  - mDNS initialized successfully
 *   false - Socket binding or multicast join failed
 */
bool initMDNS(void);

/**
 * Get reference to UDP socket for packet operations
 *
 * RETURNS:
 *   Reference to WiFiUDP object
 */
WiFiUDP& getUDPSocket(void);

/**
 * Non-blocking delay that allows system to process interrupts
 *
 * Unlike delay(), this yields to the system periodically,
 * allowing WiFi and other interrupts to be processed.
 *
 * PARAMETERS:
 *   durationMs - Duration in milliseconds to wait
 *
 * NOTES:
 *   - Returns immediately after timeout
 *   - WiFi stack can process packets during wait
 *   - Ideal for initialization phase
 */
void nonBlockingDelay(uint32_t durationMs);

#endif  // NETWORK_H
