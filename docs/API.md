# Documentation API - Serveur de Configuration

## Overview

API REST pour la gestion centralisée des configurations des devices Arduino/ESP32.

**Base URL**: `http://localhost:5050`

**Service mDNS**: `_config._tcp.local` (port 5050)

---

## Endpoints

### 1. Health Check

Vérifie l'état du serveur.

```text
GET /health
```

**Response (200 OK)**:

```json
{
  "status": "healthy",
  "service": "config-server"
}
```

---

### 2. Récupérer Configuration(s)

#### 2a. Récupérer la config d'UN device

```text
GET /config?device_id=ADC40A24&mac=A8:8F:AD:C4:0A:24
```

**Query Parameters**:

- `device_id` (required): ID du device (hexadécimal, 8-64 chars)
- `mac` (optional): Adresse MAC pour validation (format: XX:XX:XX:XX:XX:XX)

**Response (200 OK)**:

```json
{
  "device_id": "ADC40A24",
  "mac": "A8:8F:AD:C4:0A:24",
  "created_at": "2025-02-08T14:30:00Z",
  "updated_at": "2025-02-08T14:30:00Z",
  "config": {
    "mqtt_broker": "192.168.1.15",
    "mqtt_port": 1883,
    "mqtt_topic": "home/devices/config_client_A1B2C3/data",
    "poll_frequency_sec": 60,
    "heartbeat_frequency_sec": 30,
    "template": "sensor-v2"
  }
}
```

**Erreurs**:

- `404 Not Found`: Device n'existe pas
- `400 Bad Request`: MAC invalide ou device_id invalide

---

#### 2b. Récupérer toutes les configs

```text
GET /config
```

**Response (200 OK)**:

```json
{
  "devices": [
    {
      "device_id": "ADC40A24",
      "mac": "A8:8F:AD:C4:0A:24",
      "created_at": "2025-02-08T14:30:00Z",
      "updated_at": "2025-02-08T14:30:00Z",
      "config": { /* ... */ }
    },
    {
      "device_id": "B2C3D4E5",
      "mac": "A8:8F:AD:C4:0A:25",
      "created_at": "2025-02-08T14:31:00Z",
      "updated_at": "2025-02-08T14:31:00Z",
      "config": { /* ... */ }
    }
  ],
  "count": 2
}
```

---

### 3. Créer un Device

Enregistre un nouveau device avec sa configuration initiale.

```http
POST /config?device_id=ADC40A24
Content-Type: application/json

{
  "mac": "A8:8F:AD:C4:0A:24",
  "config": {
    "mqtt_broker": "192.168.1.15",
    "mqtt_port": 1883,
    "mqtt_topic": "home/devices/config_client_A1B2C3/data",
    "poll_frequency_sec": 60,
    "heartbeat_frequency_sec": 30,
    "template": "sensor-v2"
  }
}
```

**Query Parameters**:

- `device_id` (required): ID du device (hexadécimal, 8-64 chars)

**Body**:

- `mac` (required): Adresse MAC (format: XX:XX:XX:XX:XX:XX)
- `config` (required): Configuration du device
  - `mqtt_broker` (string): Adresse du broker MQTT
  - `mqtt_port` (integer): Port du broker MQTT
  - `mqtt_topic` (string): Topic MQTT pour ce device
  - `poll_frequency_sec` (integer): Fréquence de polling en secondes
  - `heartbeat_frequency_sec` (integer): Fréquence du heartbeat en secondes
  - `template` (optional): Template prédéfini (sensor-v1, sensor-v2)

**Response (201 Created)**:

```json
{
  "device_id": "ADC40A24",
  "mac": "A8:8F:AD:C4:0A:24",
  "created_at": "2025-02-08T14:30:00Z",
  "updated_at": "2025-02-08T14:30:00Z",
  "config": { /* ... */ }
}
```

**Erreurs**:

- `400 Bad Request`:
  - Device déjà existe
  - Validation échouée (mac, device_id ou config invalides)
  - JSON invalide

---

### 4. Mettre à Jour Configuration

Modifie la configuration d'un device existant.

```text
PUT /config?device_id=ADC40A24
Content-Type: application/json

{
  "config": {
    "mqtt_broker": "192.168.1.20",
    "mqtt_port": 1883,
    "mqtt_topic": "home/devices/config_client_A1B2C3/data",
    "poll_frequency_sec": 30,
    "heartbeat_frequency_sec": 15,
    "template": "sensor-v2"
  }
}
```

**Query Parameters**:

- `device_id` (required): ID du device

**Body**:

- `config` (required): Nouvelle configuration (même structure que POST)

**Response (200 OK)**:

```json
{
  "device_id": "ADC40A24",
  "mac": "A8:8F:AD:C4:0A:24",
  "created_at": "2025-02-08T14:30:00Z",
  "updated_at": "2025-02-08T14:35:00Z",
  "config": { /* ... */ }
}
```

**Erreurs**:

- `404 Not Found`: Device n'existe pas
- `400 Bad Request`: Validation échouée ou JSON invalide

---

## Exemples avec curl

### Health check

```bash
curl http://localhost:5050/health
```

### Créer un device

```bash
curl -X POST "http://localhost:5050/config?device_id=ADC40A24" \
  -H "Content-Type: application/json" \
  -d '{
    "mac": "A8:8F:AD:C4:0A:24",
    "config": {
      "mqtt_broker": "192.168.1.15",
      "mqtt_port": 1883,
      "mqtt_topic": "home/devices/config_client_A1B2C3/data",
      "poll_frequency_sec": 60,
      "heartbeat_frequency_sec": 30,
      "template": "sensor-v2"
    }
  }'
```

### Récupérer config d'un device

```bash
curl "http://localhost:5050/config?device_id=ADC40A24&mac=A8:8F:AD:C4:0A:24"
```

### Lister tous les devices

```bash
curl http://localhost:5000/config
```

### Mettre à jour config

```bash
curl -X PUT "http://localhost:5000/config?device_id=ADC40A24" \
  -H "Content-Type: application/json" \
  -d '{
    "config": {
      "mqtt_broker": "192.168.1.15",
      "mqtt_port": 1883,
      "mqtt_topic": "home/devices/config_client_A1B2C3/data",
      "poll_frequency_sec": 30,
      "heartbeat_frequency_sec": 15,
      "template": "sensor-v2"
    }
  }'
```

---

## Codes HTTP

| Code | Signification | Cas d'usage |
| --- | --- | --- |
| 200 | OK | GET/PUT réussi |
| 201 | Created | POST réussi |
| 400 | Bad Request | Validation échouée, JSON invalide |
| 404 | Not Found | Device n'existe pas |
| 500 | Server Error | Erreur serveur interne |

---

## Validation

### device_id

- Format: Hexadécimal (0-9, A-F)
- Longueur: 8-64 caractères
- Exemple: `ADC40A24`, `A1B2C3D4E5F6`

### mac

- Format: `XX:XX:XX:XX:XX:XX` (hexadécimal)
- Exemple: `A8:8F:AD:C4:0A:24`

### config

- Tous les champs requis: `mqtt_broker`, `mqtt_port`, `mqtt_topic`, `poll_frequency_sec`, `heartbeat_frequency_sec`
- `mqtt_port` et fréquences: entiers positifs

---

## Découverte mDNS

Le serveur annonce automatiquement le service mDNS:

```text
Service: _config._tcp.local
Hostname: config-server
Port: 5000
Properties:
  - path: /config
  - version: 1.0
```

**Interroger le service mDNS** (macOS):

```bash
dns-sd -B _config._tcp local
```

**Interroger le service mDNS** (Linux):

```bash
avahi-browse -rt _config._tcp
```

---

## Notes

- Toutes les requêtes/réponses utilisent JSON
- Les timestamps sont au format ISO 8601 avec timezone Z (UTC)
- Les IDs persistants sont sauvegardés en JSON dans `data/devices.json`
- Pas d'authentification requise (API locale)
