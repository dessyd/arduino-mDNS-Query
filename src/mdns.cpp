/**
 * ============================================================================
 * mDNS Module - Implementation
 * ============================================================================
 * mDNS query and response handling
 */

#include "mdns.h"
#include "packet.h"
#include "network.h"
#include "config.h"
#include <string.h>
#include <stdio.h>

// ============================================================================
// STATIC STATE
// ============================================================================
static char lastRequestedService[CONFIG_SERVICE_NAME_MAX_LEN] = {0};
static DiscoveredConfig discoveredConfig = {{0}, 0, {0}, {0}, 0, {0}, false};
static IPAddress mdnsMulticastIP(224, 0, 0, 251);

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * Validate that response matches requested service
 */
static bool validateResponseService(const byte *packet, int packetSize)
{
  if (packetSize < 14) {
    return false;
  }

  char responseName[CONFIG_SERVICE_NAME_MAX_LEN];
  uint16_t pos = 12;
  uint16_t namePos = 0;

  // Decode domain name from response
  while (pos < packetSize && namePos < sizeof(responseName) - 1) {
    byte len = packet[pos++];

    if (len == 0x00) {
      responseName[namePos] = '\0';
      break;
    }

    if ((len & 0xC0) == 0xC0) {
      pos++;
      responseName[namePos] = '\0';
      break;
    }

    if (namePos > 0) {
      responseName[namePos++] = '.';
    }

    for (uint16_t i = 0; i < len && pos < packetSize && namePos < sizeof(responseName) - 1; i++) {
      responseName[namePos++] = packet[pos++];
    }
  }

  responseName[namePos] = '\0';

  if (strcmp(responseName, lastRequestedService) != 0) {
    DEBUG_PRINT(F("✗ Response service mismatch! Expected: "));
    DEBUG_PRINTLN(lastRequestedService);
    return false;
  }

  return true;
}

/**
 * Parse SRV record to extract hostname and port
 */
static bool parseSRVRecord(const byte *packet, int packetSize, uint16_t dataOffset,
                           uint16_t dataLength, char *hostname, uint16_t hostMaxLen,
                           uint16_t& port)
{
  if (dataLength < 6) {
    DEBUG_PRINTLN(F("✗ SRV record too small (need 6+ bytes)"));
    return false;
  }

  uint16_t pos = dataOffset + 4;
  port = ((uint16_t)packet[pos] << 8) | packet[pos + 1];
  pos += 2;

  DEBUG_PRINTF(F("  ✓ Port from SRV: "), port);

  uint16_t nextPos;
  if (!decodeDNSName(packet, packetSize, pos, hostname, hostMaxLen, nextPos)) {
    DEBUG_PRINTLN(F("✗ Failed to decode SRV target hostname"));
    return false;
  }

  DEBUG_PRINT(F("  ✓ Hostname from SRV: "));
  DEBUG_PRINTLN(hostname);

  return true;
}

/**
 * Parse TXT record to extract key-value pairs
 */
static bool parseTXTRecord(const byte *packet, uint16_t dataOffset, uint16_t dataLength,
                           char *path, uint16_t pathMaxLen,
                           char *version, uint16_t versionMaxLen)
{
  if (dataLength == 0 || dataLength > 512) {
    DEBUG_PRINTLN(F("✗ Invalid TXT record length"));
    return false;
  }

  uint16_t pos = dataOffset;
  uint16_t endPos = dataOffset + dataLength;
  bool foundPath = false;

  while (pos < endPos) {
    byte strLen = packet[pos++];

    if (strLen == 0) break;

    char txtString[128];
    uint16_t strPos = 0;

    for (byte i = 0; i < strLen && pos < endPos && strPos < sizeof(txtString) - 1; i++) {
      txtString[strPos++] = packet[pos++];
    }
    txtString[strPos] = '\0';

    if (strncmp(txtString, "path=", 5) == 0) {
      strncpy(path, &txtString[5], pathMaxLen - 1);
      path[pathMaxLen - 1] = '\0';
      DEBUG_PRINT(F("  ✓ Path from TXT: "));
      DEBUG_PRINTLN(path);
      foundPath = true;
    }

    if (strncmp(txtString, "version=", 8) == 0) {
      strncpy(version, &txtString[8], versionMaxLen - 1);
      version[versionMaxLen - 1] = '\0';
      DEBUG_PRINT(F("  ✓ Version from TXT: "));
      DEBUG_PRINTLN(version);
    }
  }

  return foundPath;
}

/**
 * Parse A record to extract IPv4 address
 */
static bool parseARecord(const byte *packet, uint16_t dataOffset,
                         uint32_t &ipAddress, char *ipStr, uint16_t strMaxLen)
{
  ipAddress = ((uint32_t)packet[dataOffset] << 24) |
              ((uint32_t)packet[dataOffset + 1] << 16) |
              ((uint32_t)packet[dataOffset + 2] << 8) |
              ((uint32_t)packet[dataOffset + 3]);

  snprintf(ipStr, strMaxLen, "%d.%d.%d.%d",
           packet[dataOffset],
           packet[dataOffset + 1],
           packet[dataOffset + 2],
           packet[dataOffset + 3]);

  DEBUG_PRINT(F("  ✓ IP from A record: "));
  DEBUG_PRINTLN(ipStr);

  return true;
}

/**
 * Parse all answer records from mDNS response
 */
static bool parseAnswerRecords(const byte *packet, int packetSize, uint16_t questionPos,
                               uint16_t ancount, DiscoveredConfig &config)
{
  uint16_t pos = questionPos;
  uint16_t recordsProcessed = 0;

  while (recordsProcessed < ancount && pos < packetSize) {
    uint16_t nameEnd;
    char recordName[CONFIG_HOSTNAME_MAX_LEN];

    if (!decodeDNSName(packet, packetSize, pos, recordName, sizeof(recordName), nameEnd)) {
      DEBUG_PRINTLN(F("✗ Failed to decode record name"));
      return false;
    }
    pos = nameEnd;

    if (pos + 10 > packetSize) {
      DEBUG_PRINTLN(F("✗ Record header extends beyond packet"));
      return false;
    }

    uint16_t recordType = (packet[pos] << 8) | packet[pos + 1];
    uint16_t recordClass = (packet[pos + 2] << 8) | packet[pos + 3];
    uint32_t ttl = ((uint32_t)packet[pos + 4] << 24) |
                   ((uint32_t)packet[pos + 5] << 16) |
                   ((uint16_t)packet[pos + 6] << 8) |
                   packet[pos + 7];
    uint16_t dataLength = (packet[pos + 8] << 8) | packet[pos + 9];

    pos += 10;

    DEBUG_PRINT(F("\n  Record: "));
    DEBUG_PRINT(recordName);
    DEBUG_PRINT(F(" Type="));
    DEBUG_PRINT(recordType);
    DEBUG_PRINT(F(" Class="));
    DEBUG_PRINT(recordClass);
    DEBUG_PRINT(F(" TTL="));
    DEBUG_PRINT(ttl);
    DEBUG_PRINT(F(" Length="));
    DEBUG_PRINTLN(dataLength);

    if (pos + dataLength > packetSize) {
      DEBUG_PRINTLN(F("✗ Record data extends beyond packet"));
      return false;
    }

    // Parse based on record type
    if (recordType == 33) {  // SRV record
      DEBUG_PRINTLN(F("  → Parsing SRV record"));
      parseSRVRecord(packet, packetSize, pos, dataLength,
                     config.hostname, sizeof(config.hostname),
                     config.port);
    }
    else if (recordType == 16) {  // TXT record
      DEBUG_PRINTLN(F("  → Parsing TXT record"));
      parseTXTRecord(packet, pos, dataLength,
                     config.path, sizeof(config.path),
                     config.version, sizeof(config.version));
    }
    else if (recordType == 1) {  // A record
      if (dataLength == 4) {
        DEBUG_PRINTLN(F("  → Parsing A record"));
        parseARecord(packet, pos, config.ipAddress,
                    config.ipStr, sizeof(config.ipStr));
      }
    }

    pos += dataLength;
    recordsProcessed++;
  }

  // Validate we got required fields
  if (config.port > 0 && config.path[0] != '\0' && config.ipStr[0] != '\0') {
    config.valid = true;
    DEBUG_PRINTLN(F("\n✓ Config extraction complete!"));
    return true;
  }

  DEBUG_PRINTLN(F("\n⚠ Incomplete config (missing required fields)"));
  return false;
}

/**
 * Build HTTP URL from discovered configuration
 */
static bool buildConfigURL(const DiscoveredConfig &config, char *url, uint16_t maxLen)
{
  if (!config.valid || config.ipStr[0] == '\0' || config.path[0] == '\0') {
    DEBUG_PRINTLN(F("✗ Cannot build URL - config incomplete"));
    return false;
  }

  int len = snprintf(url, maxLen, "http://%s:%u%s",
                     config.ipStr, config.port, config.path);

  if (len < 0 || len >= (int)maxLen) {
    DEBUG_PRINTLN(F("✗ URL buffer overflow"));
    return false;
  }

  DEBUG_PRINT(F("✓ Config URL: "));
  DEBUG_PRINTLN(url);

  return true;
}

// ============================================================================
// PUBLIC FUNCTIONS
// ============================================================================

bool sendMDNSQuery(void)
{
  static char serviceName[CONFIG_SERVICE_NAME_MAX_LEN];

  if (!buildServiceName(serviceName, sizeof(serviceName))) {
    return false;
  }

  byte *packetBuffer = getPacketBuffer();
  uint16_t querySize = buildMDNSQuery(packetBuffer, getPacketBufferSize(), serviceName);
  if (querySize == 0) {
    DEBUG_PRINTLN(F("✗ Failed to build query"));
    return false;
  }

  WiFiUDP& udp = getUDPSocket();
  udp.beginPacket(mdnsMulticastIP, CONFIG_MDNS_PORT);
  udp.write(packetBuffer, querySize);
  if (!udp.endPacket()) {
    DEBUG_PRINTLN(F("✗ Failed to send mDNS query"));
    return false;
  }

  DEBUG_PRINT(F("✓ Sent mDNS query for: "));
  DEBUG_PRINTLN(serviceName);

  strncpy(lastRequestedService, serviceName, sizeof(lastRequestedService) - 1);
  lastRequestedService[sizeof(lastRequestedService) - 1] = '\0';

  return true;
}

void handleMDNSResponse(int packetSize)
{
  if (packetSize < 12) {
    DEBUG_PRINTLN(F("⚠ Packet too small for DNS header"));
    return;
  }

  byte *packetBuffer = getPacketBuffer();
  WiFiUDP& udp = getUDPSocket();

  int bytesRead = udp.read(packetBuffer, getPacketBufferSize());
  if (bytesRead < 12) {
    DEBUG_PRINTLN(F("⚠ Failed to read DNS header"));
    return;
  }

  if (!validateResponseService(packetBuffer, bytesRead)) {
    return;
  }

  uint16_t flags = (packetBuffer[2] << 8) | packetBuffer[3];
  uint16_t ancount = (packetBuffer[6] << 8) | packetBuffer[7];

  if (!(flags & 0x8000)) {
    DEBUG_PRINTLN(F("⚠ Received query, not response - ignoring"));
    return;
  }

  if (ancount > 0) {
    DEBUG_PRINT(F("✓ mDNS Response received with "));
    DEBUG_PRINT(ancount);
    DEBUG_PRINTLN(F(" answer records"));

    // Find end of question section
    uint16_t questionPos = 12;
    while (questionPos < bytesRead) {
      byte len = packetBuffer[questionPos++];

      if (len == 0x00) {
        break;
      }

      if ((len & 0xC0) == 0xC0) {
        questionPos++;
        break;
      }

      questionPos += len;
    }

    questionPos += 4;  // Skip QTYPE and QCLASS

    if (questionPos < bytesRead) {
      if (parseAnswerRecords(packetBuffer, bytesRead, questionPos, ancount, discoveredConfig)) {
        char configURL[CONFIG_URL_MAX_LEN];
        buildConfigURL(discoveredConfig, configURL, sizeof(configURL));
      }
    } else {
      DEBUG_PRINTLN(F("⚠ Question section extends beyond packet"));
    }
  }
}

const DiscoveredConfig* getDiscoveredConfig(void)
{
  return &discoveredConfig;
}
