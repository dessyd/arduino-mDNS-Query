# Sensor Telemetry API

## Overview

The sensor module provides environmental monitoring data with explicit unit designations. All sensor readings are published to MQTT broker and formatted with self-documenting field names that include units.

**Update**: As of February 2026, all sensor APIs now use explicit unit parameters and JSON field names include unit suffixes for clarity.

## Sensor Readings Structure

### C++ Data Type

```cpp
typedef struct {
  float temperature;        // Celsius (-40°C to +120°C typical)
  float humidity;          // Percentage (0-100% RH)
  float pressure;          // Millibar (26-126 kPa converted)
  float illuminance;       // Lux (0-65535)
  float uv_index;          // UV Index (0-11+, or -1.0 if unavailable)
  uint32_t timestamp;      // Seconds since device boot

  bool valid;              // True if at least one sensor initialized
  bool temp_valid;         // Individual per-sensor validity flags
  bool humidity_valid;
  bool pressure_valid;
  bool light_valid;
  bool uv_valid;
} SensorReadings;
```text

## API Functions

### Initialize Sensors

```cpp
bool initSensors(void);
```text

**Description**: Initialize all available sensors on MKR ENV Shield.

**Attempts to initialize**:

- HTS221 (Temperature & Humidity) at I2C 0x5F
- LPS22HB (Pressure) at I2C 0x5C
- TEMT6000 (Light Intensity) at I2C 0x44
- UV Sensor (optional, Rev1 boards only)

**Warm-up**: 2-second delay for sensor stabilization

**Returns**:

- `true` - At least one sensor initialized successfully
- `false` - All sensors failed or shield not detected

**Example**:

```cpp
if (!initSensors()) {
  Serial.println("✗ Sensor initialization failed");
  // Handle error
}
```text

### Read Sensor Values

```cpp
bool readSensors(SensorReadings* readings);
```text

**Description**: Read all sensor values and populate SensorReadings structure.

**Parameters**:

- `readings` - Pointer to SensorReadings struct to populate

**Unit Parameters**:

- Temperature: `CELSIUS` (ENV library constant)
- Pressure: `MILLIBAR` (ENV library constant)
- Illuminance: `LUX` (ENV library constant)

**Returns**:

- `true` - At least one valid reading obtained
- `false` - All sensors failed or not initialized

**Behavior**:

- Individual sensor failures set corresponding validity flags to false
- Last valid readings preserved if partial failure occurs
- Timestamp always updated (uses RTC if available, falls back to millis())
- Non-blocking operation (<15 ms total)

**Example**:

```cpp
SensorReadings readings;
if (readSensors(&readings)) {
  if (readings.temp_valid) {
    Serial.print("Temperature: ");
    Serial.print(readings.temperature);
    Serial.println(" °C");
  }

  if (readings.pressure_valid) {
    Serial.print("Pressure: ");
    Serial.print(readings.pressure);
    Serial.println(" mbar");
  }
}
```text

### Check Sensor Status

```cpp
bool areSensorsReady(void);
```text

**Description**: Check if sensors are initialized and ready to read.

**Returns**:

- `true` - Sensors initialized
- `false` - Not initialized or initialization failed

**Example**:

```cpp
if (areSensorsReady()) {
  readSensors(&data);
} else {
  Serial.println("Sensors not ready, skipping read");
}
```text

### Format as JSON

```cpp
char* formatSensorJSON(const SensorReadings* readings,
                       char* buffer, size_t buffer_size);
```text

**Description**: Format sensor readings as JSON string with explicit units.

**Parameters**:

- `readings` - Pointer to SensorReadings struct
- `buffer` - Output buffer for JSON string
- `buffer_size` - Maximum buffer size in bytes (recommend ≥256)

**Returns**:

- Pointer to buffer on success
- `NULL` if invalid parameters or buffer too small

**Behavior**:

- Only includes valid sensor readings (checks individual validity flags)
- Always includes timestamp
- Field names include unit suffixes
- Handles buffer overflow gracefully

**Example**:

```cpp
SensorReadings readings;
char jsonBuffer[256];

if (readSensors(&readings)) {
  char* json = formatSensorJSON(&readings, jsonBuffer, sizeof(jsonBuffer));
  if (json != NULL) {
    Serial.println(json);
  }
}
```text

## JSON Output Format

### Complete Example (All Sensors Valid)

```json
{
  "temperature_celsius": 24.5,
  "humidity_percent": 52.3,
  "pressure_millibar": 1013.25,
  "illuminance_lux": 450.5,
  "uv_index": 2.1,
  "timestamp": 12345
}
```text

### Partial Example (Humidity Sensor Failed)

```json
{
  "temperature_celsius": 24.5,
  "pressure_millibar": 1013.25,
  "illuminance_lux": 450.5,
  "timestamp": 12345
}
```text

Note: `humidity_percent` field omitted because humidity sensor failed

### Field Descriptions

| Field | Type | Unit | Range | Description |
|-------|------|------|-------|-------------|
| `temperature_celsius` | float | °C | -40 to +120 | Environmental temperature from HTS221 |
| `humidity_percent` | float | % RH | 0-100 | Relative humidity from HTS221 |
| `pressure_millibar` | float | mbar | 26-126 | Atmospheric pressure from LPS22HB |
| `illuminance_lux` | float | lux | 0-65535 | Light intensity from TEMT6000 |
| `uv_index` | float | index | 0-11+ | UV index from UV sensor (Rev1 only, -1.0 if unavailable) |
| `timestamp` | uint32 | seconds | 0-2³²-1 | Seconds since device boot (or RTC if synced) |

## MQTT Publishing

### Topic Structure

```text
devices/<device_id>/telemetry
```text

**Example**: `devices/MKR1010-ABCD1234/telemetry`

### Publishing Settings

- **QoS**: 1 (at-least-once delivery)
- **Retain**: true (broker stores last message)
- **Interval**: 10 seconds (configurable)
- **Format**: JSON with unit suffixes

### MQTT Payload Example

```json
{
  "temperature_celsius": 24.5,
  "humidity_percent": 52.3,
  "pressure_millibar": 1013.25,
  "illuminance_lux": 450.5,
  "uv_index": 2.1,
  "timestamp": 12345
}
```text

## Hardware Specifications

### MKR ENV Shield Sensors

| Sensor | Model | Interface | Range | Accuracy | Resolution |
|--------|-------|-----------|-------|----------|-----------|
| Temperature | HTS221 | I2C | -40 to +120°C | ±0.5°C | 0.002°C |
| Humidity | HTS221 | I2C | 0-100% RH | ±3.5% | 0.002% |
| Pressure | LPS22HB | I2C | 300-1100 hPa | ±1 hPa | 0.01 hPa |
| Light | TEMT6000 | I2C | 0-65535 lux | ±5% | Variable |
| UV (Rev1 only) | ML8511 | I2C | 0-11+ index | ±0.5 | Variable |

### I2C Configuration

```cpp
// Pin assignments
#define I2C_SDA 11  // SAMD21 PA08
#define I2C_SCL 12  // SAMD21 PA09

// Speed: 100 kHz (standard mode)
Wire.setClock(100000);

// External pull-up resistors required
// 2.2kΩ - 4.7kΩ on both SDA and SCL to 3.3V
```text

**Critical**: MKR1010 has weak internal pull-ups (~35kΩ). External resistors are essential for reliable I2C communication at standard clock speeds.

## Sensor Calibration

### Temperature Offset

The HTS221 sensor can have a temperature offset. To apply a calibration:

```cpp
// In readSensors():
readings->temperature = ENV.readTemperature(CELSIUS) + TEMP_CALIBRATION_OFFSET;
// where TEMP_CALIBRATION_OFFSET is defined in arduino_configs.h
```text

### Light Sensor Linearity

The TEMT6000 is approximately linear but may require individual calibration for applications requiring ±1% accuracy.

### Pressure Altitude Correction

To convert pressure to altitude:

```cpp
// Barometric formula (simplified)
float altitude = 44330 * (1.0 - pow(pressure_mbar / SEA_LEVEL_PRESSURE, 1.0/5.255));
```text

## Error Handling

### Validity Flags

Each sensor has an associated validity flag. Check before using:

```cpp
if (readings.temp_valid) {
  float temp = readings.temperature;
  // Use temperature safely
} else {
  Serial.println("Temperature sensor reading is invalid");
  // Handle missing data
}
```text

### Graceful Degradation

When individual sensors fail:

- Valid sensors continue providing data
- Invalid fields omitted from JSON output
- Application can continue partial monitoring
- All failures logged to serial (debug mode)

**Example**: If humidity fails but temperature works:

```json
{
  "temperature_celsius": 24.5,
  "pressure_millibar": 1013.25,
  "illuminance_lux": 450.5,
  "timestamp": 12345
}
```text

## Polling Strategy

### Recommended Intervals

| Scenario | Interval | Justification |
|----------|----------|---------------|
| Environmental monitoring | 10-30 seconds | Captures slow environmental changes |
| HVAC control feedback | 5 seconds | Responsive to system changes |
| Long-term logging | 60 seconds | Reduces storage/bandwidth |
| Research/calibration | 1 second | High-frequency sampling |

### Memory Considerations

Sensor readings do not require dynamic memory allocation:

```cpp
// Efficient - stack allocation
SensorReadings readings;
readSensors(&readings);

// Not required - static data
static SensorReadings last_reading;
```text

## Voltage Level Safety

### ⚠️ Critical Warning

MKR WiFi 1010 operates at **3.3V only**. The MKR ENV Shield is 3.3V compatible and safe.

**Do not connect** 5V sensors directly to MKR1010 I2C bus - this will destroy the microcontroller.

### 5V Sensor Level Shifting (If Required)

For external 5V sensors, use voltage divider or I2C level shifter:

```text
5V Sensor → [10kΩ] → MKR Pin (with 20kΩ to GND)
Output voltage: 5V × 20kΩ/(10kΩ+20kΩ) = 3.33V ✅
```text

## Testing

### Unit Test Example (Mock I2C)

```cpp
#include "sensors.h"
#include "mock_wire.h"

void testTemperatureReading() {
  // Mock I2C response for HTS221
  MockWire::setExpectedTemperature(25.0);

  SensorReadings readings;
  readSensors(&readings);

  assert(readings.temp_valid == true);
  assert(readings.temperature == 25.0);
  assert(readings.timestamp > 0);
}
```text

### Integration Test (On Device)

```cpp
// Upload debug build
pio run -e mkr1010_debug -t upload

// Monitor output
pio device monitor -e mkr1010_debug -b 115200

// Expected output:
// === SENSOR INITIALIZATION ===
// → Initializing MKR ENV Shield...
// → Warming up sensors (2 seconds)...
// → Testing individual sensors...
//   ✓ Temperature sensor ready (HTS221)
//   ✓ Humidity sensor ready (HTS221)
//   ✓ Pressure sensor ready (LPS22HB)
//   ✓ Light sensor ready (TEMT6000)
//   ⚠ UV sensor not available (Rev2 board or not present)
// ✓ Environmental sensors initialized successfully
```text

## Version History

| Date | Version | Changes |
|------|---------|---------|
| 2026-02-13 | 2.0 | Added explicit unit suffixes to JSON field names (temperature_celsius, humidity_percent, pressure_millibar, illuminance_lux) |
| 2026-02-12 | 1.0 | Initial sensor API documentation |

## Related Documentation

- [ARCHITECTURE.md](ARCHITECTURE.md) - System design overview
- [MQTT_SETUP.md](MQTT_SETUP.md) - MQTT broker configuration
- [README.md](../README.md) - Quick start guide
- [API.md](API.md) - Configuration server API

---

**Last Updated**: 2026-02-13
**Platform**: Arduino MKR WiFi 1010 with MKR ENV Shield
**Framework**: Arduino + PlatformIO
