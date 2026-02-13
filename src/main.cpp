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

#include "mdns/network.h"
#include "mdns/packet.h"
#include "mdns/mdns.h"
#include "device_id/device_id.h"
#include "config_fetch/config_fetch.h"
#include "mqtt/mqtt_publish.h"
#include "sensors/sensors.h"

// ============================================================================
// GLOBAL STATE - Device and configuration tracking
// ============================================================================

static DeviceID device;
static MQTTConfig mqtt_config;
static bool config_fetched = false;
static uint32_t last_config_fetch_attempt = 0;
static const uint32_t CONFIG_FETCH_RETRY_INTERVAL = 30000;  // Retry every 30s

static bool mqtt_initialized = false;
static uint32_t last_publish_time = 0;
static const uint32_t PUBLISH_INTERVAL = 10000;  // Publish every 10s

static SensorReadings sensor_data;
static bool sensors_initialized = false;

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
  while (!Serial && millis() - serialWaitStart < CONFIG_SERIAL_WAIT_TIMEOUT)
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

  // Initialize device identification (serial number + MAC address)
  device = initializeDeviceID();
  if (!device.valid)
  {
    DEBUG_PRINTLN(F("✗ Device ID initialization failed - halting"));
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

  // Initialize environmental sensors
  if (!initSensors())
  {
    DEBUG_PRINTLN(F("⚠ Sensor initialization failed - will publish without sensor data"));
    sensors_initialized = false;
  }
  else
  {
    sensors_initialized = true;
    DEBUG_PRINTLN(F("✓ Environmental sensors initialized"));
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

  // === IF CONFIG ALREADY FETCHED: FOCUS ON MQTT ===
  if (config_fetched)
  {
    // Maintain MQTT connection
    MQTTStatus mqtt_status = maintainMQTT();

    // Publish sensor data at regular intervals
    if (isMQTTReady() && now - last_publish_time >= PUBLISH_INTERVAL)
    {
      last_publish_time = now;

      char payload[512];  // Increased from 256 to accommodate full sensor data

      // Read and publish sensor data if available
      if (sensors_initialized && readSensors(&sensor_data))
      {
        // Format sensor readings as JSON
        char sensor_json[256];
        if (formatSensorJSON(&sensor_data, sensor_json, sizeof(sensor_json)))
        {
          // Combine device info + sensor data
          // sensor_json is like: {"temperature":23.5,"humidity":45.2,...}
          // We skip the opening brace and insert after device fields
          snprintf(payload, sizeof(payload),
                   "{\"device_id\":\"%s\",\"mac\":\"%s\",%s",
                   device.device_id,
                   device.mac_address,
                   sensor_json + 1);  // Skip opening brace from sensor JSON
        }
        else
        {
          // JSON formatting failed, fall back to device info only
          snprintf(payload, sizeof(payload),
                   "{\"device_id\":\"%s\",\"mac\":\"%s\",\"timestamp\":%lu}",
                   device.device_id,
                   device.mac_address,
                   now / 1000);
        }
      }
      else
      {
        // Sensors not available or read failed, publish device info only
        snprintf(payload, sizeof(payload),
                 "{\"device_id\":\"%s\",\"mac\":\"%s\",\"timestamp\":%lu}",
                 device.device_id,
                 device.mac_address,
                 now / 1000);
      }

      publishToMQTT(nullptr, payload);
    }
    return;  // Skip remaining config discovery code
  }

  // === IF NO CONFIG YET: DISCOVER AND FETCH ===

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

  // === STEP 3: Fetch config from discovered server ===
  // (Waits CONFIG_FETCH_RETRY_INTERVAL before first attempt to allow mDNS discovery)

  if (now - last_config_fetch_attempt >= CONFIG_FETCH_RETRY_INTERVAL)
  {
    last_config_fetch_attempt = now;

    const DiscoveredConfig* discovered = getDiscoveredConfig();
    if (discovered && discovered->valid)
    {
      DEBUG_PRINTLN(F(""));
      DEBUG_PRINT(F("→ Attempting to fetch config from: "));
      DEBUG_PRINT(discovered->ipStr);
      DEBUG_PRINT(F(":"));
      DEBUG_PRINTLN(discovered->port);

      // Fetch configuration from server
      ConfigResponse response = fetchConfigFromServer(
          discovered->ipStr,
          discovered->port,
          &device
      );

      if (response.success)
      {
        // Parse the JSON configuration
        mqtt_config = parseConfigJSON(response.config_json);
        config_fetched = true;

        DEBUG_PRINTLN(F(""));
        DEBUG_PRINTLN(F("=== CONFIGURATION SUCCESSFULLY RETRIEVED ==="));
        DEBUG_PRINT(F("MQTT Broker: "));
        DEBUG_PRINTLN(mqtt_config.mqtt_broker);
        DEBUG_PRINT(F("MQTT Port: "));
        DEBUG_PRINTLN(mqtt_config.mqtt_port);
        DEBUG_PRINT(F("MQTT Topic: "));
        DEBUG_PRINTLN(mqtt_config.mqtt_topic);
        DEBUG_PRINT(F("Poll Interval: "));
        DEBUG_PRINT(mqtt_config.poll_frequency_sec);
        DEBUG_PRINTLN(F(" seconds"));
        DEBUG_PRINTLN(F(""));

        // Initialize MQTT connection
        if (initMQTT(&mqtt_config))
        {
          mqtt_initialized = true;
          DEBUG_PRINTLN(F("✓ MQTT module initialized"));
          DEBUG_PRINTLN(F("✓ Switching to MQTT publishing mode..."));
        }
        else
        {
          DEBUG_PRINTLN(F("✗ Failed to initialize MQTT"));
        }
      }
      else
      {
        DEBUG_PRINT(F("✗ Failed to fetch config: "));
        DEBUG_PRINTLN(response.error_msg);
      }
    }
    else
    {
      DEBUG_PRINTLN(F("⚠ No valid server discovered yet..."));
    }
  }
}
