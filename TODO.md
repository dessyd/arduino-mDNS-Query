# Project TODOs

## Optimization: Selective Publishing for Change Detection

**Status**: ✅ COMPLETED

**Description**: When publishing sensor values outside of heartbeat cycles
(i.e., on change detection), only publish the changed value(s) plus timestamp,
rather than publishing all sensor values as in the current design.

**Related Code**:

- Heartbeat publishing logic
- Change detection publishing logic
- SENSOR_TELEMETRY.md

**Rationale**: Reduces MQTT message volume and bandwidth by omitting
unchanged values during change-triggered publishes.

**Impact**: Performance optimization, reduced network traffic.

## Optimization: Eliminate Duplicate Config Logging

**Status**: ✅ COMPLETED

**Description**: The configuration is currently printed twice during startup:

1. Once with verbose status messages during the fetch process
2. Again at the end with a summary display

**Related Code**:

- Configuration retrieval/parsing logic in `config_fetch.cpp`
- Startup serial output in `main.cpp`

**Solution**: Removed detailed field-by-field logging from
`parseConfigJSON()` and kept only the summary display in `main.cpp`
initialization. Added minimal confirmation message that parsing succeeded.

**Impact**: Cleaner startup output, reduced serial verbosity, easier to read logs.

## Future Enhancements

### Performance & Optimization

#### 1. Add configurable change detection thresholds via HTTP API

- Currently hardcoded in `arduino_configs.h`
- Allow dynamic threshold adjustment from config server
- Impact: Better adaptability to different environments

#### 2. Implement MQTT message batching for rapid changes

- Queue multiple changed values during rapid fluctuations
- Publish batch once per heartbeat instead of per change
- Impact: Further reduce MQTT overhead during environmental spikes

#### 3. Add sensor outlier detection

- Filter anomalous readings (e.g., 99°C spike)
- Prevents false change detections
- Impact: More reliable change detection

### Features & Observability

#### 4. Add device uptime/status monitoring

- Track boot count, last successful publish time
- Publish as separate MQTT status topic
- Impact: Better device health visibility

#### 5. Implement graceful degradation logging

- Currently silent when sensors fail
- Add periodic health check messages
- Impact: Easier troubleshooting of sensor failures

#### 6. Add MQTT last-will-and-testament (LWT)

- Publish "offline" message on unexpected disconnect
- Impact: Better device availability monitoring

### Security & Stability

#### 7. Add TLS/MQTT over SSL support

- Currently port 8883 falls back to 1883
- Implement proper certificate handling
- Impact: Encrypted telemetry channel

#### 8. Add configuration validation on device

- Validate poll/heartbeat intervals on receipt
- Check JSON schema before processing
- Impact: Prevent invalid configurations from breaking device
