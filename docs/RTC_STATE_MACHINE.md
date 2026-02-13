# RTC (Real-Time Clock) State Machine Documentation

## Overview

The Arduino RTC module implements a state machine that tracks the synchronization status of the real-time clock with network time from WiFiNINA. This document describes the four states, transitions, and behavior.

---

## State Machine Diagram

```mermaid
stateDiagram-v2
    [*] --> RTC_UNINITIALIZED

    RTC_UNINITIALIZED --> RTC_INITIALIZED: initRTC() called<br/>(setup phase)

    RTC_INITIALIZED --> RTC_SYNCED: syncRTCWithNetwork()<br/>succeeds<br/>(WiFi connected,<br/>got network time)

    RTC_INITIALIZED --> RTC_INITIALIZED: syncRTCWithNetwork()<br/>fails<br/>(WiFi not ready,<br/>rate limit, or no NTP)

    RTC_SYNCED --> RTC_SYNCED: syncRTCWithNetwork()<br/>succeeds<br/>(refresh time)

    RTC_SYNCED --> RTC_SYNCED: syncRTCWithNetwork()<br/>fails<br/>(but last sync recent)

    RTC_SYNCED --> RTC_SYNC_STALE: 5+ minutes since<br/>last successful sync<br/>(detected in<br/>getRTCTimestamp() or<br/>getRTCStatus())

    RTC_SYNC_STALE --> RTC_SYNCED: syncRTCWithNetwork()<br/>succeeds<br/>(refresh time)

    RTC_SYNC_STALE --> RTC_SYNC_STALE: syncRTCWithNetwork()<br/>fails<br/>(still stale)

    RTC_INITIALIZED --> RTC_SYNC_STALE: 5+ minutes since<br/>boot without sync
```text

---

## State Definitions

### 1. RTC_UNINITIALIZED

**Entry Point:** Device boot (before `initRTC()` called)

**Characteristics:**

- RTC hardware not yet initialized
- No timestamp available
- Initial state

**Transitions:**

- → **RTC_INITIALIZED** when `initRTC()` is called in setup()

**Code:**

```cpp
static RTCStatus rtc_status = RTC_UNINITIALIZED;  // Line 12, rtc.cpp
```text

---

### 2. RTC_INITIALIZED

**Entry:** `initRTC()` called successfully

**Characteristics:**

- RTC hardware started
- Time set to epoch 0 (January 1, 1970 00:00:00 UTC)
- Waiting for network synchronization
- Timestamp: `0` (or very small value)

**When Used:**

- Device just booted
- WiFi not yet connected
- WiFiNINA doesn't have network time yet (no NTP response)

**Timestamp Behavior:**

```text
getRTCTimestamp() → rtc.getEpoch() → 0 (seconds since 1970)
```text

**Transitions:**

- → **RTC_SYNCED** when `syncRTCWithNetwork()` succeeds
  - Condition: WiFi connected AND WiFiNINA has network time
- → **RTC_SYNC_STALE** if 5+ minutes elapsed without sync

**Code:**

```cpp
bool initRTC(void) {
  rtc.begin();
  rtc.setEpoch(0);           // Start at epoch 0
  rtc_status = RTC_INITIALIZED;
  return true;
}
```text

---

### 3. RTC_SYNCED

**Entry:** Successful network time synchronization

**Characteristics:**

- RTC synchronized with WiFiNINA network time
- Last sync within 5 minutes (FRESH)
- Timestamp: Accurate wall-clock Unix timestamp
- `last_sync_time`: Recorded at moment of sync

**When Used:**

- Device has WiFi connection
- WiFiNINA successfully obtained NTP time
- Publishing sensor data with accurate timestamps

**Timestamp Behavior:**

```text
getRTCTimestamp() → rtc.getEpoch() → Accurate wall-clock time
Example: 1739435810 (Feb 13, 2026 14:30:10 UTC)
```text

**Staleness Check:**

```cpp
uint32_t now = millis();
if (rtc_status == RTC_SYNCED && (now - last_sync_time > CONFIG_RTC_STALE_THRESHOLD_MS)) {
  rtc_status = RTC_SYNC_STALE;  // Becomes stale after 5 minutes
}
```text

**Transitions:**

- → **RTC_SYNCED** (refresh) when `syncRTCWithNetwork()` succeeds again
  - Updates `last_sync_time` to current millis()
  - Re-syncs RTC with fresh network time
- → **RTC_SYNC_STALE** when 5+ minutes elapsed without new sync
  - Automatic transition (checked in `getRTCTimestamp()` or `getRTCStatus()`)
- Stays **RTC_SYNCED** if `syncRTCWithNetwork()` fails (but still fresh)
  - Example: WiFi temporarily unavailable but within 5-minute window

**Code:**

```cpp
bool syncRTCWithNetwork(void) {
  unsigned long wifi_time = WiFi.getTime();
  if (wifi_time != 0) {
    rtc.setEpoch(wifi_time);
    last_sync_time = millis();
    rtc_status = RTC_SYNCED;     // ← Transition here
    return true;
  }
  return false;
}
```text

---

### 4. RTC_SYNC_STALE

**Entry:** 5+ minutes elapsed since last successful sync

**Characteristics:**

- RTC has synced data but outdated
- Time value still valid but not current
- Last sync more than 5 minutes ago
- WARNING: timestamp may have drifted

**When Used:**

- Temporary WiFi outage
- Network connection lost but device still running
- Time since last sync > CONFIG_RTC_STALE_THRESHOLD_MS (300,000ms)

**Timestamp Behavior:**

```text
getRTCTimestamp() → rtc.getEpoch() → Last synced value (may have drifted)
Accuracy: Approximately correct, but drifting with each second
```text

**Drift Calculation:**

- RTC drifts ~5-15 seconds per day (typical SAMD21)
- After 1 hour stale: ~0.2 seconds drift
- After 24 hours stale: ~5-15 seconds drift
- Still accurate to within minutes for most use cases

**Staleness Detection:**

```cpp
// Detected automatically when getting timestamp
if (rtc_status == RTC_SYNCED && (now - last_sync_time > 300000)) {
  rtc_status = RTC_SYNC_STALE;  // ← Transition happens here
}
```text

**Transitions:**

- → **RTC_SYNCED** when `syncRTCWithNetwork()` succeeds
  - Refreshes time and resets sync timer
- Stays **RTC_SYNC_STALE** if `syncRTCWithNetwork()` fails
  - Continues using last synced (drifting) time

**Code:**

```cpp
uint32_t getRTCTimestamp(void) {
  uint32_t now = millis();
  if (rtc_status == RTC_SYNCED && (now - last_sync_time > CONFIG_RTC_STALE_THRESHOLD_MS)) {
    rtc_status = RTC_SYNC_STALE;  // ← Mark as stale
  }
  return rtc.getEpoch();          // Still returns valid time
}
```text

---

## State Transition Conditions

### Transition: INITIALIZED → SYNCED

```text
Condition: syncRTCWithNetwork() returns true
Required:
  1. WiFi.status() == WL_CONNECTED
  2. WiFi.getTime() != 0 (NTP response available)
  3. Rate limit satisfied (60 second interval)

Action:
  - rtc.setEpoch(wifi_time)
  - last_sync_time = millis()
  - rtc_status = RTC_SYNCED
  - Debug output: "✓ RTC synced with network time"
```text

### Transition: SYNCED → STALE

```text
Condition: (now - last_sync_time) > CONFIG_RTC_STALE_THRESHOLD_MS
Where:
  - now = millis() (uptime in milliseconds)
  - last_sync_time = millis() at last successful sync
  - CONFIG_RTC_STALE_THRESHOLD_MS = 300,000 (5 minutes)

Automatic:
  - Checked in getRTCTimestamp() (called every 10 seconds when publishing)
  - Checked in getRTCStatus() (called on demand)
  - No action required, transition happens silently
```text

### Transition: STALE → SYNCED

```text
Condition: syncRTCWithNetwork() returns true
Required: Same as INITIALIZED → SYNCED
Action: Same as INITIALIZED → SYNCED
Effect: Resets "stale" marker, refreshes time
```text

---

## Rate Limiting

Rate limiting prevents excessive sync attempts and reduces power consumption.

```cpp
if (now - last_sync_time < CONFIG_RTC_SYNC_INTERVAL_MS) {
  return false;  // Too soon, skip sync
}
```text

**Configuration:**

- `CONFIG_RTC_SYNC_INTERVAL_MS = 60000` (60 seconds)
- Sync attempted every 60 seconds maximum
- Actual sync only succeeds if WiFi + NTP available

**Timeline:**

```text
T=0s:   Device boots
T=0s:   initRTC() → RTC_INITIALIZED
T=0-60s: syncRTCWithNetwork() called, but rate limited (returns false)
T=60s:   First actual sync attempt
  - If WiFi connected: RTC_SYNCED
  - If WiFi not ready: stays RTC_INITIALIZED
T=120s:  Second sync attempt
T=60+300s (360s): Staleness timer starts (only if RTC_SYNCED)
T=360s:  If no refresh, RTC_SYNCED → RTC_SYNC_STALE
```text

---

## Configuration Values

```cpp
// arduino_configs.h

// Sync interval (milliseconds)
// Only attempt sync every 60 seconds
#define CONFIG_RTC_SYNC_INTERVAL_MS 60000

// Staleness threshold (milliseconds)
// Mark as stale after 5 minutes without sync
#define CONFIG_RTC_STALE_THRESHOLD_MS 300000

// Bootstrap timestamp (no longer used, kept for reference)
// Default: 2026-02-13 00:00:00 UTC (Unix: 1739404800)
#define CONFIG_RTC_BOOTSTRAP_TIMESTAMP 1739404800UL
```text

---

## API Functions

### `bool initRTC(void)`

**Purpose:** Initialize RTC hardware
**Entry State:** RTC_UNINITIALIZED
**Exit State:** RTC_INITIALIZED
**Called:** Once in setup()

### `bool syncRTCWithNetwork(void)`

**Purpose:** Attempt to sync with WiFiNINA network time
**Entry State:** RTC_INITIALIZED or RTC_SYNCED or RTC_SYNC_STALE
**Exit State:** RTC_SYNCED (if success) or unchanged (if fail)
**Called:** Every loop cycle (non-blocking, rate-limited)
**Returns:**

- `true` = sync succeeded, RTC updated
- `false` = sync skipped (rate limit, no WiFi, no NTP)

### `uint32_t getRTCTimestamp(void)`

**Purpose:** Get current Unix timestamp
**Entry State:** Any state
**Exit State:** May transition SYNCED → STALE
**Returns:** Unix timestamp (0 = epoch, larger = more recent)
**Called:** Every sensor read (~10 seconds)
**Side Effect:** Updates RTC status (staleness check)

### `RTCStatus getRTCStatus(void)`

**Purpose:** Get current RTC synchronization status
**Entry State:** Any state
**Exit State:** May transition SYNCED → STALE
**Returns:** One of four RTCStatus enum values
**Called:** On demand (optional for monitoring)
**Side Effect:** Updates RTC status (staleness check)

---

## Practical Examples

### Example 1: Device Boot with Immediate WiFi

```text
T=0ms:    [Boot] → RTC_UNINITIALIZED
T=50ms:   initRTC() called in setup()
          → rtc.setEpoch(0)
          → rtc_status = RTC_INITIALIZED
          → returns true

T=100ms:  Main loop starts
          syncRTCWithNetwork() called
          → Rate limit check: 100ms < 60000ms → returns false

T=60,000ms: (60 seconds) Sync attempt #1
            syncRTCWithNetwork() called
            → Rate limit passed ✓
            → WiFi.status() == WL_CONNECTED ✓
            → WiFi.getTime() = 1739435810 ✓
            → rtc.setEpoch(1739435810)
            → rtc_status = RTC_SYNCED
            → last_sync_time = 60000
            → returns true

T=70,000ms: readSensors() calls getRTCTimestamp()
            → Staleness check: (70000 - 60000) = 10000ms < 300000ms
            → Still SYNCED ✓
            → returns 1739435810

T=360,000ms: (300 seconds after first sync) readSensors()
             → Staleness check: (360000 - 60000) = 300000ms == threshold
             → rtc_status = RTC_SYNC_STALE
             → returns 1739435810 (but now stale)
```text

### Example 2: WiFi Drops During Operation

```text
T=60,000ms: RTC_SYNCED (WiFi connected)
            last_sync_time = 60000

T=100,000ms: WiFi connection lost
             syncRTCWithNetwork() called
             → WiFi.status() != WL_CONNECTED
             → returns false (no sync)
             → rtc_status stays RTC_SYNCED

T=120,000ms: syncRTCWithNetwork() called
             → WiFi.status() != WL_CONNECTED
             → returns false (no sync)
             → rtc_status stays RTC_SYNCED

T=360,000ms: readSensors() calls getRTCTimestamp()
             → Staleness check: (360000 - 60000) = 300000ms == threshold
             → rtc_status = RTC_SYNC_STALE ⚠️
             → Time has drifted ~0.2 seconds since loss

T=400,000ms: WiFi reconnects
             syncRTCWithNetwork() called
             → WiFi.status() == WL_CONNECTED ✓
             → WiFi.getTime() = 1739435850 ✓
             → rtc.setEpoch(1739435850)
             → rtc_status = RTC_SYNCED ✓
             → last_sync_time = 400000
             → returns true
```text

### Example 3: No WiFi at All

```text
T=0ms:      [Boot] → RTC_UNINITIALIZED

T=50ms:     initRTC()
            → RTC_INITIALIZED (time = 0)

T=10,000ms: syncRTCWithNetwork() called
            → WiFi.status() != WL_CONNECTED
            → returns false
            → RTC stays INITIALIZED

T=60,000ms: syncRTCWithNetwork() called (rate limit passed)
            → WiFi.status() != WL_CONNECTED
            → returns false
            → RTC stays INITIALIZED

T=120,000ms: readSensors() calls getRTCTimestamp()
             → getRTCTimestamp() returns rtc.getEpoch() = 0
             → Sensor timestamp = 0 (epoch)
             ⚠️ Not useful for real timestamps
```text

---

## Debug Output

### Successful Flow

```text
=== RTC INITIALIZATION ===
→ Initializing RTCZero...
✓ RTCZero initialized

[later, after 60 seconds with WiFi]

✓ RTC synced with network time: 1739435810
```text

### WiFi Unavailable

```text
=== RTC INITIALIZATION ===
→ Initializing RTCZero...
✓ RTCZero initialized

[Every 60 seconds]
✗ WiFi not connected (can't sync)

[At 5+ minutes]
⚠ RTC sync stale (last: 1739435810)
```text

---

## Testing the State Machine

### Test 1: Check initialization

```cpp
Serial.print("RTC Status: ");
Serial.println(getRTCStatus());
// Expected: RTC_INITIALIZED (0)
```text

### Test 2: Check sync success

```cpp
delay(65000);  // Wait for rate limit
// Should see: "✓ RTC synced with network time: [timestamp]"
Serial.println(getRTCStatus());
// Expected: RTC_SYNCED (2)
```text

### Test 3: Check staleness after WiFi loss

```cpp
// Disconnect WiFi
delay(301000);  // Wait 5+ minutes
Serial.println(getRTCTimestamp());
Serial.println(getRTCStatus());
// Expected: RTC_SYNC_STALE (3)
```text

---

## Summary

| State                | Time Value       | Accuracy   | Use Case           |
| -------------------- | ---------------- | ---------- | ------------------ |
| **UNINITIALIZED**    | N/A              | ❌ No      | Pre-boot only      |
| **INITIALIZED**      | 0 (epoch)        | ❌ No      | Waiting for WiFi   |
| **SYNCED**           | Accurate         | ✅ Yes     | Normal operation   |
| **STALE**            | Accurate but old | ⚠️ Maybe   | WiFi lost, <5min   |

The state machine ensures that:

1. ✅ RTC always has a valid value (never crashes)
2. ✅ Timestamps are fresh and accurate when synced
3. ✅ Graceful degradation when WiFi unavailable
4. ✅ Non-blocking sync prevents timing issues
5. ✅ Staleness detection flags unreliable timestamps
