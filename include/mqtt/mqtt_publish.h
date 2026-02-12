/**
 * ============================================================================
 * MQTT Publishing Module Header
 * ============================================================================
 * Handles connection and publishing to MQTT broker
 */

#ifndef MQTT_PUBLISH_H
#define MQTT_PUBLISH_H

#include <Arduino.h>
#include "config_fetch/config_fetch.h"

/**
 * MQTT Connection Status
 */
typedef enum {
  MQTT_DISCONNECTED = 0,
  MQTT_CONNECTING = 1,
  MQTT_CONNECTED = 2,
  MQTT_ERROR = 3
} MQTTStatus;

/**
 * Initialize MQTT connection
 * Must be called after config is fetched
 *
 * Parameters:
 *   - config: MQTT configuration from config server
 *
 * Returns:
 *   true if connection initiated successfully
 */
bool initMQTT(const MQTTConfig* config);

/**
 * Maintain MQTT connection and handle network events
 * Must be called regularly in loop
 *
 * Returns:
 *   Current MQTT connection status
 */
MQTTStatus maintainMQTT();

/**
 * Publish message to MQTT topic
 *
 * Parameters:
 *   - topic: MQTT topic (or NULL to use configured topic)
 *   - message: JSON or plain text message
 *
 * Returns:
 *   true if message sent successfully
 */
bool publishToMQTT(const char* topic, const char* message);

/**
 * Get current MQTT connection status
 *
 * Returns:
 *   Current status (CONNECTED, DISCONNECTED, etc.)
 */
MQTTStatus getMQTTStatus();

/**
 * Check if MQTT is ready for publishing
 *
 * Returns:
 *   true if connected and ready
 */
bool isMQTTReady();

/**
 * Disconnect from MQTT broker
 */
void disconnectMQTT();

#endif  // MQTT_PUBLISH_H
