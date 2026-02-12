#include <Arduino.h>
#include "mqtt/mqtt_publish.h"
#include "arduino_configs.h"
#include <ArduinoMqttClient.h>
#include <WiFiNINA.h>

// ============================================================================
// STATIC STATE - MQTT Connection and Status
// ============================================================================

// Use WiFiSSLClient for TLS/SSL connections (port 8883)
static WiFiSSLClient wifiClient;
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
bool initMQTT(const MQTTConfig* config)
{
  if (!config || config->mqtt_broker[0] == '\0')
  {
    DEBUG_PRINTLN(F("✗ Invalid MQTT config"));
    mqtt_status = MQTT_ERROR;
    return false;
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

  // Note: WiFiSSLClient requires proper certificate setup for port 8883
  // For testing, consider using port 1883 (non-TLS) if your broker supports it

  mqtt_status = MQTT_CONNECTING;
  mqtt_initialized = true;

  DEBUG_PRINTLN(F("✓ MQTT initialized and ready to connect"));
  DEBUG_PRINTLN(F("  Note: Port 8883 requires TLS certificate validation"));
  return true;
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

      if (!mqttClient.connect(
          mqtt_config_copy.mqtt_broker,
          mqtt_config_copy.mqtt_port))
      {
        mqtt_status = MQTT_DISCONNECTED;
        DEBUG_PRINTLN(F("✗ MQTT connection failed"));
        return mqtt_status;
      }

      mqtt_status = MQTT_CONNECTED;
      DEBUG_PRINTLN(F("✓ Connected to MQTT broker"));
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
bool publishToMQTT(const char* topic, const char* message)
{
  if (!message)
  {
    DEBUG_PRINTLN(F("✗ Empty message"));
    return false;
  }

  if (!isMQTTReady())
  {
    DEBUG_PRINTLN(F("✗ MQTT not connected"));
    return false;
  }

  // Use configured topic if not specified
  const char* publish_topic = topic ? topic : mqtt_config_copy.mqtt_topic;

  if (!publish_topic || publish_topic[0] == '\0')
  {
    DEBUG_PRINTLN(F("✗ No topic specified"));
    return false;
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
      return false;
    }
  }
  else
  {
    DEBUG_PRINTLN(F("✗ Failed to begin message"));
    mqtt_status = MQTT_ERROR;
    return false;
  }

  DEBUG_PRINTLN(F("✓ Message published"));
  return true;
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
void disconnectMQTT()
{
  if (mqttClient.connected())
  {
    mqttClient.stop();
    DEBUG_PRINTLN(F("✓ Disconnected from MQTT"));
  }
  mqtt_status = MQTT_DISCONNECTED;
}
