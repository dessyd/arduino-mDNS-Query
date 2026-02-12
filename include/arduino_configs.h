
// ===============================
// Configuration Debug
// ===============================

// Debug: Messages de debug activés
#ifndef DEBUG
#define DEBUG true // FALSE pour production - économise mémoire et CPU
#endif


// ===============================
// Configuration Réseau
// ===============================

// Port UDP local pour mDNS
#ifndef CONFIG_LOCAL_UDP_PORT
#define CONFIG_LOCAL_UDP_PORT 5354
#endif

// Port mDNS standard
#ifndef CONFIG_MDNS_PORT
#define CONFIG_MDNS_PORT 5353
#endif

// ===============================
// Configuration mDNS - Service Discovery
// ===============================

// Type de service à rechercher - PRODUCTION: Plus générique

#ifndef CONFIG_MDNS_SERVICE_TYPE
#define CONFIG_MDNS_SERVICE_TYPE "config" // Générique pour compatibilité maximale
#endif

// Protocol pour le service mDNS
#ifndef CONFIG_MDNS_PROTOCOL
#define CONFIG_MDNS_PROTOCOL "tcp"
#endif

// Domaine mDNS
#ifndef CONFIG_MDNS_DOMAIN
#define CONFIG_MDNS_DOMAIN "local"
#endif