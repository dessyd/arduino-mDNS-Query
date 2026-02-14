/**
 * ============================================================================
 * Real-Time Clock (RTC) Management Module
 * ============================================================================
 * Interfaces with Arduino MKR WiFi 1010 RTCZero for absolute timestamps
 * Syncs time via WiFiNINA.getTime() (NTP-sourced by WiFiNINA internally)
 *
 * PLATFORM: Arduino MKR WiFi 1010 (SAMD21) with built-in RTC
 * SYNC METHOD: Non-blocking periodic sync from WiFiNINA network time
 *
 * ============================================================================
 */

#ifndef RTC_H
#define RTC_H

#include <Arduino.h>

/**
 * RTC Status Enumeration
 * Tracks synchronization state with network time
 */
typedef enum {
  RTC_UNINITIALIZED,  // RTC not yet initialized
  RTC_INITIALIZED,    // RTC running but not synced with network
  RTC_SYNCED,         // RTC synced with network time (most recent)
  RTC_SYNC_STALE      // RTC synced but last sync >5 minutes ago
} RTCStatus;

/**
 * Initialize RTC hardware
 * Starts the real-time clock at epoch 0 (Jan 1, 1970)
 * RTC will be synced with network time via WiFiNINA on first successful sync
 *
 * Returns:
 *   RTC_INITIALIZED if successful
 *   RTC_UNINITIALIZED if initialization failed
 */
RTCStatus initRTC(void);

/**
 * Synchronize RTC with network time from WiFiNINA
 * Non-blocking operation - should be called periodically from main loop
 *
 * Returns:
 *   RTC_SYNCED if sync successful (time updated from network)
 *   RTC_SYNC_STALE if sync was skipped (too soon or WiFi unavailable)
 *   RTC_INITIALIZED if unable to sync but RTC is running
 *
 * Behavior:
 *   - Only syncs if WiFi is connected
 *   - Only syncs every SYNC_INTERVAL_MS to avoid excessive updates
 *   - Updates internal state for status tracking
 */
RTCStatus syncRTCWithNetwork(void);

/**
 * Get current Unix timestamp from RTC
 *
 * Returns:
 *   Unix timestamp (seconds since Jan 1, 1970)
 *   Falls back to millis()/1000 if RTC not synced
 */
uint32_t getRTCTimestamp(void);

/**
 * Get RTC synchronization status
 *
 * Returns:
 *   RTCStatus enum value indicating sync state
 */
RTCStatus getRTCStatus(void);

/**
 * Format RTC timestamp for human-readable output
 * Used for debug logging
 *
 * Parameters:
 *   - timestamp: Unix timestamp to format
 *   - buffer: Output buffer for formatted string
 *   - buffer_size: Maximum buffer size
 *
 * Returns:
 *   Pointer to buffer on success
 *   NULL if parameters invalid or buffer too small
 *
 * Format example:
 *   "2026-02-13 14:35:22"
 */
char* formatRTCTime(uint32_t timestamp, char* buffer, size_t buffer_size);

#endif  // RTC_H
