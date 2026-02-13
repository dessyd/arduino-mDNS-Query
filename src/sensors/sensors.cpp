#include <Arduino.h>
#include "sensors/sensors.h"
#include "arduino_configs.h"
#include <Arduino_MKRENV.h>

// ============================================================================
// STATIC STATE - Sensor initialization and last readings
// ============================================================================

static bool sensors_initialized = false;
static bool has_uv_sensor = false;
static SensorReadings last_valid_readings;

// ============================================================================
// HELPER FUNCTIONS - Sensor detection and initialization
// ============================================================================

/**
 * Test if a sensor is available by attempting a read
 */
static bool testSensorAvailable(float (*test_read)(void))
{
  if (!test_read)
  {
    return false;
  }

  // Attempt a test read
  float value = test_read();

  // Simple heuristic: if we get a number (not NaN), sensor is responsive
  // Note: This is basic; ideally would check against expected ranges
  return !isnan(value);
}

// ============================================================================
// PUBLIC API IMPLEMENTATION
// ============================================================================

/**
 * Initialize all available sensors
 */
bool initSensors(void)
{
  DEBUG_PRINTLN(F(""));
  DEBUG_PRINTLN(F("=== SENSOR INITIALIZATION ==="));
  DEBUG_PRINTLN(F("→ Initializing MKR ENV Shield..."));

  // Initialize the ENV shield (I2C sensors)
  if (!ENV.begin())
  {
    DEBUG_PRINTLN(F("✗ Failed to initialize ENV shield - check I2C connection"));
    sensors_initialized = false;
    return false;
  }

  // Give sensors time to stabilize (2 second warm-up)
  DEBUG_PRINTLN(F("→ Warming up sensors (2 seconds)..."));
  delay(2000);

  // Initialize readings structure
  memset(&last_valid_readings, 0, sizeof(SensorReadings));
  last_valid_readings.uv_index = -1.0;  // Default UV to unavailable

  // Test individual sensors
  DEBUG_PRINTLN(F("→ Testing individual sensors..."));

  // Test Temperature/Humidity (HTS221 at I2C 0x5F)
  if (testSensorAvailable([]() { return ENV.readTemperature(); }))
  {
    DEBUG_PRINTLN(F("  ✓ Temperature sensor ready (HTS221)"));
    last_valid_readings.temp_valid = true;
  }
  else
  {
    DEBUG_PRINTLN(F("  ✗ Temperature sensor failed"));
  }

  if (testSensorAvailable([]() { return ENV.readHumidity(); }))
  {
    DEBUG_PRINTLN(F("  ✓ Humidity sensor ready (HTS221)"));
    last_valid_readings.humidity_valid = true;
  }
  else
  {
    DEBUG_PRINTLN(F("  ✗ Humidity sensor failed"));
  }

  // Test Pressure (LPS22HB at I2C 0x5C)
  if (testSensorAvailable([]() { return ENV.readPressure(); }))
  {
    DEBUG_PRINTLN(F("  ✓ Pressure sensor ready (LPS22HB)"));
    last_valid_readings.pressure_valid = true;
  }
  else
  {
    DEBUG_PRINTLN(F("  ✗ Pressure sensor failed"));
  }

  // Test Light (TEMT6000 at I2C 0x44)
  if (testSensorAvailable([]() { return ENV.readIlluminance(); }))
  {
    DEBUG_PRINTLN(F("  ✓ Light sensor ready (TEMT6000)"));
    last_valid_readings.light_valid = true;
  }
  else
  {
    DEBUG_PRINTLN(F("  ✗ Light sensor failed"));
  }

  // Test UV sensor (optional, may not be present on Rev2)
  if (testSensorAvailable([]() { return ENV.readUVA(); }))
  {
    DEBUG_PRINTLN(F("  ✓ UV sensor available (Rev1 board)"));
    has_uv_sensor = true;
    last_valid_readings.uv_valid = true;
  }
  else
  {
    DEBUG_PRINTLN(F("  ⚠ UV sensor not available (Rev2 board or not present)"));
    has_uv_sensor = false;
  }

  // Determine overall validity (at least one sensor working)
  if (last_valid_readings.temp_valid ||
      last_valid_readings.humidity_valid ||
      last_valid_readings.pressure_valid ||
      last_valid_readings.light_valid)
  {
    sensors_initialized = true;
    last_valid_readings.valid = true;

    DEBUG_PRINTLN(F(""));
    DEBUG_PRINTLN(F("✓ Environmental sensors initialized successfully"));
    return true;
  }

  DEBUG_PRINTLN(F("✗ All sensors failed - shield may not be properly connected"));
  sensors_initialized = false;
  return false;
}

/**
 * Read all sensor values
 */
bool readSensors(SensorReadings* readings)
{
  if (!readings)
  {
    DEBUG_PRINTLN(F("✗ Invalid readings pointer"));
    return false;
  }

  if (!sensors_initialized)
  {
    DEBUG_PRINTLN(F("✗ Sensors not initialized"));
    return false;
  }

  // Initialize validity flags to false (mark as invalid until we read successfully)
  readings->temp_valid = false;
  readings->humidity_valid = false;
  readings->pressure_valid = false;
  readings->light_valid = false;
  readings->uv_valid = false;
  readings->valid = false;

  // Always update timestamp
  readings->timestamp = millis() / 1000;

  // Read Temperature
  if (last_valid_readings.temp_valid)
  {
    readings->temperature = ENV.readTemperature();
    if (!isnan(readings->temperature))
    {
      readings->temp_valid = true;
    }
  }

  // Read Humidity
  if (last_valid_readings.humidity_valid)
  {
    readings->humidity = ENV.readHumidity();
    if (!isnan(readings->humidity) && readings->humidity >= 0 && readings->humidity <= 100)
    {
      readings->humidity_valid = true;
    }
  }

  // Read Pressure
  if (last_valid_readings.pressure_valid)
  {
    readings->pressure = ENV.readPressure();
    if (!isnan(readings->pressure))
    {
      readings->pressure_valid = true;
    }
  }

  // Read Light Intensity
  if (last_valid_readings.light_valid)
  {
    readings->illuminance = ENV.readIlluminance();
    if (!isnan(readings->illuminance) && readings->illuminance >= 0)
    {
      readings->light_valid = true;
    }
  }

  // Read UV (if available)
  if (has_uv_sensor)
  {
    float uva = ENV.readUVA();
    float uvb = ENV.readUVB();

    // Simple UV index calculation (very approximate)
    // Real implementation would use proper coefficients
    float uv_approx = (uva + uvb) / 2.0;

    if (!isnan(uva) && !isnan(uvb) && uv_approx >= 0)
    {
      readings->uv_index = uv_approx;
      readings->uv_valid = true;
    }
  }
  else
  {
    readings->uv_index = -1.0;  // Unavailable on Rev2
  }

  // Determine overall validity (at least one sensor read successfully)
  if (readings->temp_valid ||
      readings->humidity_valid ||
      readings->pressure_valid ||
      readings->light_valid ||
      readings->uv_valid)
  {
    readings->valid = true;
    return true;
  }

  DEBUG_PRINTLN(F("⚠ All sensor reads failed"));
  return false;
}

/**
 * Check if sensors are ready
 */
bool areSensorsReady(void)
{
  return sensors_initialized;
}

/**
 * Format sensor readings as JSON
 */
char* formatSensorJSON(const SensorReadings* readings,
                       char* buffer, size_t buffer_size)
{
  if (!readings || !buffer || buffer_size < 50)
  {
    return NULL;
  }

  // Start building JSON (use temp_valid to know if we've added any fields)
  int offset = 0;
  buffer[offset++] = '{';

  // Add temperature if valid
  if (readings->temp_valid)
  {
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "\"temperature\":%.1f,", readings->temperature);
  }

  // Add humidity if valid
  if (readings->humidity_valid)
  {
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "\"humidity\":%.1f,", readings->humidity);
  }

  // Add pressure if valid
  if (readings->pressure_valid)
  {
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "\"pressure\":%.1f,", readings->pressure);
  }

  // Add illuminance if valid
  if (readings->light_valid)
  {
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "\"illuminance\":%.1f,", readings->illuminance);
  }

  // Add UV index if valid
  if (readings->uv_valid && readings->uv_index >= 0)
  {
    offset += snprintf(buffer + offset, buffer_size - offset,
                       "\"uv_index\":%.1f,", readings->uv_index);
  }

  // Always add timestamp
  offset += snprintf(buffer + offset, buffer_size - offset,
                     "\"timestamp\":%lu", readings->timestamp);

  // Close JSON
  if (offset + 1 < buffer_size)
  {
    buffer[offset++] = '}';
    buffer[offset] = '\0';
    return buffer;
  }

  // Buffer overflow
  return NULL;
}
