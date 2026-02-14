#include <Arduino.h>
#include "mqtt/mqtt_publish.h"
#include "arduino_configs.h"
#include <ArduinoMqttClient.h>
#include <WiFiNINA.h>

// ============================================================================
// STATIC STATE - MQTT Connection and Status
// ============================================================================

// Use WiFiClient for non-TLS connections (port 1883)
// For TLS connections (port 8883), certificate validation is required
static WiFiClient wifiClient;
static MqttClient mqttClient(wifiClient);
static MQTTStatus mqtt_status = MQTT_DISCONNECTED;
static MQTTConfig mqtt_config_copy;
static bool mqtt_initialized = false;

// ============================================================================
// MQTT CONNECTION MANAGEMENT
// ============================================================================

/**
 * Initialize MQTT connection with broker
 */
MQTTStatus initMQTT(const MQTTConfig* config)
{
  if (!config || config->mqtt_broker[0] == '\0')
  {
    DEBUG_PRINTLN(F("✗ Invalid MQTT config"));
    mqtt_status = MQTT_ERROR;
    return MQTT_ERROR;
  }

  // Store configuration
  memcpy(&mqtt_config_copy, config, sizeof(MQTTConfig));

  DEBUG_PRINTLN(F(""));
  DEBUG_PRINTLN(F("=== MQTT INITIALIZATION ==="));
  DEBUG_PRINT(F("→ Broker: "));
  DEBUG_PRINT(mqtt_config_copy.mqtt_broker);
  DEBUG_PRINT(F(":"));
  DEBUG_PRINTLN(mqtt_config_copy.mqtt_port);
  DEBUG_PRINT(F("→ Topic: "));
  DEBUG_PRINTLN(mqtt_config_copy.mqtt_topic);

  // Set broker and port
  mqttClient.setId(F("arduino-mdns-query"));
  mqttClient.setUsernamePassword(nullptr, nullptr);

  // Port selection guidance
  DEBUG_PRINTLN(F(""));
  if (mqtt_config_copy.mqtt_port == 8883)
  {
    DEBUG_PRINTLN(F("  ⚠ Port 8883 detected (TLS/SSL required)"));
    DEBUG_PRINTLN(F("  For testing without TLS, modify config to use port 1883"));
  }
  else if (mqtt_config_copy.mqtt_port == 1883)
  {
    DEBUG_PRINTLN(F("  ✓ Port 1883 detected (non-TLS, standard MQTT)"));
  }

  mqtt_status = MQTT_CONNECTING;
  mqtt_initialized = true;

  DEBUG_PRINTLN(F("✓ MQTT initialized and ready to connect"));
  return MQTT_CONNECTING;
}

/**
 * Maintain MQTT connection - must be called in loop
 */
MQTTStatus maintainMQTT()
{
  if (!mqtt_initialized)
  {
    return MQTT_DISCONNECTED;
  }

  // Try to connect if disconnected
  if (mqtt_status == MQTT_CONNECTING || mqtt_status == MQTT_DISCONNECTED)
  {
    if (!mqttClient.connected())
    {
      // Attempt connection with debug output
      DEBUG_PRINT(F("→ Connecting to MQTT broker: "));
      DEBUG_PRINT(mqtt_config_copy.mqtt_broker);
      DEBUG_PRINT(F(":"));
      DEBUG_PRINTLN(mqtt_config_copy.mqtt_port);

      uint16_t port_to_try = mqtt_config_copy.mqtt_port;

      if (!mqttClient.connect(
          mqtt_config_copy.mqtt_broker,
          port_to_try))
      {
        // Fallback: If configured for TLS port (8883), try non-TLS port (1883)
        if (port_to_try == 8883)
        {
          DEBUG_PRINTLN(F("  → Port 8883 failed (requires TLS)"));
          DEBUG_PRINTLN(F("  → Trying fallback port 1883 (non-TLS)..."));

          port_to_try = 1883;
          if (!mqttClient.connect(
              mqtt_config_copy.mqtt_broker,
              port_to_try))
          {
            mqtt_status = MQTT_DISCONNECTED;
            DEBUG_PRINTLN(F("✗ Connection failed on both ports"));
            return mqtt_status;
          }

          DEBUG_PRINTLN(F("✓ Connected on fallback port 1883 (non-TLS)"));
        }
        else
        {
          mqtt_status = MQTT_DISCONNECTED;
          DEBUG_PRINTLN(F("✗ MQTT connection failed"));
          return mqtt_status;
        }
      }

      mqtt_status = MQTT_CONNECTED;
      if (port_to_try == mqtt_config_copy.mqtt_port)
      {
        DEBUG_PRINTLN(F("✓ Connected to MQTT broker"));
      }
    }
  }

  // Poll for MQTT messages (maintains connection)
  if (mqttClient.connected())
  {
    mqttClient.poll();
    mqtt_status = MQTT_CONNECTED;
  }
  else
  {
    if (mqtt_status == MQTT_CONNECTED)
    {
      DEBUG_PRINTLN(F("✗ MQTT connection lost"));
    }
    mqtt_status = MQTT_DISCONNECTED;
  }

  return mqtt_status;
}

/**
 * Publish message to MQTT
 */
MQTTStatus publishToMQTT(const char* topic, const char* message)
{
  if (!message)
  {
    DEBUG_PRINTLN(F("✗ Empty message"));
    return MQTT_ERROR;
  }

  if (!isMQTTReady())
  {
    DEBUG_PRINTLN(F("✗ MQTT not connected"));
    return mqtt_status;
  }

  // Use configured topic if not specified
  const char* publish_topic = topic ? topic : mqtt_config_copy.mqtt_topic;

  if (!publish_topic || publish_topic[0] == '\0')
  {
    DEBUG_PRINTLN(F("✗ No topic specified"));
    return MQTT_ERROR;
  }

  // Publish message
  DEBUG_PRINT(F("→ Publishing to: "));
  DEBUG_PRINTLN(publish_topic);
  DEBUG_PRINT(F("  Message: "));
  DEBUG_PRINTLN(message);

  // Use String to avoid ambiguous overload
  String topic_str = String(publish_topic);
  if (mqttClient.beginMessage(topic_str))
  {
    mqttClient.print(message);
    if (!mqttClient.endMessage())
    {
      DEBUG_PRINTLN(F("✗ Failed to publish"));
      mqtt_status = MQTT_ERROR;
      return MQTT_ERROR;
    }
  }
  else
  {
    DEBUG_PRINTLN(F("✗ Failed to begin message"));
    mqtt_status = MQTT_ERROR;
    return MQTT_ERROR;
  }

  DEBUG_PRINTLN(F("✓ Message published"));
  return MQTT_CONNECTED;
}

/**
 * Get current MQTT status
 */
MQTTStatus getMQTTStatus()
{
  return mqtt_status;
}

/**
 * Check if MQTT is ready
 */
bool isMQTTReady()
{
  return mqtt_initialized && mqtt_status == MQTT_CONNECTED && mqttClient.connected();
}

/**
 * Disconnect from MQTT
 */
MQTTStatus disconnectMQTT()
{
  if (mqttClient.connected())
  {
    mqttClient.stop();
    DEBUG_PRINTLN(F("✓ Disconnected from MQTT"));
  }
  mqtt_status = MQTT_DISCONNECTED;
  return MQTT_DISCONNECTED;
}
