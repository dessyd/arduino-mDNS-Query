# arduino-mDNS-Query

IoT firmware for Arduino MKR WiFi 1010 with environmental sensors, MQTT telemetry, and local mDNS service discovery.

## Features

- **Environmental Monitoring**: Temperature, humidity, pressure, and light intensity via MKR ENV Shield
- **mDNS Service Discovery**: Query local network services using RFC 6762 multicast DNS
- **MQTT Telemetry**: Publish sensor readings to MQTT broker with configurable intervals
- **RTC Integration**: Real-time clock for accurate timestamps
- **Device Identity**: Unique device ID based on SAMD21 silicon serial number
- **Remote Configuration**: Fetch MQTT and network settings from mDNS discovery

## Hardware Requirements

- Arduino MKR WiFi 1010 (SAMD21, 256KB Flash, 32KB RAM)
- Arduino MKR ENV Shield (HTS221, LPS22HB, TEMT6000 sensors)
- WiFi network with mDNS support
- MQTT broker (optional, for telemetry)
- USB power or external 5-20V power supply

## Quick Start

### 1. Install PlatformIO

```bash
pip install platformio
# Or install VS Code PlatformIO extension
```

### 2. Clone and Configure

```bash
git clone https://github.com/dessyd/arduino-mDNS-Query.git
cd arduino-mDNS-Query
cp include/arduino_secrets.h.template include/arduino_secrets.h
# Edit arduino_secrets.h with your WiFi SSID and password
```

### 3. Build and Upload

```bash
# Build for MKR1010 (development with serial logging)
pio run -e mkr1010_debug -t upload

# Monitor serial output
pio device monitor -e mkr1010_debug -b 115200
```

### 4. Watch Sensor Data

Once uploaded, the device will:

1. Initialize WiFi and sensor shield
2. Broadcast its presence via mDNS
3. Attempt to discover MQTT broker on local network
4. Read sensors every 10 seconds
5. Output JSON with units:

   ```json
   {"temperature_celsius":24.5,"humidity_percent":52.3,"pressure_millibar":1013.2,"illuminance_lux":450.5,"timestamp":12345}
   ```

## Documentation

| Document | Purpose |
|----------|---------|
| [ARCHITECTURE.md](docs/ARCHITECTURE.md) | System design and module interactions |
| [API.md](docs/API.md) | Public API reference |
| [HARDWARE.md](docs/HARDWARE.md) | Pin assignments and wiring |
| [MQTT_SETUP.md](docs/MQTT_SETUP.md) | MQTT configuration and topics |
| [RTC_STATE_MACHINE.md](docs/RTC_STATE_MACHINE.md) | RTC synchronization state machine |

## Build Environments

```bash
# Development (debug logging enabled, no low-power)
pio run -e mkr1010_debug

# Staging (limited logging, low-power enabled)
pio run -e mkr1010_staging

# Production (no logging, full low-power optimization)
pio run -e mkr1010_production
```

## Sensor Data Format

All sensor readings include explicit units in JSON output:

```json
{
  "temperature_celsius": 24.5,      // -40°C to +85°C
  "humidity_percent": 52.3,          // 0-100% RH
  "pressure_millibar": 1013.2,       // Atmospheric pressure
  "illuminance_lux": 450.5,          // Light intensity
  "uv_index": 2.1,                   // UV Index (if available)
  "timestamp": 12345                 // Seconds since boot
}
```

**Note**: UV sensor is only available on MKR ENV Shield Rev1. Rev2 boards omit UV support.

## Network Configuration

### mDNS Service Discovery

The device advertises as `<device_id>._http._tcp.local` allowing other devices to discover it without hardcoded IP addresses.

### WiFi Connection

- Configurable SSID and password in `arduino_secrets.h`
- 10-second connection timeout
- Automatic retry every 30 seconds on failure

### MQTT Broker Discovery

Device attempts to locate MQTT broker via mDNS (`_mqtt._tcp.local`) before connecting directly.

## Memory Usage

| Module | RAM | Flash |
|--------|-----|-------|
| WiFiNINA | ~8 KB | ~32 KB |
| Sensors | ~1.5 KB | ~6 KB |
| MQTT | ~2 KB | ~8 KB |
| mDNS | ~2 KB | ~6 KB |
| RTC | ~256 B | ~2 KB |
| **Total Used** | **~14 KB** | **~55 KB** |
| **Available** | **32 KB** | **256 KB** |
| **Margin** | **56%** | **79%** |

## Troubleshooting

### Serial Monitor Shows "✗ Failed to initialize ENV shield"

**Cause**: Shield not detected or I2C connection issue

**Solutions**:

- Verify shield is properly seated on the MKR1010
- Check I2C pull-up resistors (2.2kΩ - 4.7kΩ on SDA/SCL to 3.3V)
- Ensure no 5V devices are connected to I2C bus
- Use `pio device monitor` to see detailed error messages

### WiFi Connection Fails

**Cause**: Incorrect SSID/password or WiFi network not found

**Solutions**:

- Verify `arduino_secrets.h` has correct WiFi credentials
- Ensure WiFi network is 2.4GHz (5GHz not supported on MKR1010)
- Check WiFiNINA firmware version: should be 1.4.8 or later
- Check serial monitor for detailed error messages

### MQTT Connection Not Established

**Cause**: Broker not found or configuration issue

**Solutions**:

- Verify MQTT broker running on local network
- Check broker advertises `_mqtt._tcp.local` mDNS service
- See [MQTT_SETUP.md](docs/MQTT_SETUP.md) for broker configuration

## Development Workflow

### Code Changes

1. Make changes to source files in `src/` and `include/`
2. Build locally: `pio run -e mkr1010_debug`
3. Upload to device: `pio run -e mkr1010_debug -t upload`
4. Monitor: `pio device monitor -e mkr1010_debug -b 115200`

### Adding New Sensors

1. Add sensor driver in `src/sensors/` directory
2. Update `SensorReadings` struct in `include/sensors/sensors.h`
3. Update `formatSensorJSON()` to include new fields with unit suffixes
4. Document new fields in [API.md](docs/API.md)

## Contributing

Before submitting changes:

1. Follow naming conventions: `lowercase_with_underscores` for files
2. Add Doxygen-style comments to public APIs
3. Test on device with `pio run -e mkr1010_debug`
4. Keep `arduino_secrets.h` out of commits (uses `.gitignore`)
5. Update relevant documentation files

## License

MIT License - see [LICENSE](LICENSE)

## Support

For issues and questions:

- Check [TROUBLESHOOTING.md](#troubleshooting) section above
- Review existing [GitHub Issues](https://github.com/dessyd/arduino-mDNS-Query/issues)
- Open a new issue with detailed error messages and serial output
