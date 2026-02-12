#include <Arduino.h>
#include "config_fetch/config_fetch.h"
#include "arduino_configs.h"
#include <WiFiNINA.h>
#include <ArduinoJson.h>

/**
 * Fetch configuration from HTTP server
 * Constructs GET request: GET /config?device_id=<serial>&mac=<mac>
 */
ConfigResponse fetchConfigFromServer(
    const char *host,
    uint16_t port,
    const DeviceID *device_id)
{
  ConfigResponse response;
  response.success = false;
  response.http_code = 0;

  if (!device_id || !device_id->valid)
  {
    snprintf(response.error_msg, sizeof(response.error_msg),
             "Invalid device ID");
    return response;
  }

  DEBUG_PRINT(F("→ Connecting to: "));
  DEBUG_PRINT(host);
  DEBUG_PRINT(F(":"));
  DEBUG_PRINTLN(port);

  // Create WiFi client
  WiFiClient client;

  // Connect to server
  if (!client.connect(host, port))
  {
    DEBUG_PRINTLN(F("✗ Connection failed"));
    snprintf(response.error_msg, sizeof(response.error_msg),
             "Failed to connect to %s:%u", host, port);
    return response;
  }

  DEBUG_PRINTLN(F("✓ Connected"));

  // Build request URL with parameters
  char request_url[512];
  snprintf(request_url, sizeof(request_url),
           "/config?device_id=%s&mac=%s",
           device_id->device_id,
           device_id->mac_address);

  // Debug: Print full HTTP GET request URL
  #if DEBUG
  char full_url[768];
  snprintf(full_url, sizeof(full_url),
           "GET http://%s:%u%s HTTP/1.1",
           host,
           port,
           request_url);
  DEBUG_PRINT(F("→ Sending: "));
  DEBUG_PRINTLN(full_url);
  #endif

  // Send HTTP GET request
  client.print(F("GET "));
  client.print(request_url);
  client.println(F(" HTTP/1.1"));
  client.print(F("Host: "));
  client.print(host);
  client.print(F(":"));
  client.println(port);
  client.println(F("User-Agent: Arduino/1.0"));
  client.println(F("Connection: close"));
  client.println();

  // Wait for response
  unsigned long timeout = millis();
  while (!client.available() && millis() - timeout < 5000)
  {
    delay(10);
  }

  if (!client.available())
  {
    DEBUG_PRINTLN(F("✗ No response from server"));
    snprintf(response.error_msg, sizeof(response.error_msg),
             "Server timeout");
    client.stop();
    return response;
  }

  // Read response header and extract HTTP status code
  String status_line = client.readStringUntil('\n');
  response.http_code = 0;
  sscanf(status_line.c_str(), "HTTP/1.%*d %d", &response.http_code);

  DEBUG_PRINT(F("✓ HTTP Response: "));
  DEBUG_PRINTLN(response.http_code);

  // Skip headers until empty line
  while (client.connected())
  {
    String header = client.readStringUntil('\n');
    if (header == "\r")
      break;
  }

  // Read response body
  size_t body_index = 0;
  while (client.connected() && client.available())
  {
    char c = client.read();
    if (body_index < sizeof(response.config_json) - 1)
    {
      response.config_json[body_index++] = c;
    }
  }
  response.config_json[body_index] = '\0';

  client.stop();

  // Check for success
  if (response.http_code == 200)
  {
    response.success = true;
    DEBUG_PRINT(F("✓ Configuration retrieved ("));
    DEBUG_PRINT(body_index);
    DEBUG_PRINTLN(F(" bytes)"));
  }
  else
  {
    snprintf(response.error_msg, sizeof(response.error_msg),
             "HTTP %d", response.http_code);
    DEBUG_PRINT(F("✗ Server returned HTTP "));
    DEBUG_PRINTLN(response.http_code);
  }

  return response;
}

/**
 * Parse configuration JSON
 * Uses ArduinoJson library (included in PlatformIO)
 */
MQTTConfig parseConfigJSON(const char *json_response)
{
  MQTTConfig mqtt_config;
  memset(&mqtt_config, 0, sizeof(mqtt_config));

  // Allocate JSON document
  StaticJsonDocument<2048> doc;

  // Parse JSON
  DeserializationError error = deserializeJson(doc, json_response);
  if (error)
  {
    DEBUG_PRINT(F("✗ JSON parse error: "));
    DEBUG_PRINTLN(error.c_str());
    return mqtt_config;
  }

  // Extract config section
  JsonObject config = doc["config"];
  if (config.isNull())
  {
    DEBUG_PRINTLN(F("✗ Missing 'config' section in response"));
    return mqtt_config;
  }

  // Extract MQTT settings
  if (config.containsKey("mqtt_broker"))
  {
    strlcpy(mqtt_config.mqtt_broker,
            config["mqtt_broker"].as<const char *>(),
            sizeof(mqtt_config.mqtt_broker));
  }

  if (config.containsKey("mqtt_port"))
  {
    mqtt_config.mqtt_port = config["mqtt_port"].as<uint16_t>();
  }

  if (config.containsKey("mqtt_topic"))
  {
    strlcpy(mqtt_config.mqtt_topic,
            config["mqtt_topic"].as<const char *>(),
            sizeof(mqtt_config.mqtt_topic));
  }

  if (config.containsKey("poll_frequency_sec"))
  {
    mqtt_config.poll_frequency_sec =
        config["poll_frequency_sec"].as<uint16_t>();
  }

  if (config.containsKey("heartbeat_frequency_sec"))
  {
    mqtt_config.heartbeat_frequency_sec =
        config["heartbeat_frequency_sec"].as<uint16_t>();
  }

  if (config.containsKey("template"))
  {
    strlcpy(mqtt_config.template_name,
            config["template"].as<const char *>(),
            sizeof(mqtt_config.template_name));
  }

  DEBUG_PRINTLN(F("✓ Configuration parsed successfully"));
  DEBUG_PRINT(F("  MQTT Broker: "));
  DEBUG_PRINTLN(mqtt_config.mqtt_broker);
  DEBUG_PRINT(F("  MQTT Port: "));
  DEBUG_PRINTLN(mqtt_config.mqtt_port);
  DEBUG_PRINT(F("  Topic: "));
  DEBUG_PRINTLN(mqtt_config.mqtt_topic);

  return mqtt_config;
}
