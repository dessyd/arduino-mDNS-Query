/**
 * ============================================================================
 * Environmental Sensor Management Module
 * ============================================================================
 * Interfaces with Arduino MKR ENV Shield
 * Sensors: HTS221 (Temp/Humidity), LPS22HB (Pressure), TEMT6000 (Light)
 *
 * PLATFORM: Arduino MKR WiFi 1010 with MKR ENV Shield
 * LIBRARY: Arduino_MKRENV@^1.2.2
 *
 * ============================================================================
 */

#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>

/**
 * Sensor Readings Structure
 * Holds environmental sensor values with individual validity flags
 * for graceful degradation when sensors fail
 */
typedef struct {
  float temperature;        // Celsius (-40 to 120°C typical range)
  float humidity;          // Percentage (0-100%)
  float pressure;          // kPa (26-126 kPa typical)
  float illuminance;       // Lux (0-65535)
  float uv_index;          // UV Index (0-11+, -1.0 if not available)
  uint32_t timestamp;      // Seconds since device boot (millis() / 1000)

  bool valid;              // True if at least one sensor initialized
  bool temp_valid;         // Individual sensor validity flags
  bool humidity_valid;
  bool pressure_valid;
  bool light_valid;
  bool uv_valid;
} SensorReadings;

/**
 * Initialize all available sensors on MKR ENV Shield
 *
 * Attempts to initialize:
 *   - HTS221 (Temperature & Humidity)
 *   - LPS22HB (Pressure)
 *   - TEMT6000 (Light Intensity)
 *   - UV Sensor (if available on Rev1 boards)
 *
 * Includes 2-second warm-up delay for sensor stability.
 *
 * Returns:
 *   true if at least one sensor initialized successfully
 *   false if complete failure (shield not connected/responsive)
 */
bool initSensors(void);

/**
 * Read all sensor values and update readings structure
 *
 * Parameters:
 *   - readings: Pointer to SensorReadings struct to populate
 *
 * Returns:
 *   true if at least one valid reading obtained
 *   false if all sensors failed or sensors not initialized
 *
 * Behavior:
 *   - Individual sensor failures set validity flags to false
 *   - Last valid readings are preserved on partial failure
 *   - Timestamp is always updated to current millis()
 *   - Non-blocking operation (sensor reads take <10ms total)
 */
bool readSensors(SensorReadings* readings);

/**
 * Check if sensors are initialized and ready
 *
 * Returns:
 *   true if sensors were successfully initialized
 *   false if not initialized or initialization failed
 */
bool areSensorsReady(void);

/**
 * Check if sensor readings have changed significantly
 *
 * Compares two sensor readings and returns true if any value has changed
 * beyond its configured threshold. Uses individual sensor validity flags
 * to avoid comparing invalid readings.
 *
 * Parameters:
 *   - prev: Pointer to previous SensorReadings struct
 *   - curr: Pointer to current SensorReadings struct
 *
 * Returns:
 *   true if any sensor value exceeded its change threshold OR
 *   if any validity flag changed (sensor failure/recovery)
 *   false if all sensor values are within thresholds and no validity changes
 *
 * Thresholds (configurable in arduino_configs.h):
 *   - Temperature: ±0.5°C (CONFIG_TEMP_THRESHOLD_CELSIUS)
 *   - Humidity: ±3.5% (CONFIG_HUMIDITY_THRESHOLD_PERCENT)
 *   - Pressure: ±1.0 hPa (CONFIG_PRESSURE_THRESHOLD_HPA)
 *   - Illuminance: ±5% relative or ±50 lux absolute (CONFIG_ILLUMINANCE_THRESHOLD_*)
 *   - UV Index: ±0.5 (CONFIG_UV_THRESHOLD_INDEX)
 *
 * Notes:
 *   - Timestamp differences are ignored (not a sensor change)
 *   - NaN values are handled via validity flags
 *   - Returns true if either sensor changes OR validity changes (OR logic)
 */
bool hasSignificantChange(const SensorReadings* prev, const SensorReadings* curr);

/**
 * Format sensor readings as JSON string
 *
 * Parameters:
 *   - readings: Pointer to SensorReadings struct to format
 *   - buffer: Output buffer for JSON string
 *   - buffer_size: Maximum buffer size in bytes
 *
 * Returns:
 *   Pointer to buffer on success
 *   NULL if invalid parameters or buffer too small
 *
 * Format (example with all sensors valid):
 *   {"temperature":23.5,"humidity":45.2,"pressure":101.3,
 *    "illuminance":350.5,"uv_index":2.1,"timestamp":1707840000}
 *
 * Notes:
 *   - Only includes valid sensor readings
 *   - Always includes timestamp
 *   - Handles buffer overflow gracefully
 */
char* formatSensorJSON(const SensorReadings* readings,
                       char* buffer, size_t buffer_size);

#endif  // SENSORS_H
