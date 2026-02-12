/**
 * ============================================================================
 * Packet Module Header
 * ============================================================================
 * DNS packet building, encoding, and decoding functions
 */

#ifndef PACKET_H
#define PACKET_H

#include <Arduino.h>
#include <stdint.h>
#include "arduino_configs.h"

/**
 * Build mDNS service name from configuration
 *
 * Constructs full service name: "_<service>._<protocol>.<domain>"
 * Example: "_http._tcp.local"
 *
 * PARAMETERS:
 *   buffer - Output buffer for service name
 *   maxLen - Maximum buffer size
 *
 * RETURNS:
 *   true if successful, false if buffer overflow
 */
bool buildServiceName(char *buffer, size_t maxLen);

/**
 * Encode domain name to DNS wire format
 *
 * Converts "example.local" to length-delimited labels:
 *   07 65 78 61 6d 70 6c 65 05 6c 6f 63 61 6c 00
 *
 * PARAMETERS:
 *   name     - Plain text domain name
 *   encoded  - Output buffer for DNS-encoded name
 *   maxLen   - Maximum length of encoded buffer
 *
 * RETURNS:
 *   Number of bytes written (0 on error)
 */
uint16_t encodeDomainName(const char *name, byte *encoded, uint16_t maxLen);

/**
 * Build complete mDNS PTR query packet
 *
 * Packet structure:
 *   [12-byte header] [encoded domain] [QTYPE=PTR] [QCLASS=IN]
 *
 * PARAMETERS:
 *   packet       - Output buffer for complete query packet
 *   maxLen       - Maximum buffer size
 *   serviceName  - Service name to query for
 *
 * RETURNS:
 *   Total packet size in bytes (0 on error)
 */
uint16_t buildMDNSQuery(byte *packet, uint16_t maxLen, const char *serviceName);

/**
 * Decode DNS domain name from wire format
 *
 * Handles compression pointers (0xC0 prefix) as per RFC 1035.
 * Converts "05 65 78 61 6d 70 6c 65 05 6c 6f 63 61 6c 00" to "example.local"
 *
 * PARAMETERS:
 *   packet      - Packet buffer
 *   packetSize  - Total packet size
 *   offset      - Starting position in packet
 *   name        - Output buffer for decoded name
 *   nameMaxLen  - Maximum size of output buffer
 *   nextOffset  - [output] Position after decoded name
 *
 * RETURNS:
 *   true if name decoded successfully, false on error
 */
bool decodeDNSName(const byte *packet, int packetSize, uint16_t offset,
                   char *name, uint16_t nameMaxLen, uint16_t &nextOffset);

/**
 * Get or allocate packet buffer for send/receive operations
 *
 * RETURNS:
 *   Pointer to static packet buffer
 */
byte* getPacketBuffer(void);

/**
 * Get size of packet buffer
 *
 * RETURNS:
 *   Size in bytes
 */
uint16_t getPacketBufferSize(void);

#endif  // PACKET_H
