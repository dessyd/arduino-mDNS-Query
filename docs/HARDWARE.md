# Hardware Setup Guide

## System Overview

```text
┌─────────────────────────────────────────────────────┐
│        Arduino MKR WiFi 1010 (SAMD21)               │
│                                                       │
│  ┌─────────────────────────────────────────────┐   │
│  │ WiFiNINA Module (U-blox NINA-W102)          │   │
│  │ • WiFi 802.11b/g/n                          │   │
│  │ • SPI interface (pins 7-10)                 │   │
│  └─────────────────────────────────────────────┘   │
│                                                       │
│  ┌─────────────────────────────────────────────┐   │
│  │ I2C Interface (Wire library)                │   │
│  │ SDA: Pin 11 (PA08)                          │   │
│  │ SCL: Pin 12 (PA09)                          │   │
│  └─────────────────────────────────────────────┘   │
│                                                       │
│  ┌─────────────────────────────────────────────┐   │
│  │ SAMD21 Microcontroller                      │   │
│  │ • 32-bit ARM Cortex-M0+                     │   │
│  │ • 256 KB Flash, 32 KB RAM                   │   │
│  │ • On-chip RTC                               │   │
│  └─────────────────────────────────────────────┘   │
│                                                       │
└─────────────────────────────────────────────────────┘
              │
              │ I2C Bus (3.3V)
              │
    ┌─────────┴────────┐
    │                  │
    v                  v
┌─────────────┐   ┌─────────────┐
│   MKR ENV   │   │ (Optional)  │
│   Shield    │   │ Additional  │
│             │   │ I2C Sensors │
├─────────────┤   │             │
│ • HTS221    │   │ 3.3V only   │
│   (Temp)    │   │             │
│ • LPS22HB   │   └─────────────┘
│   (Pressure)│
│ • TEMT6000  │
│   (Light)   │
│ • ML8511    │
│   (UV-Rev1) │
└─────────────┘
```text

## Pin Assignments

### Reserved Pins (WiFiNINA - DO NOT USE)

```cpp
#define PIN_WIFI_CS    7    // SPI Chip Select
#define PIN_WIFI_IRQ   5    // WiFi Interrupt
#define PIN_WIFI_RST   6    // WiFi Reset
#define PIN_WIFI_MOSI  8    // SPI MOSI
#define PIN_WIFI_MISO  10   // SPI MISO
#define PIN_WIFI_SCK   9    // SPI Clock
```text

**Warning**: Do not use pins 5-10 for user applications. These are dedicated to WiFiNINA module control.

### I2C Bus Pins (Sensor Shield)

```cpp
#define PIN_SDA 11  // I2C Data (SAMD21 PA08)
#define PIN_SCL 12  // I2C Clock (SAMD21 PA09)

// I2C configuration
Wire.begin();
Wire.setClock(100000);  // 100 kHz standard mode
```text

### Available User Pins

```cpp
#define PIN_LED_STATUS   LED_BUILTIN  // Built-in LED (13)
#define PIN_ANALOG_A0   A0  // Analog input 0
#define PIN_ANALOG_A1   A1  // Analog input 1
#define PIN_ANALOG_A5   A5  // Analog input 5 (free for power control)
#define PIN_DIGITAL_2   2   // External interrupt capable
#define PIN_DIGITAL_3   3   // External interrupt capable
#define PIN_DIGITAL_4   4   // Digital I/O
```text

## MKR ENV Shield I2C Addresses

| Sensor | Model | I2C Address | Interface |
|--------|-------|-------------|-----------|
| Temperature/Humidity | HTS221 | 0x5F | I2C @ 100 kHz |
| Pressure | LPS22HB | 0x5C | I2C @ 100 kHz |
| Light | TEMT6000 | 0x44 | I2C @ 100 kHz |
| UV (Rev1 only) | ML8511 | N/A | Analog input |

## Wiring Diagram

### Standard Setup (MKR ENV Shield Stacked)

```text
MKR1010 Top View:

      VCC ──── 3.3V
      GND ──── GND
      RST
      AREF
      A0
      A1
      A2
      A3
      A4 ┐
      A5 │
       3 │
       2 │ Available pins
       4 │
      11 ├─── I2C SDA (to shield)
      12 ├─── I2C SCL (to shield)
       8 │
      10 │
       9 │
       7 │
       6 ├─── WiFiNINA (reserved)
       5 │
      VIN
      5V (from shield)
      GND
      MOSI (8) ├─── WiFiNINA SPI
      MISO (10)├─── WiFiNINA SPI
      SCK (9) ├─── WiFiNINA SPI
      CS (7) ──── WiFiNINA SPI
```text

### I2C External Pull-up Resistors (Mandatory)

```text
            ┌─── SDA (Pin 11)
            │
      3.3V──┤
      │     │
      R1    R2     (2.2kΩ - 4.7kΩ each)
      │     │
      └─────┴─────┬─── MKR1010 I2C Bus

            ┌─── SCL (Pin 12)
            │
      3.3V──┤
      │     │
      R3    R4     (2.2kΩ - 4.7kΩ each)
      │     │
      └─────┴─────┬─── MKR1010 I2C Bus


GND connections for all pull-ups must connect to MKR1010 GND
```text

**Component Selection**:

```cpp
// Standard mode (100 kHz) ✓ Recommended
Pull-up: 4.7kΩ resistors

// Fast mode (400 kHz) - if needed
Pull-up: 2.2kΩ resistors
```text

### Adding External 5V Sensors (Level Shifting Required)

For connecting 5V I2C sensors (if required):

```text
External 5V Sensor:
    SDA (5V) ─┬─────[10kΩ]────┬─── MKR Pin 11 (3.3V)
              │                │
              └──────[20kΩ]────┴─── GND

    SCL (5V) ─┬─────[10kΩ]────┬─── MKR Pin 12 (3.3V)
              │                │
              └──────[20kΩ]────┴─── GND

// Voltage divider output: 5V × 20kΩ/(10kΩ+20kΩ) = 3.33V ✓
```text

**Recommended for easier level shifting**: I2C Level Shifter module

- Model: PCA9306 or similar
- Handles bidirectional I2C translation
- Supports 100 kHz and 400 kHz modes

## Bill of Materials (BOM)

### Required Components

| Part | Qty | Model/Spec | Source | Notes |
|------|-----|-----------|--------|-------|
| Arduino MKR WiFi 1010 | 1 | SAMD21 | Arduino Store | Microcontroller board |
| Arduino MKR ENV Shield | 1 | Official Shield | Arduino Store | Sensors: Temp/Humidity/Pressure/Light |
| Pull-up Resistors | 4 | 4.7kΩ 1/4W | Any electronics store | For I2C SDA/SCL |
| USB Cable | 1 | USB-C | Any electronics store | Programming & power |

### Optional Components

| Part | Qty | Model/Spec | Source | Notes |
|------|-----|-----------|--------|-------|
| External Power Supply | 1 | 5-20V DC | Any electronics store | For longer-term deployment |
| RTC Battery | 1 | CR2032 | Any electronics store | Time persistence when powered off |
| WiFi Antenna | 1 | 2.4GHz | Arduino Store | Improves range (some boards need this) |
| I2C Level Shifter | 1 | PCA9306 | SparkFun/Adafruit | For 5V sensor connections |
| Breadboard & Jumpers | 1 set | Solderless | Any electronics store | Prototyping external sensors |

## Power Supply

### Specifications

| Parameter | Value | Notes |
|-----------|-------|-------|
| Operating Voltage | 3.3V (internal) | MKR1010 regulates from input |
| Input Voltage Range | 5-20V DC | Via VIN pin |
| USB Power | 5V / 500 mA | Via USB-C connector |
| Typical Current | 80-150 mA | WiFi active, sensors reading |
| Peak Current | 300 mA | WiFi transmission spike |

### Battery Considerations

For standalone deployment:

```text
Battery Bank → VIN (5V) → MKR1010 → Onboard regulator → 3.3V system
```text

**Recommended battery**:

- 5V USB Power Bank (10,000 mAh)
- Runtime: ~20-30 hours continuous operation
- USB cable connection to MKR1010 USB-C port

## Memory Budget

### SRAM Usage (32 KB Total)

```text
WiFiNINA Stack:      8 KB (40%)
Sensor Buffers:      1.5 KB (5%)
MQTT Client:         2 KB (6%)
mDNS Packets:        2 KB (6%)
Global Variables:    512 B (2%)
Stack Space:         ~18 KB (56% remaining) ✓
```text

**Status**: ✅ Healthy margin - room for additional features

### Flash Usage (256 KB Total)

```text
WiFiNINA Firmware:   32 KB (12%)
Arduino Runtime:     16 KB (6%)
Application Code:    6 KB (2%)
Sensor Module:       6 KB (2%)
MQTT Client:         8 KB (3%)
mDNS Module:         6 KB (2%)
RTC Module:          2 KB (1%)
Free Space:          ~180 KB (70%) ✓
```text

**Status**: ✅ Very healthy - ample space for code expansion

## Sensor Shield Variants

### MKR ENV Shield Rev1 (with UV sensor)

```cpp
#include <Arduino_MKRENV.h>

// Available sensors:
ENV.readTemperature(CELSIUS)
ENV.readHumidity()
ENV.readPressure(MILLIBAR)
ENV.readIlluminance(LUX)
ENV.readUVA()        // ✓ Available
ENV.readUVB()        // ✓ Available
```text

### MKR ENV Shield Rev2 (without UV sensor)

```cpp
// Same as Rev1 except UV not available:
ENV.readUVA()        // ✗ Returns NaN
ENV.readUVB()        // ✗ Returns NaN
```text

**Detection Code**:

```cpp
float uva = ENV.readUVA();
if (isnan(uva)) {
  Serial.println("UV sensor not available (Rev2 board)");
  has_uv_sensor = false;
} else {
  has_uv_sensor = true;
}
```

## Troubleshooting

### I2C Communication Issues

**Symptom**: "Failed to initialize ENV shield"

**Diagnosis Steps**:

1. **Check Physical Connection**

   ```bash
   # Verify shield is properly seated
   # Check for bent pins
   # Ensure correct pin alignment
   ```

2. **Verify Pull-up Resistors**

   ```text
   Measure voltage on SDA with multimeter:
   - Expected at rest: 3.3V
   - If <1V: Pull-up may be missing
   - If >3.3V: Wiring error
   ```

3. **I2C Scan Test**

   ```cpp
   // Add to setup()
   Wire.begin();
   Wire.setClock(100000);

   for (byte addr = 1; addr < 127; addr++) {
     Wire.beginTransmission(addr);
     if (Wire.endTransmission() == 0) {
       Serial.print("Found device at 0x");
       Serial.println(addr, HEX);
     }
   }
   // Should find devices at 0x5F, 0x5C, 0x44
   ```

4. **Check Voltage Levels**

   ```text
   Measure with multimeter:
   VCC: Should be 3.3V ✓
   GND: Should be 0V ✓
   SDA idle: Should be 3.3V ✓
   SCL idle: Should be 3.3V ✓

   If any are low, check pull-up resistors
   ```

### WiFi Connection Issues

**Symptom**: WiFi connects/disconnects repeatedly

**Solutions**:

1. Check antenna connection (if external)
2. Verify SSID is 2.4 GHz (not 5 GHz)
3. Update WiFiNINA firmware to latest
4. Check WiFi password is correct

### Sensor Readings Unrealistic

**Symptom**: Temperature reading 200°C or garbage values

**Causes**:

- I2C communication errors (check pull-ups)
- Sensor not initialized (check initSensors() return)
- Defective sensor (try replacing shield)
- Temperature sensor out of range (expected -40 to +120°C)

**Fix**:

```cpp
// Check individual sensor validity before use
if (readings.temp_valid && readings.temperature > -40 && readings.temperature < 120) {
  // Safe to use reading
} else {
  // Sensor data invalid, skip
}
```text

## Testing Checklist

Before deployment:

- [ ] USB power connects and board enumerates
- [ ] Serial monitor opens at 115200 baud
- [ ] Sensor shield properly seated
- [ ] I2C devices detected (scan test)
- [ ] Temperature reading within expected range
- [ ] Humidity reading between 0-100%
- [ ] Pressure reading between 950-1050 mbar
- [ ] Light sensor responds to room brightness
- [ ] WiFi connects successfully
- [ ] mDNS discovery finds config server
- [ ] MQTT messages published to broker

## Debugging via Serial Monitor

```bash
# Build with debug output
pio run -e mkr1010_debug -t upload

# Monitor serial output
pio device monitor -e mkr1010_debug -b 115200

# Expected output on startup:
# === SENSOR INITIALIZATION ===
# → Initializing MKR ENV Shield...
# → Warming up sensors (2 seconds)...
# → Testing individual sensors...
#   ✓ Temperature sensor ready (HTS221)
#   ✓ Humidity sensor ready (HTS221)
#   ✓ Pressure sensor ready (LPS22HB)
#   ✓ Light sensor ready (TEMT6000)
#   ⚠ UV sensor not available (Rev2 board or not present)
# ✓ Environmental sensors initialized successfully
#
# === WIFI SETUP ===
# → Connecting to WiFi "MyNetwork"...
# ✓ WiFi connected - IP: 192.168.1.100
#
# === SENSOR TELEMETRY ===
# {"temperature_celsius":24.5,"humidity_percent":52.3,...}
```text

## Safety Warnings

### ⚠️ Critical

1. **Never connect 5V logic to MKR1010 I2C pins** - use level shifters
2. **Check pull-up resistor values** - wrong values cause I2C failures
3. **Use only 3.3V sensors** - 5V sensors require external level shifting
4. **Verify power supply voltage** - incorrect voltage damages board

### Important

1. **WiFiNINA pins 5-10 reserved** - do not use for custom hardware
2. **USB power limit** - ~500 mA max from computer USB ports
3. **RTC battery optional** - without it, timestamps reset on power cycle
4. **Heat dissipation** - board generates minimal heat, passive cooling sufficient

## Regulatory Compliance

### FCC (USA)

The WiFiNINA module contains certified radio transmitter. The complete system should follow FCC regulations for Wi-Fi devices operating at 2.4 GHz.

### CE (Europe)

Ensure compliance with:

- EMC Directive 2014/30/EU
- Radio Equipment Directive 2014/53/EU

### RoHS

All components comply with RoHS restrictions on hazardous substances.

---

**Last Updated**: 2026-02-13
**Platform**: Arduino MKR WiFi 1010 with MKR ENV Shield
**Framework**: Arduino + PlatformIO
