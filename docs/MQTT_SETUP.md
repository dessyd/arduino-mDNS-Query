# MQTT Publishing Setup Guide

## Overview

The Arduino MKR WiFi 1010 now publishes real environmental sensor data to an MQTT broker after:

1. Discovering the config server via mDNS
2. Fetching MQTT broker details and publish interval from the config server
3. Connecting to the MQTT broker
4. Publishing real sensor readings at the configurable interval received from the config server

## Configuration Flow

```text
mDNS Discovery (10s interval)
    ↓
Config Server HTTP Fetch (30s wait)
    ↓
MQTT Initialization
    ↓
MQTT Publishing (interval as received from Config Server)
```

## MQTT Broker Configuration

### Port Options

#### Option A: Non-TLS (Port 1883) - Recommended for Testing

- **Advantages**: No certificate validation needed, simple setup
- **Use case**: Development, testing, internal networks
- **Setup**: Ensure your MQTT broker listens on port 1883

#### Option B: TLS/SSL (Port 8883) - For Production

- **Advantages**: Secure, encrypted communication
- **Requirements**: Proper certificate validation (not yet implemented)
- **Use case**: Production deployments, cloud MQTT services

### Port Fallback Logic

The code automatically tries both ports:

```text
1. Attempt connection on configured port (default: 8883)
2. If TLS port fails, try fallback to port 1883
3. If both fail, retry on next loop cycle
```

## Testing Setup

### Step 1: Verify mDNS Server is Running

```bash
# Your config server should advertise _config._tcp.local
# Test with: avahi-browse -r _config._tcp.local
```

### Step 2: Configure MQTT Broker

**Using Mosquitto (example):**

```bash
# Install
brew install mosquitto

# Edit config to listen on port 1883
# /usr/local/etc/mosquitto/mosquitto.conf
# listener 1883
# protocol mqtt

# Start broker
mosquitto -c /usr/local/etc/mosquitto/mosquitto.conf
```

**Using Docker:**

```bash
docker run -d \
  --name mosquitto \
  -p 1883:1883 \
  eclipse-mosquitto
```

### Step 3: Subscribe to Topic (Monitor Messages)

```bash
mosquitto_sub -h 192.168.2.50 -p 1883 \
  -t "home/devices/config_client_A1B2C3/updated"
```

### Step 4: Run the Arduino

1. Upload the sketch to MKR WiFi 1010
2. Monitor serial output at 115200 baud
3. Wait ~30 seconds for config fetch
4. Watch for MQTT connection messages
5. See published payloads in mosquitto_sub

## Sample Output Sequence

```text
✓ WiFi connected! IP: 192.168.1.123
✓ mDNS initialized...
→ Attempting to fetch config from: 192.168.1.21:5050
✓ Configuration retrieved (329 bytes)
=== CONFIGURATION SUCCESSFULLY RETRIEVED ===
MQTT Broker: 192.168.2.50
MQTT Port: 8883
MQTT Topic: home/devices/config_client_A1B2C3/updated
Poll Interval: 5 seconds

=== MQTT INITIALIZATION ===
→ Port 8883 detected (TLS/SSL required)
→ Connecting to MQTT broker: 192.168.2.50:8883
  → Port 8883 failed (requires TLS)
  → Trying fallback port 1883 (non-TLS)...
✓ Connected to MQTT broker
→ Publishing to: home/devices/config_client_A1B2C3/updated
  Message: {"temperature_celsius":23.5,"humidity_percent":45.2,"pressure_millibar":1013.25,"illuminance_lux":542,"uv_index":2.1,"timestamp":45000}
✓ Message published
```

## Sample Published Data

The Arduino publishes this JSON payload at the interval configured by the config server:

```json
{
  "temperature_celsius": 23.5,
  "humidity_percent": 45.2,
  "pressure_millibar": 1013.25,
  "illuminance_lux": 542,
  "uv_index": 2.1,
  "timestamp": 45000
}
```

**Fields (only valid sensor fields are included):**

- `temperature_celsius`: Real sensor reading from HTS221 temperature sensor
- `humidity_percent`: Real sensor reading from HTS221 humidity sensor (0-100%)
- `pressure_millibar`: Real sensor reading from LPS22HB pressure sensor
- `illuminance_lux`: Real sensor reading from TEMT6000 light sensor
- `uv_index`: Real sensor reading from UV sensor (Rev1 boards only; not available on Rev2)
- `timestamp`: Unix timestamp (from RTC if available, or relative milliseconds)

## Troubleshooting

### Issue: "MQTT connection failed"

**Cause**: Broker not reachable on either port

**Solutions**:

1. Check broker is running: `netstat -an | grep 1883`
2. Verify firewall allows port 1883
3. Confirm broker IP address (should be 192.168.2.50 from config)
4. Check WiFi connectivity: Monitor serial for WiFi status

### Issue: "Port 8883 detected (TLS/SSL required)"

**This is normal!** The code detects TLS port and automatically falls back to 1883.

**Options**:

1. Use port 1883 broker (easiest for testing)
2. Modify config server to return port 1883
3. Implement proper TLS certificates (advanced)

### Issue: Data not appearing in mosquitto_sub

1. Verify correct topic: `home/devices/config_client_A1B2C3/updated`
2. Check MQTT logs: `sudo tail -f /var/log/mosquitto/mosquitto.log`
3. Monitor Arduino serial output for publish messages
4. Verify broker IP/port in config server response

## MQTT API Reference

### Connection Status Enum

All MQTT functions return a consistent `MQTTStatus` enum that represents the actual connection state:

```cpp
typedef enum {
  MQTT_DISCONNECTED = 0,
  MQTT_CONNECTING = 1,
  MQTT_CONNECTED = 2,
  MQTT_ERROR = 3
} MQTTStatus;
```

### Function API

#### Initialize MQTT Connection

```cpp
MQTTStatus initMQTT(const MQTTConfig* config);
```

- **Parameters**: Configuration struct with broker, port, and topic
- **Returns**: `MQTT_CONNECTING` if successful, `MQTT_ERROR` if failed
- **Must be called once** after config is fetched

#### Maintain Connection

```cpp
MQTTStatus maintainMQTT();
```

- **Returns**: Current connection status
- **Must be called regularly in loop** to maintain connection and poll for messages
- **Side effects**: Updates global `mqtt_status` based on actual socket state

#### Publish Message

```cpp
MQTTStatus publishToMQTT(const char* topic, const char* message);
```

- **Parameters**:
  - `topic`: MQTT topic (or `NULL` to use configured topic)
  - `message`: JSON or plain text payload
- **Returns**: `MQTT_CONNECTED` if sent successfully, `MQTT_ERROR` if failed
- **Use case**: Call only when `isMQTTReady()` returns true

#### Query Connection Status

```cpp
MQTTStatus getMQTTStatus();
```

- **Returns**: Current MQTT status enum
- **Thread-safe**: Reading global state

#### Check Ready State

```cpp
bool isMQTTReady();
```

- **Returns**: `true` if initialized, connected, and socket is active
- **Use before**: Calling `publishToMQTT()`

#### Disconnect Gracefully

```cpp
MQTTStatus disconnectMQTT();
```

- **Returns**: `MQTT_DISCONNECTED`
- **Side effects**: Closes socket and updates `mqtt_status`

### Status-Based Error Handling

The refactored API uses consistent `MQTTStatus` returns instead of mixing bool/status:

**Before (inconsistent):**

```cpp
if (initMQTT(&config)) {  // bool return confusing
  // config was invalid OR connection attempted?
}
if (!publishToMQTT(topic, msg)) {  // bool return, modify global on error
  // success or failure? what's the state?
}
```

**After (consistent):**

```cpp
MQTTStatus status = initMQTT(&config);
if (status != MQTT_ERROR) {  // clear intent: initialization failed
  mqtt_initialized = true;
}

status = publishToMQTT(topic, msg);
if (status == MQTT_ERROR) {  // clear: publish failed, return state
  DEBUG_PRINTLN(F("Publish failed"));
}
```

## Next Steps

### To Change the Publish Interval

The publish interval is now controlled by the config server's `poll_frequency_sec` parameter. Modify your config server response to set the desired interval:

**Example config server response:**

```json
{
  "mqtt_broker": "192.168.2.50",
  "mqtt_port": 1883,
  "mqtt_topic": "home/devices/config_client_A1B2C3/updated",
  "poll_frequency_sec": 5
}
```

The Arduino will publish data every 5 seconds (or whatever value you set).

### To Add MQTT Authentication

Add to `src/mqtt/mqtt_publish.cpp` in `initMQTT()`:

```cpp
mqttClient.setUsernamePassword("username", "password");
```

### To Customize Sensor Data

The sensor readings are automatically published by `readSensors()` in `src/sensors/sensors.cpp`. To add custom fields or modify the JSON format, edit `formatSensorJSON()` in the same file.

## Production Checklist

- [ ] TLS certificates configured for port 8883
- [ ] MQTT authentication (username/password) enabled
- [ ] Real sensor data integrated
- [ ] Error handling for connection failures
- [ ] Memory usage monitored (currently ~19% RAM)
- [ ] Flash usage within limits (~25% currently)
- [ ] Network resilience tested (WiFi dropouts, broker restarts)
