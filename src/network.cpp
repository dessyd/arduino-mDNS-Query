/**
 * ============================================================================
 * Network Module - Implementation
 * ============================================================================
 * WiFi and mDNS initialization
 */

#include "network.h"
#include "config.h"
#include "arduino_secrets.h"

#include <WiFiNINA.h>

// Global WiFi credentials (from arduino_secrets.h)
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

// Global UDP socket for mDNS communication
static WiFiUDP udpSocket;

// mDNS multicast address (224.0.0.251)
static IPAddress mdnsMulticastIP(224, 0, 0, 251);

// ============================================================================
// PUBLIC FUNCTIONS
// ============================================================================

bool connectToWiFi(void)
{
  DEBUG_PRINT(F("Connecting to WiFi: "));
  DEBUG_PRINTLN(ssid);

  WiFi.begin(ssid, pass);

  const uint32_t START_TIME = millis();

  while (WiFi.status() != WL_CONNECTED) {
    // Check for timeout
    if (millis() - START_TIME > CONFIG_WIFI_TIMEOUT_MS) {
      DEBUG_PRINTLN(F("\n✗ WiFi connection timeout!"));
      return false;
    }

    nonBlockingDelay(250);  // Non-blocking wait with yield
    DEBUG_PRINT(F("."));
  }

  DEBUG_PRINTLN(F(""));
  DEBUG_PRINT(F("✓ WiFi connected! IP: "));
  DEBUG_PRINTLN(WiFi.localIP());
  return true;
}

bool initMDNS(void)
{
  DEBUG_PRINT(F("Initializing mDNS on port "));
  DEBUG_PRINTLN(CONFIG_MDNS_PORT);

  // Begin listening on mDNS port
  if (!udpSocket.begin(CONFIG_LOCAL_UDP_PORT)) {
    DEBUG_PRINTLN(F("✗ Failed to bind UDP socket!"));
    return false;
  }

  DEBUG_PRINTLN(F("✓ mDNS initialized, listening for responses..."));
  return true;
}

WiFiUDP& getUDPSocket(void)
{
  return udpSocket;
}

IPAddress getMDNSMulticastIP(void)
{
  return mdnsMulticastIP;
}

void nonBlockingDelay(uint32_t durationMs)
{
  uint32_t startTime = millis();

  while (millis() - startTime < durationMs) {
    yield();  // Allow WiFi stack to process interrupts
  }
}
