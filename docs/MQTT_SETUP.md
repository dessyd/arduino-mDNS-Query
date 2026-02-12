# MQTT Publishing Setup Guide

## Overview

The Arduino MKR WiFi 1010 now publishes sensor data to an MQTT broker after:
1. Discovering the config server via mDNS
2. Fetching MQTT broker details from the config server
3. Connecting to the MQTT broker
4. Publishing dummy JSON payloads every 10 seconds

## Configuration Flow

```
mDNS Discovery (10s interval)
    ↓
Config Server HTTP Fetch (30s wait)
    ↓
MQTT Initialization
    ↓
MQTT Publishing (10s interval)
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
```
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

```
✓ WiFi connected! IP: 192.168.1.123
✓ mDNS initialized...
→ Attempting to fetch config from: 192.168.1.21:5050
✓ Configuration retrieved (329 bytes)
=== CONFIGURATION SUCCESSFULLY RETRIEVED ===
MQTT Broker: 192.168.2.50
MQTT Port: 8883

=== MQTT INITIALIZATION ===
→ Port 8883 detected (TLS/SSL required)
→ Connecting to MQTT broker: 192.168.2.50:8883
  → Port 8883 failed (requires TLS)
  → Trying fallback port 1883 (non-TLS)...
✓ Connected to MQTT broker
→ Publishing to: home/devices/config_client_A1B2C3/updated
  Message: {"device_id":"0123B4364F467BF2","mac":"A8:8F:AD:C4:0A:24","timestamp":45000,"temperature":23.5,"humidity":45}
✓ Message published
```

## Sample Published Data

The Arduino publishes this JSON payload every 10 seconds:

```json
{
  "device_id": "0123B4364F467BF2EE",
  "mac": "A8:8F:AD:C4:0A:24",
  "timestamp": 45000,
  "temperature": 23.5,
  "humidity": 45
}
```

**Fields:**
- `device_id`: ATECC608A serial number (9 bytes)
- `mac`: WiFi MAC address
- `timestamp`: Seconds since device boot
- `temperature`: Dummy sensor value (23.5°C)
- `humidity`: Dummy sensor value (45%)

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

## Next Steps

### To Use Real Sensor Data
Edit `src/main.cpp` in the MQTT publishing section:
```cpp
// Replace dummy values with real sensor readings
snprintf(payload, sizeof(payload),
         "{\"device_id\":\"%s\",\"temperature\":%.1f}",
         device.device_id,
         readTemperature());  // Add your sensor function
```

### To Add Authentication
Add to `src/mqtt/mqtt_publish.cpp` in `initMQTT()`:
```cpp
mqttClient.setUsernamePassword("username", "password");
```

### To Add Heartbeat Messages
Modify `PUBLISH_INTERVAL` in `src/main.cpp`:
```cpp
static const uint32_t PUBLISH_INTERVAL = 5000;  // Every 5 seconds
```

## Production Checklist

- [ ] TLS certificates configured for port 8883
- [ ] MQTT authentication (username/password) enabled
- [ ] Real sensor data integrated
- [ ] Error handling for connection failures
- [ ] Memory usage monitored (currently ~19% RAM)
- [ ] Flash usage within limits (~25% currently)
- [ ] Network resilience tested (WiFi dropouts, broker restarts)
