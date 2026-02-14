# System Architecture

## Overview

`arduino-mDNS-Query` is a modular IoT firmware system that combines environmental monitoring with network service discovery and cloud telemetry. The architecture follows a layered design pattern with clear separation of concerns.

```text
┌─────────────────────────────────────────────────────┐
│                   Main Program Flow                  │
│              (setup/loop/event handlers)             │
└──────────────┬──────────────────────────────────────┘
               │
    ┌──────────┼──────────┬──────────┬──────────────┐
    │          │          │          │              │
    v          v          v          v              v
┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐  ┌──────────┐
│Sensors │ │Network │ │  mDNS  │ │ MQTT   │  │   RTC    │
│        │ │        │ │ Query  │ │ Broker │  │          │
└────────┘ └────────┘ └────────┘ └────────┘  └──────────┘
    │
    └──────────────────────────────────────────────────┐
                   (WiFi & UDP packets)
                             │
                             v
                    ┌────────────────┐
                    │  WiFiNINA I/O  │
                    └────────────────┘
```text

## Module Breakdown

### 1. Sensor Module (`sensors/sensors.h/cpp`)

**Purpose**: Environmental sensor acquisition and JSON formatting

**Responsibilities**:

- Initialize MKR ENV Shield (I2C interface to HTS221, LPS22HB, TEMT6000)
- Poll individual sensors with error handling
- Track per-sensor validity flags
- Format readings with explicit units in JSON

**Key Data Structures**:

```cpp
typedef struct {
  float temperature;        // °C (CELSIUS unit)
  float humidity;          // % RH (HUMIDITY percent)
  float pressure;          // millibar (MILLIBAR unit)
  float illuminance;       // lux (LUX unit)
  float uv_index;          // UV Index (-1.0 if unavailable)
  uint32_t timestamp;

  bool valid;              // At least one sensor working
  bool temp_valid;         // Individual per-sensor flags
  bool humidity_valid;
  bool pressure_valid;
  bool light_valid;
  bool uv_valid;
} SensorReadings;
```text

**JSON Output Format**:

```json
{
  "temperature_celsius": 24.5,
  "humidity_percent": 52.3,
  "pressure_millibar": 1013.2,
  "illuminance_lux": 450.5,
  "uv_index": 2.1,
  "timestamp": 12345
}
```text

**Unit Decisions**:

- **Temperature**: Celsius (ENV library standard)
- **Pressure**: Millibar (kPa converted via MILLIBAR constant)
- **Illuminance**: Lux (standard SI unit for light)
- **Humidity**: Percent (0-100% RH)

**Memory**: ~1.5 KB RAM, ~6 KB Flash

### 2. Network Module (`mdns/network.h/cpp`)

**Purpose**: WiFi and UDP socket initialization

**Responsibilities**:

- Initialize WiFiNINA module
- Connect to configured WiFi network with timeout
- Create UDP socket for mDNS communication
- Handle network status transitions

**Configuration**:

```cpp
#define CONFIG_WIFI_SSID SECRET_WIFI_SSID          // From secrets
#define CONFIG_WIFI_PASSWORD SECRET_WIFI_PASSWORD
#define CONFIG_WIFI_TIMEOUT_MS 10000               // 10s connection timeout
#define CONFIG_MDNS_MULTICAST_ADDR "224.0.0.251"
#define CONFIG_MDNS_PORT 5353
```text

**Memory**: ~8 KB (WiFiNINA overhead) + UDP socket buffers

### 3. mDNS Query Module (`mdns/mdns.h/cpp`, `mdns/packet.h/cpp`)

**Purpose**: RFC 6762 Multicast DNS service discovery

**Responsibilities**:

- Build DNS PTR query packets according to RFC 1035/6762
- Send queries to mDNS multicast group (224.0.0.251:5353)
- Parse PTR/SRV/TXT/A resource records from responses
- Extract hostname, port, and service metadata

**Query Flow**:

```text
1. Build PTR Query for "_http._tcp.local"
2. Send to 224.0.0.251:5353
3. Wait for responses (1-5 seconds typical)
4. Parse and extract:
   - PTR: Service instance name (SRV target)
   - SRV: Hostname + port
   - TXT: Key=value attributes (path, version)
   - A: Hostname IP address
5. Validate all required fields present
6. Return DiscoveredConfig
```text

**Data Structure**:

```cpp
typedef struct {
  char hostname[64];          // "config-server.local"
  uint16_t port;              // e.g., 8080
  char path[32];              // "/config" from TXT record
  char version[16];           // "1.0" from TXT record
  uint32_t ipAddress;         // IPv4 in network byte order
  char ipStr[16];             // "192.168.1.100"
  bool valid;
} DiscoveredConfig;
```text

**Memory**: ~2 KB RAM, ~6 KB Flash

### 4. Configuration Fetch Module (`config_fetch/config_fetch.h/cpp`)

**Purpose**: Retrieve MQTT settings from remote config server via HTTP

**Responsibilities**:

- Use mDNS discovery to locate config server
- Fetch JSON configuration via HTTP GET
- Parse MQTT broker hostname, port, topic prefix
- Store for MQTT initialization

**Flow**:

```text
1. Discover config server via mDNS (_http._tcp.local)
2. HTTP GET http://config-server:port/config
3. Parse JSON response:
   {
     "mqtt_host": "broker.example.com",
     "mqtt_port": 1883,
     "mqtt_topic_prefix": "devices/sensor123",
     "retain": true
   }
4. Store in MQTTConfig struct
5. Pass to initMQTT()
```text

**Retry Strategy**:

- Attempt config fetch every 30 seconds if failed
- Timeout after 5 seconds per attempt
- Continue running sensors even if config fetch fails

**Memory**: ~512 B + HTTP response buffer

### 5. MQTT Module (`mqtt/mqtt_publish.h/cpp`)

**Purpose**: Telemetry publishing to MQTT broker

**Responsibilities**:

- Initialize WiFi client connection to MQTT broker
- Handle connection lifecycle (connect/disconnect/reconnect)
- Publish sensor readings to configured topic
- Maintain connection polling in loop

**State Machine**:

```text
[DISCONNECTED]
    ↓ initMQTT()
[CONNECTING] → [CONNECTED] → [DISCONNECTED]
    ↓                            ↑
  timeout/error ─────────────────┘
```text

**Publishing**:

```text
Topic: devices/sensor123/telemetry
Payload: {"temperature_celsius":24.5,...,"timestamp":12345}
QoS: 1 (at-least-once)
Retain: true (broker stores last message)
```text

**Memory**: ~2 KB RAM, ~8 KB Flash

### 6. RTC Module (`rtc/rtc.h/cpp`)

**Purpose**: Real-time clock synchronization and timestamping

**Responsibilities**:

- Synchronize internal RTC with NTP via WiFi
- Provide accurate Unix timestamps for sensor readings
- Fall back to millis() if sync not available
- Track RTC validity state

**Timestamp Strategy**:

```text
If RTC synced:     Return getRTCTime()        (accurate)
Else:              Return millis() / 1000     (seconds since boot)
```text

**State Machine**: See [RTC_STATE_MACHINE.md](RTC_STATE_MACHINE.md)

**Memory**: ~256 B, ~2 KB Flash

### 7. Device ID Module (`device_id/device_id.h/cpp`)

**Purpose**: Unique device identification and naming

**Responsibilities**:

- Extract SAMD21 silicon serial number (128-bit)
- Generate compact device ID via XOR reduction
- Format device name for mDNS advertising

**Device Name Generation**:

```text
SAMD21 Unique IDs: [WORD0] [WORD1] [WORD2] [WORD3]
XOR Reduction:     ID = WORD0 ^ WORD1 ^ WORD2 ^ WORD3  (32-bit)
Device Name:       "MKR1010-XXXXXXXX"
```text

**Memory**: ~128 B

### 8. Main Program Flow (`src/main.cpp`)

**Initialization (setup)**:

```cpp
1. Serial.begin(115200)                    // Debug logging
2. initSensors()                           // I2C sensors
3. initRTC()                               // Real-time clock
4. WiFi.begin(SSID, password)              // Network connection
5. Broadcast device via mDNS
6. Create UDP socket for mDNS queries
```text

**Event Loop (loop)**:

```cpp
Every iteration:
  1. Check WiFi status → attempt reconnect if down
  2. Poll sensors every 10 seconds
  3. Send mDNS query every 60 seconds (if not yet found)
  4. Fetch remote config every 30 seconds (if not yet fetched)
  5. Publish to MQTT every 10 seconds (if connected)
  6. Handle incoming mDNS responses (non-blocking)
  7. Yield to WiFiNINA stack periodically
```text

**Timing Budget**:

| Operation | Duration | Frequency |
|-----------|----------|-----------|
| Sensor read | 12 ms | Every 10s |
| mDNS query send | <5 ms | Every 60s |
| mDNS response parse | 20-100 ms | Variable |
| MQTT publish | 100-200 ms | Every 10s |
| RTC sync | 500 ms | First time only |

**Memory**: ~512 B local + global state

## Data Flow Diagrams

### Sensor Reading & Telemetry

```text
┌──────────────┐
│ MKR ENV      │  I2C (100 kHz)
│ Shield       │
├──────────────┤
│ HTS221       │
│ LPS22HB      │
│ TEMT6000     │
└──────────────┘
       │
       │ Raw ADC values
       ↓
┌──────────────────────┐
│ readSensors()        │
│ - Poll each sensor   │
│ - Validate values    │
│ - Set validity flags │
└──────────────────────┘
       │
       │ SensorReadings
       ↓
┌──────────────────────┐
│ formatSensorJSON()   │
│ Output with units:   │
│ temperature_celsius  │
│ humidity_percent     │
│ pressure_millibar    │
│ illuminance_lux      │
└──────────────────────┘
       │
       │ JSON string
       ↓
┌──────────────────────┐
│ MQTT Broker          │
│ Topic: devices/...   │
│ /telemetry           │
└──────────────────────┘
```text

### Configuration Discovery

```text
┌──────────────────────┐
│ Start-up             │
│ Send mDNS PTR query  │
│ "_http._tcp.local"   │
└──────────────────────┘
       │
       │ Query packet
       │ to 224.0.0.251:5353
       ↓
┌──────────────────────┐
│ Config Server       │
│ (mDNS responder)     │
└──────────────────────┘
       │
       │ PTR/SRV/TXT/A records
       │ (multicast response)
       ↓
┌──────────────────────┐
│ parseMDNSResponse()  │
│ Extract:             │
│ - hostname           │
│ - port               │
│ - path               │
│ - version            │
│ - IP address         │
└──────────────────────┘
       │
       │ DiscoveredConfig
       ↓
┌──────────────────────┐
│ HTTP GET             │
│ /config              │
│ fetchMQTTConfig()    │
└──────────────────────┘
       │
       │ JSON config
       ↓
┌──────────────────────┐
│ MQTTConfig           │
│ Store broker details │
└──────────────────────┘
```text

## Design Decisions

### 1. Unit Representation Strategy

**Decision**: Explicit unit suffixes in JSON field names

**Rationale**:

- Eliminates ambiguity (is pressure in Pa, kPa, or hPa?)
- Self-documenting for API consumers
- Prevents unit conversion bugs in remote systems
- Complies with ISO 80000 standard naming

**Example**:

```json
// ✓ Clear
{"temperature_celsius": 24.5, "pressure_millibar": 1013.2}

// ✗ Ambiguous
{"temperature": 24.5, "pressure": 1013.2}
```text

### 2. Validity Flags per Sensor

**Decision**: Individual `bool` validity flags for each sensor

**Rationale**:

- Graceful degradation when partial sensors fail
- Consumers know which fields are valid
- Reduces false sensor readings affecting other systems
- Example: Pressure sensor fails but temp/humidity still work

**Data Structure**:

```cpp
struct {
  float temperature;
  bool temp_valid;      // Can be false independently

  float pressure;
  bool pressure_valid;  // Can be false independently
}
```text

### 3. mDNS over Hard-Coded IPs

**Decision**: Service discovery via mDNS PTR queries instead of static configs

**Rationale**:

- Dynamic network environments (device IPs change)
- No manual configuration required
- Standard RFC 6762 protocol (interoperable)
- Supports multiple config servers in network
- Self-documenting infrastructure

**Trade-off**: Slightly higher startup latency (1-5 seconds for discovery)

### 4. JSON over Binary Protocol

**Decision**: Human-readable JSON in debug mode, binary telemetry possible in production

**Rationale**:

- Debug mode: Easy to inspect serial output and understand data
- Human-readable for troubleshooting
- Standard interchange format (all platforms support JSON)
- Bandwidth cost acceptable for 10-second intervals

### 5. Remote Configuration Fetch

**Decision**: Pull configuration from mDNS-discovered HTTP server

**Rationale**:

- Centralized MQTT broker location management
- No hardcoded server addresses in firmware
- Easy to change broker without reflashing
- Update broker details without device downtime

**Trade-off**: Requires config server on network (mitigated by fallback to defaults)

### 6. RTC Synchronization Strategy

**Decision**: Optional RTC sync with millis() fallback

**Rationale**:

- Accurate timestamps when available
- Graceful degradation if NTP unavailable
- Sensor readings still timestamped if RTC fails
- Reduces complexity in main loop

**Timeline**:

- Boot: Use millis() / 1000
- First WiFi: Sync RTC via NTP
- Continue: Use RTC (survives WiFi disconnects via backup battery)

## Failure Modes & Handling

| Failure Mode | Impact | Handling |
|--------------|--------|----------|
| Sensor init fails | Readings unavailable | Continue, set validity flags false |
| WiFi unavailable | No telemetry/discovery | Retry every 10s, run sensors locally |
| mDNS discovery timeout | Config not fetched | Retry every 30s with backoff |
| MQTT connect fails | No telemetry uploaded | Retry automatically, continue reading |
| RTC sync fails | Timestamps inaccurate | Fall back to millis() |
| Memory exhaustion | Crashes/resets | Monitored via memory budget |

## Performance Characteristics

### Latency

- **Sensor read to JSON**: <15 ms
- **mDNS discovery (cold start)**: 1-5 seconds
- **HTTP config fetch**: 500-1000 ms
- **MQTT publish**: 100-200 ms

### Power Consumption (Estimated)

- **WiFi idle**: 50-100 mA
- **Sensor read**: 5-10 mA
- **mDNS query**: 30-50 mA (brief spike)
- **MQTT publish**: 40-60 mA

### Memory Budget

**Total Available**: 32 KB RAM, 256 KB Flash

| Component | RAM | Flash |
|-----------|-----|-------|
| WiFiNINA | 8 KB | 32 KB |
| Sensors | 1.5 KB | 6 KB |
| mDNS | 2 KB | 6 KB |
| MQTT | 2 KB | 8 KB |
| RTC | 256 B | 2 KB |
| Main + buffers | 512 B | 2 KB |
| **Used** | **14 KB** | **56 KB** |
| **Available** | **18 KB** | **200 KB** |
| **Margin** | **56%** | **78%** |

## Extension Points

### Adding New Sensors

1. Update `SensorReadings` struct with new fields
2. Add `read<SensorName>()` function in `sensors.cpp`
3. Add new fields to `formatSensorJSON()` with unit suffixes
4. Update [API.md](API.md) documentation

### Adding New Network Services

1. Create new module in `src/<service>/` with `.h` and `.cpp`
2. Implement initialization and polling functions
3. Call from main loop with appropriate timing
4. Update [ARCHITECTURE.md](ARCHITECTURE.md)

### Supporting Different Microcontrollers

1. Sensor driver uses Arduino abstractions (portable)
2. WiFiNINA library supports multiple boards
3. Changes needed:
   - I2C pin definitions (check datasheet)
   - Memory budget recalculation
   - RTC compatibility check

## Testing Strategy

### Unit Testing

- Mock I2C interface for sensor tests
- Parse mDNS packets without network
- Validate JSON formatting independently

### Integration Testing

- Deploy to device with debug build
- Monitor serial output for logs
- Verify mDNS discovery on network
- Publish to local MQTT broker

### Hardware Testing

- Verify 3.3V on all I2C lines
- Check pull-up resistor presence
- Test sensor readings match multimeter
- Confirm MQTT broker receives messages

---

**Last Updated**: 2026-02-13
**Platform**: Arduino MKR WiFi 1010 (SAMD21)
**Framework**: Arduino + PlatformIO
