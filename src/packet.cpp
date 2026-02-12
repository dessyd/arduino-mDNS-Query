/**
 * ============================================================================
 * Packet Module - Implementation
 * ============================================================================
 * DNS packet building and parsing
 */

#include "packet.h"
#include "config.h"
#include <string.h>
#include <stdio.h>

// ============================================================================
// STATIC BUFFERS
// ============================================================================
static byte packetBuffer[PACKET_BUFFER_SIZE];
static uint16_t queryTransactionID = 0x1234;

// ============================================================================
// PUBLIC FUNCTIONS
// ============================================================================

bool buildServiceName(char *buffer, size_t maxLen)
{
  int len = snprintf(buffer, maxLen, "_%s._%s.%s",
                     CONFIG_MDNS_SERVICE_TYPE,
                     CONFIG_MDNS_PROTOCOL,
                     CONFIG_MDNS_DOMAIN);

  if (len < 0 || len >= (int)maxLen) {
    DEBUG_PRINTLN(F("✗ Service name too long!"));
    return false;
  }
  return true;
}

uint16_t encodeDomainName(const char *name, byte *encoded, uint16_t maxLen)
{
  if (!name || !encoded || maxLen < 2) {
    return 0;
  }

  uint16_t pos = 0;
  const char *start = name;

  for (const char *p = name; *p != '\0'; p++) {
    if (*p == '.') {
      uint16_t labelLen = p - start;

      // Check DNS label size limitation (max 63 bytes per RFC 1035)
      if (labelLen == 0 || labelLen > 63) {
        DEBUG_PRINTF(F("✗ Invalid label length: "), labelLen);
        return 0;
      }

      // Check buffer overflow
      if (pos + labelLen + 1 >= maxLen) {
        DEBUG_PRINTF(F("✗ Encoded name buffer overflow at pos "), pos);
        return 0;
      }

      // Write label length + label bytes
      encoded[pos++] = (byte)labelLen;
      memcpy(&encoded[pos], start, labelLen);
      pos += labelLen;

      start = p + 1;
    }
  }

  // Handle final label (after last dot)
  uint16_t labelLen = strlen(start);
  if (labelLen > 0) {
    if (labelLen > 63) {
      DEBUG_PRINTF(F("✗ Invalid final label length: "), labelLen);
      return 0;
    }

    if (pos + labelLen + 2 >= maxLen) {
      DEBUG_PRINTLN(F("✗ Encoded name buffer overflow at final label"));
      return 0;
    }

    encoded[pos++] = (byte)labelLen;
    memcpy(&encoded[pos], start, labelLen);
    pos += labelLen;
  }

  // Terminate with root label (0x00)
  if (pos >= maxLen) {
    DEBUG_PRINTLN(F("✗ No room for root label terminator"));
    return 0;
  }

  encoded[pos++] = 0x00;
  return pos;
}

uint16_t buildMDNSQuery(byte *packet, uint16_t maxLen, const char *serviceName)
{
  if (!packet || !serviceName || maxLen < 30) {
    return 0;
  }

  // Clear buffer
  memset(packet, 0, maxLen);

  // Build 12-byte header
  packet[0] = (queryTransactionID >> 8) & 0xFF;
  packet[1] = queryTransactionID & 0xFF;
  packet[2] = 0x00;  // Flags: standard query
  packet[3] = 0x00;
  packet[4] = 0x00;  // QDCOUNT = 1
  packet[5] = 0x01;
  packet[6] = 0x00;  // ANCOUNT = 0
  packet[7] = 0x00;
  packet[8] = 0x00;  // NSCOUNT = 0
  packet[9] = 0x00;
  packet[10] = 0x00; // ARCOUNT = 0
  packet[11] = 0x00;

  // Encode domain name
  uint16_t pos = 12;
  uint16_t nameLen = encodeDomainName(serviceName, &packet[pos], maxLen - pos);
  if (nameLen == 0) {
    DEBUG_PRINTLN(F("✗ Domain name encoding failed"));
    return 0;
  }
  pos += nameLen;

  // Add QTYPE (PTR = 12) and QCLASS (IN = 1)
  if (pos + 4 > maxLen) {
    return 0;
  }

  packet[pos++] = 0x00;
  packet[pos++] = DNS_TYPE_PTR;
  packet[pos++] = 0x00;
  packet[pos++] = DNS_CLASS_IN;

  DEBUG_PRINTF(F("✓ Built query: "), pos);
  DEBUG_PRINTLN(F(" bytes"));

  return pos;
}

bool decodeDNSName(const byte *packet, int packetSize, uint16_t offset,
                   char *name, uint16_t nameMaxLen, uint16_t& nextOffset)
{
  if (!packet || !name || offset >= packetSize) {
    return false;
  }

  uint16_t pos = offset;
  uint16_t namePos = 0;
  uint16_t jumps = 0;
  const uint16_t MAX_JUMPS = 10;
  bool jumped = false;

  while (pos < packetSize && namePos < nameMaxLen - 1) {
    byte len = packet[pos++];

    // End of name marker
    if (len == 0x00) {
      name[namePos] = '\0';
      if (!jumped) {
        nextOffset = pos;
      }
      return true;
    }

    // Compression pointer
    if ((len & 0xC0) == 0xC0) {
      if (pos >= packetSize) return false;

      uint16_t pointer = ((len & 0x3F) << 8) | packet[pos++];

      if (!jumped) {
        nextOffset = pos;
        jumped = true;
      }

      if (jumps++ >= MAX_JUMPS) {
        DEBUG_PRINTLN(F("✗ DNS compression pointer loop detected"));
        return false;
      }

      if (pointer >= packetSize) {
        DEBUG_PRINTLN(F("✗ DNS compression pointer out of bounds"));
        return false;
      }

      pos = pointer;
      continue;
    }

    // Label length must be less than 64 bytes
    if (len > 63) {
      DEBUG_PRINTF(F("✗ Invalid label length: "), len);
      return false;
    }

    // Add dot separator before label (except first label)
    if (namePos > 0 && namePos < nameMaxLen - 1) {
      name[namePos++] = '.';
    }

    // Copy label bytes
    if (pos + len > packetSize) {
      DEBUG_PRINTLN(F("✗ Label extends beyond packet"));
      return false;
    }

    for (uint16_t i = 0; i < len && namePos < nameMaxLen - 1; i++) {
      name[namePos++] = packet[pos++];
    }
  }

  name[namePos] = '\0';
  if (!jumped) {
    nextOffset = pos;
  }
  return true;
}

byte* getPacketBuffer(void)
{
  return packetBuffer;
}

uint16_t getPacketBufferSize(void)
{
  return PACKET_BUFFER_SIZE;
}

uint16_t getNextTransactionID(void)
{
  return ++queryTransactionID;
}
