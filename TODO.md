# Project TODOs

## Optimization: Selective Publishing for Change Detection

**Status**: Pending

**Description**: When publishing sensor values outside of heartbeat cycles (i.e., on change detection), only publish the changed value(s) plus timestamp, rather than publishing all sensor values as in the current design.

**Status**: ✅ COMPLETED

**Related Code**:

- Heartbeat publishing logic
- Change detection publishing logic
- SENSOR_TELEMETRY.md

**Rationale**: Reduces MQTT message volume and bandwidth by omitting unchanged values during change-triggered publishes.

**Impact**: Performance optimization, reduced network traffic.

## Optimization: Eliminate Duplicate Config Logging

**Status**: ✅ COMPLETED

**Description**: The configuration is currently printed twice during startup:

1. Once with verbose status messages during the fetch process
2. Again at the end with a summary display

**Related Code**:

- Configuration retrieval/parsing logic in `config_fetch.cpp`
- Startup serial output in `main.cpp`

**Solution**: Removed detailed field-by-field logging from `parseConfigJSON()` and kept only the summary display in `main.cpp` initialization. Added minimal confirmation message that parsing succeeded.

**Impact**: Cleaner startup output, reduced serial verbosity, easier to read logs.
