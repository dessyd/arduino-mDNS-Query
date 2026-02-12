/**
 * ============================================================================
 * Arduino mDNS Service Discovery (PTR Query Only)
 * ============================================================================
 *
 * PURPOSE:
 *   Query for mDNS services using simple PTR queries.
 *   Sends to 224.0.0.251:5353 and listens for responses.
 *
 * PLATFORM: Arduino MKR WiFi 1010 (SAMD21, 32KB RAM)
 * PROTOCOL: RFC 6762 (Multicast DNS) - PTR queries only
 * MEMORY: ~5.8 KB RAM (18% usage)
 *
 * ============================================================================
 * MODULE ARCHITECTURE:
 * ============================================================================
 * - config.h        : Centralized configuration and data structures
 * - network.h/.cpp  : WiFi and mDNS UDP socket initialization
 * - packet.h/.cpp   : DNS packet building and parsing
 * - mdns.h/.cpp     : mDNS query sending and response handling
 * - main (this file): Program flow (setup/loop)
 *
 * ============================================================================
 */

#include "config.h"
#include "network.h"
#include "packet.h"
#include "mdns.h"

// ============================================================================
// SETUP - Initialize hardware, WiFi, and mDNS
// ============================================================================

/**
 * setup() - Run once at startup
 *
 * INITIALIZATION ORDER:
 *   1. Serial communication (debugging)
 *   2. WiFi connection
 *   3. mDNS network setup
 *   4. Send initial query
 */
void setup(void)
{
#if DEBUG
  // Initialize serial communication (debug only)
  Serial.begin(115200);

  // Wait for serial port (with timeout for non-USB boards)
  uint32_t serialWaitStart = millis();
  while (!Serial && millis() - serialWaitStart < SERIAL_WAIT_TIMEOUT)
  {
    yield();  // Allow system to process without blocking
  }

  DEBUG_PRINTLN(F(""));
  DEBUG_PRINTLN(F("=== Arduino mDNS Service Discovery ==="));
  DEBUG_PRINTLN(F("RFC 6762 / RFC 6763 Implementation"));
  DEBUG_PRINTLN(F(""));
#endif

  // Establish WiFi connection
  if (!connectToWiFi())
  {
    DEBUG_PRINTLN(F("✗ WiFi setup failed - halting"));
    while (1)
    {
      yield();  // Fatal error - halt indefinitely
    }
  }

  // Initialize mDNS networking
  if (!initMDNS())
  {
    DEBUG_PRINTLN(F("✗ mDNS setup failed - halting"));
    while (1)
    {
      yield();  // Fatal error - halt indefinitely
    }
  }

  // Send initial mDNS query
  if (!sendMDNSQuery())
  {
    DEBUG_PRINTLN(F("⚠ Initial query failed, retrying in loop"));
  }

  DEBUG_PRINTLN(F("✓ Setup complete - entering main loop"));
}

// ============================================================================
// LOOP - Main event loop
// ============================================================================

/**
 * loop() - Run continuously
 *
 * TIMING:
 *   - Sends mDNS queries every 10 seconds
 *   - Listens for responses continuously
 *   - Non-blocking (uses millis() instead of delay)
 *
 * STATE MACHINE:
 *   1. Check if query interval elapsed
 *   2. If yes: send new mDNS query
 *   3. Listen for UDP responses from mDNS responders
 *   4. Process and log discovered services
 *   5. Loop back
 */
void loop(void)
{
  static uint32_t lastQueryTime = 0;
  uint32_t now = millis();

  // === STEP 1: Send periodic mDNS queries ===
  if (now - lastQueryTime >= CONFIG_QUERY_INTERVAL_MS)
  {
    lastQueryTime = now;
    sendMDNSQuery();
  }

  // === STEP 2: Listen for mDNS responses ===
  WiFiUDP& udp = getUDPSocket();
  int packetSize = udp.parsePacket();
  if (packetSize > 0)
  {
    handleMDNSResponse(packetSize);
  }

  // Loop runs continuously - WiFi interrupts handled by hardware
}
