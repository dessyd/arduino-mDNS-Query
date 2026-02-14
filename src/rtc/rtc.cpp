#include <Arduino.h>
#include <RTCZero.h>
#include <WiFiNINA.h>
#include "rtc/rtc.h"
#include "arduino_configs.h"

// ============================================================================
// STATIC STATE - RTC Initialization and Synchronization
// ============================================================================

static RTCZero rtc;
static RTCStatus rtc_status = RTC_UNINITIALIZED;
static uint32_t last_sync_time = 0;
static uint32_t last_sync_timestamp = 0;

// Note: Sync interval and bootstrap timestamp are now in arduino_configs.h
// CONFIG_RTC_CONFIG_RTC_SYNC_INTERVAL_MS and CONFIG_RTC_CONFIG_RTC_BOOTSTRAP_TIMESTAMP

// ============================================================================
// PUBLIC API IMPLEMENTATION
// ============================================================================

/**
 * Initialize RTC hardware
 */
RTCStatus initRTC(void)
{
  DEBUG_PRINTLN(F(""));
  DEBUG_PRINTLN(F("=== RTC INITIALIZATION ==="));
  DEBUG_PRINTLN(F("→ Initializing RTCZero..."));

  // Initialize RTC
  rtc.begin();

  // RTC starts at epoch 0 (Jan 1, 1970)
  // Will be synced with network time via WiFiNINA on first successful sync
  rtc.setEpoch(0);

  rtc_status = RTC_INITIALIZED;
  last_sync_timestamp = 0;

  DEBUG_PRINTLN(F("✓ RTCZero initialized"));

  return RTC_INITIALIZED;
}

/**
 * Synchronize RTC with network time from WiFiNINA
 */
RTCStatus syncRTCWithNetwork(void)
{
  uint32_t now = millis();

  // Rate-limit sync attempts (only sync every CONFIG_RTC_SYNC_INTERVAL_MS)
  if (now - last_sync_time < CONFIG_RTC_SYNC_INTERVAL_MS)
  {
    return rtc_status;  // Too soon to sync again, return current status
  }

  // Check if WiFi is connected
  if (WiFi.status() != WL_CONNECTED)
  {
    return rtc_status;  // Can't sync without WiFi, return current status
  }

  // Get current time from WiFiNINA
  unsigned long wifi_time = WiFi.getTime();

  if (wifi_time == 0)
  {
    // WiFiNINA doesn't have time yet (no NTP response)
    return rtc_status;  // No time available, return current status
  }

  // Update RTC with network time
  rtc.setEpoch(wifi_time);

  last_sync_time = now;
  last_sync_timestamp = wifi_time;
  rtc_status = RTC_SYNCED;

  DEBUG_PRINT(F("✓ RTC synced with network time: "));
  DEBUG_PRINTLN(wifi_time);

  return RTC_SYNCED;
}

/**
 * Get current Unix timestamp from RTC
 */
uint32_t getRTCTimestamp(void)
{
  // Check if sync is stale (>5 minutes old)
  uint32_t now = millis();
  if (rtc_status == RTC_SYNCED && (now - last_sync_time > CONFIG_RTC_STALE_THRESHOLD_MS))
  {
    rtc_status = RTC_SYNC_STALE;
  }

  // Return current RTC time (starts at 0, updated via network sync)
  return rtc.getEpoch();
}

/**
 * Get RTC synchronization status
 */
RTCStatus getRTCStatus(void)
{
  // Update staleness check
  if (rtc_status == RTC_SYNCED)
  {
    uint32_t now = millis();
    if (now - last_sync_time > CONFIG_RTC_STALE_THRESHOLD_MS)
    {
      rtc_status = RTC_SYNC_STALE;
    }
  }

  return rtc_status;
}

/**
 * Format RTC timestamp for human-readable output
 */
char* formatRTCTime(uint32_t timestamp, char* buffer, size_t buffer_size)
{
  if (!buffer || buffer_size < 20)
  {
    return NULL;
  }

  // Convert Unix timestamp to date/time components
  // Simplified approach: approximate calculation
  uint32_t days_since_epoch = timestamp / 86400;
  uint32_t secs_today = timestamp % 86400;

  uint8_t hour = secs_today / 3600;
  uint8_t minute = (secs_today % 3600) / 60;
  uint8_t second = secs_today % 60;

  // Approximate year calculation (good for 2020-2030)
  // Days per year: 365 or 366 for leap years
  uint16_t year = 1970;
  uint32_t remaining_days = days_since_epoch;

  while (remaining_days > 0)
  {
    uint32_t days_in_year = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ? 366 : 365;
    if (remaining_days >= days_in_year)
    {
      remaining_days -= days_in_year;
      year++;
    }
    else
    {
      break;
    }
  }

  // Approximate month/day calculation
  // This is simplified - good enough for display purposes
  uint8_t month = 1;
  uint8_t day = remaining_days + 1;
  static const uint8_t daysPerMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  bool isLeapYear = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));

  for (uint8_t m = 0; m < 12; m++)
  {
    uint8_t daysInMonth = daysPerMonth[m];
    if (m == 1 && isLeapYear)
    {
      daysInMonth = 29;
    }

    if (day <= daysInMonth)
    {
      month = m + 1;
      break;
    }

    day -= daysInMonth;
  }

  // Format as YYYY-MM-DD HH:MM:SS
  snprintf(buffer, buffer_size,
           "%04u-%02u-%02u %02u:%02u:%02u",
           year, month, day, hour, minute, second);

  return buffer;
}
