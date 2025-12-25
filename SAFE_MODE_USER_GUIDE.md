# SynDimm Safe Mode - Complete User Guide

## Table of Contents
1. [Introduction](#introduction)
2. [How Safe Mode Works](#how-safe-mode-works)
3. [Password Format](#password-format)
4. [API Configuration Fields](#api-configuration-fields)
5. [HTTP Methods Explained](#http-methods-explained)
6. [Step-by-Step Examples](#step-by-step-examples)
7. [Advanced Configuration](#advanced-configuration)
8. [Troubleshooting](#troubleshooting)

---

## Introduction

Safe Mode transforms your SynDimm device into a password-protected API trigger. By entering specific encoder movement sequences (passwords), you can trigger HTTP API calls to control smart home devices, unlock doors, activate automations, and more.

### Key Features
- **5 Independent Passwords**: Each with its own API configuration
- **Unlimited URL/Body Size**: No character limits (stored on flash)
- **4 HTTP Methods**: GET, POST, PUT, DELETE
- **Custom Headers**: Multiple headers supported
- **Authorization Support**: Bearer tokens, Basic auth, API keys
- **Lazy Loading**: API config loaded only when needed (RAM efficient)

---

## How Safe Mode Works

```
┌─────────────────────────────────────────────────────────────────────┐
│                        SAFE MODE FLOW                                │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  ┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐      │
│  │ Encoder  │───▶│ SafeLock │───▶│ Password │───▶│  Load    │      │
│  │ Movement │    │ Buffer   │    │  Match   │    │ API JSON │      │
│  └──────────┘    └──────────┘    └──────────┘    └────┬─────┘      │
│                                                        │            │
│                                                        ▼            │
│  ┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────┐      │
│  │  Result  │◀───│   HTTP   │◀───│  Build   │◀───│   API    │      │
│  │ Success  │    │ Request  │    │ Request  │    │  Config  │      │
│  └──────────┘    └──────────┘    └──────────┘    └──────────┘      │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

### Step-by-Step Process:
1. **User rotates encoder** - Left (L) or Right (R) movements are recorded
2. **User presses button (optional)** - Button press (B) can end password
3. **Buffer comparison** - System compares movements against stored passwords
4. **Password match found** - If sequence matches, API config is loaded
5. **HTTP request sent** - Request is built and sent to configured URL
6. **Response received** - Success (2xx) or failure is logged

---

## Password Format

### Syntax
```
[Direction][Count]-[Direction][Count]-...-[Direction][Count][-B]
```

### Components
| Symbol | Meaning | Example |
|--------|---------|---------|
| `L` | Left rotation | `L5` = 5 clicks left |
| `R` | Right rotation | `R3` = 3 clicks right |
| `B` | Button press | Ends password sequence |
| `-` | Step separator | Separates movements |
| Number | Click count | 1-50 clicks per step |

### Rules
- **Minimum steps**: 3
- **Maximum steps**: 6
- **Clicks per step**: 1-50
- **Button (B)**: Optional, always at the end

### Password Examples

| Password | Meaning |
|----------|---------|
| `L3-R5-L2` | Left 3, Right 5, Left 2 (auto-detect) |
| `R10-L5-R3-B` | Right 10, Left 5, Right 3, Button press |
| `L1-R1-L1-R1-L1-R1` | Alternating pattern (6 steps max) |
| `R50-L50-R50-B` | Large movements with button |

---

## API Configuration Fields

### Web Interface Form

```
┌─────────────────────────────────────────────────────────────────┐
│ ☑ Enable Password                                      ✓ API   │
├─────────────────────────────────────────────────────────────────┤
│ Password: [L3-R5-L2-R4-B____________________________________]   │
│                                                                 │
│ ▼ API Settings                                                  │
│ ┌─────────────────────────────────────────────────────────────┐ │
│ │                                                             │ │
│ │ URL:                                                        │ │
│ │ [http://192.168.1.100/api/action______________________]     │ │
│ │                                                             │ │
│ │ Method:              Content-Type:                          │ │
│ │ [GET      ▼]         [application/json________________]     │ │
│ │                                                             │ │
│ │ Authorization:                                              │ │
│ │ [Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..._____]       │ │
│ │                                                             │ │
│ │ Custom Headers:                                             │ │
│ │ [X-API-Key: your-api-key-here_________________________]     │ │
│ │ [X-Device-ID: syndimm-001____________________________]      │ │
│ │                                                             │ │
│ │ Body (JSON):                                                │ │
│ │ [{"action": "unlock", "duration": 5}__________________]     │ │
│ │                                                             │ │
│ └─────────────────────────────────────────────────────────────┘ │
│                                                                 │
│ [Save]                                              [Test API]  │
└─────────────────────────────────────────────────────────────────┘
```

### Field Descriptions

| Field | Required | Default | Description |
|-------|----------|---------|-------------|
| **URL** | ✅ Yes | - | Full API endpoint URL (http:// or https://) |
| **Method** | No | GET | HTTP method: GET, POST, PUT, DELETE |
| **Content-Type** | No | application/json | MIME type for request body |
| **Authorization** | No | - | Auth header value (Bearer token, Basic, etc.) |
| **Custom Headers** | No | - | Additional headers (one per line) |
| **Body** | No | {} | Request body for POST/PUT methods |

---

## HTTP Methods Explained

### GET
**Purpose**: Retrieve data or trigger simple actions without sending data.

**Characteristics**:
- No request body
- Parameters in URL query string
- Idempotent (same request = same result)
- Cacheable

**When to use**:
- Simple device toggles (Shelly, Tasmota)
- Webhook triggers
- Status checks
- IFTTT/Zapier integrations

**Example request**:
```http
GET /relay/0?turn=toggle HTTP/1.1
Host: 192.168.1.50
Authorization: Bearer abc123
```

---

### POST
**Purpose**: Create new resources or trigger actions with data payload.

**Characteristics**:
- Request body required
- Content-Type header important
- Not idempotent (may have different effects)
- Not cached

**When to use**:
- Home Assistant service calls
- Creating new resources
- Complex automation triggers
- Sending JSON/XML data

**Example request**:
```http
POST /api/services/switch/toggle HTTP/1.1
Host: homeassistant.local:8123
Content-Type: application/json
Authorization: Bearer eyJ0eXAiOiJKV1Q...

{"entity_id": "switch.garage_door"}
```

---

### PUT
**Purpose**: Update existing resources or set specific states.

**Characteristics**:
- Request body required
- Idempotent (same request = same result)
- Replaces entire resource
- Used for updates

**When to use**:
- Setting device to specific state
- Updating configuration
- RESTful API updates
- State synchronization

**Example request**:
```http
PUT /api/devices/lock/status HTTP/1.1
Host: api.smartlock.com
Content-Type: application/json
Authorization: Bearer sk_live_xxx
X-Request-ID: syndimm-12345

{"status": "unlocked", "duration": 30}
```

---

### DELETE
**Purpose**: Remove resources or cancel actions.

**Characteristics**:
- Usually no request body
- Idempotent
- Permanent action
- Less commonly used in IoT

**When to use**:
- Canceling scheduled tasks
- Removing temporary access
- Clearing alarms
- Revoking permissions

**Example request**:
```http
DELETE /api/access/temporary/abc123 HTTP/1.1
Host: api.accesscontrol.com
Authorization: Bearer admin_token
```

---

## Step-by-Step Examples

### Example 1: Shelly Relay Toggle (Simple GET)

**Scenario**: Toggle a Shelly relay when correct password is entered.

**Device**: Shelly 1 at IP 192.168.1.50

#### Step 1: Define Password
```
Password: L3-R5-L3-B
Meaning: Left 3 clicks, Right 5 clicks, Left 3 clicks, Button press
```

#### Step 2: Configure API
| Field | Value |
|-------|-------|
| URL | `http://192.168.1.50/relay/0?turn=toggle` |
| Method | `GET` |
| Content-Type | *(leave empty)* |
| Authorization | *(leave empty)* |
| Custom Headers | *(leave empty)* |
| Body | *(leave empty)* |

#### Step 3: How it works
```
User action:  ←←← →→→→→ ←←← [BUTTON]
Encoder:      L3  R5     L3  B
Password:     L3-R5-L3-B ✓ MATCH!

HTTP Request:
GET /relay/0?turn=toggle HTTP/1.1
Host: 192.168.1.50

Response: 200 OK
Relay toggles on/off
```

---

### Example 2: Home Assistant Switch Control (POST with Bearer Token)

**Scenario**: Toggle a Home Assistant switch entity with authentication.

**Device**: Home Assistant at homeassistant.local:8123

#### Step 1: Define Password
```
Password: R10-L5-R10-B
Meaning: Right 10, Left 5, Right 10, Button
```

#### Step 2: Get Long-Lived Access Token
1. Open Home Assistant
2. Go to Profile → Long-Lived Access Tokens
3. Create new token, copy it

#### Step 3: Configure API
| Field | Value |
|-------|-------|
| URL | `http://homeassistant.local:8123/api/services/switch/toggle` |
| Method | `POST` |
| Content-Type | `application/json` |
| Authorization | `Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiI...` |
| Custom Headers | *(leave empty)* |
| Body | `{"entity_id": "switch.garage_door"}` |

#### Step 4: How it works
```
User action:  →→→→→→→→→→ ←←←←← →→→→→→→→→→ [BUTTON]
Encoder:      R10        L5     R10        B
Password:     R10-L5-R10-B ✓ MATCH!

HTTP Request:
POST /api/services/switch/toggle HTTP/1.1
Host: homeassistant.local:8123
Content-Type: application/json
Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...

{"entity_id": "switch.garage_door"}

Response: 200 OK
Switch toggles
```

---

### Example 3: Tasmota Power Control (POST with Form Data)

**Scenario**: Send command to Tasmota device using form-urlencoded data.

**Device**: Tasmota device at 192.168.1.60

#### Step 1: Define Password
```
Password: L5-R5-L5
Meaning: Left 5, Right 5, Left 5 (no button - auto-detect)
```

#### Step 2: Configure API
| Field | Value |
|-------|-------|
| URL | `http://192.168.1.60/cm` |
| Method | `POST` |
| Content-Type | `application/x-www-form-urlencoded` |
| Authorization | *(leave empty)* |
| Custom Headers | *(leave empty)* |
| Body | `cmnd=Power%20Toggle` |

#### Step 3: How it works
```
User action:  ←←←←← →→→→→ ←←←←←
Encoder:      L5    R5    L5
Password:     L5-R5-L5 ✓ MATCH!

HTTP Request:
POST /cm HTTP/1.1
Host: 192.168.1.60
Content-Type: application/x-www-form-urlencoded

cmnd=Power%20Toggle

Response: 200 OK {"POWER":"ON"} or {"POWER":"OFF"}
```

---

### Example 4: Smart Lock API (PUT with Multiple Headers)

**Scenario**: Unlock a smart lock using RESTful API with custom headers.

**Device**: Smart lock API at api.smartlock.com

#### Step 1: Define Password
```
Password: R3-L3-R3-L3-R3-B
Meaning: Alternating R3-L3 pattern, ends with button (high security)
```

#### Step 2: Configure API
| Field | Value |
|-------|-------|
| URL | `https://api.smartlock.com/v2/devices/front-door/unlock` |
| Method | `PUT` |
| Content-Type | `application/json` |
| Authorization | `Bearer sk_live_xxxxxxxxxxxxx` |
| Custom Headers | `X-Device-ID: syndimm-001`<br>`X-Request-Source: encoder`<br>`X-Timestamp: auto` |
| Body | `{"command": "unlock", "duration": 10, "notify": true}` |

#### Step 3: How it works
```
User action:  →→→ ←←← →→→ ←←← →→→ [BUTTON]
Encoder:      R3  L3  R3  L3  R3  B
Password:     R3-L3-R3-L3-R3-B ✓ MATCH!

HTTP Request:
PUT /v2/devices/front-door/unlock HTTP/1.1
Host: api.smartlock.com
Content-Type: application/json
Authorization: Bearer sk_live_xxxxxxxxxxxxx
X-Device-ID: syndimm-001
X-Request-Source: encoder
X-Timestamp: auto

{"command": "unlock", "duration": 10, "notify": true}

Response: 200 OK
Lock unlocks for 10 seconds
Notification sent to phone
```

---

### Example 5: IFTTT Webhook (Simple GET with Key)

**Scenario**: Trigger IFTTT applet via webhook.

**Service**: IFTTT Webhooks

#### Step 1: Get IFTTT Webhook URL
1. Go to ifttt.com/maker_webhooks
2. Click "Documentation"
3. Note your key: `aBcDeFgHiJkLmNoPqRs`

#### Step 2: Define Password
```
Password: L1-R1-L1-R1
Meaning: Quick alternating pattern (emergency trigger)
```

#### Step 3: Configure API
| Field | Value |
|-------|-------|
| URL | `https://maker.ifttt.com/trigger/door_opened/with/key/aBcDeFgHiJkLmNoPqRs` |
| Method | `GET` |
| Content-Type | *(leave empty)* |
| Authorization | *(leave empty)* |
| Custom Headers | *(leave empty)* |
| Body | *(leave empty)* |

#### Step 4: How it works
```
User action:  ← → ← →
Encoder:      L1 R1 L1 R1
Password:     L1-R1-L1-R1 ✓ MATCH!

HTTP Request:
GET /trigger/door_opened/with/key/aBcDeFgHiJkLmNoPqRs HTTP/1.1
Host: maker.ifttt.com

Response: 200 OK
IFTTT applet triggered
```

---

### Example 6: Node-RED Endpoint (POST with JSON)

**Scenario**: Send data to Node-RED HTTP input node.

**Service**: Node-RED at 192.168.1.10:1880

#### Step 1: Create Node-RED Flow
```
[HTTP In] → [Function] → [HTTP Response]
   │
   └── Method: POST, URL: /syndimm/trigger
```

#### Step 2: Define Password
```
Password: R20-L10-R5-B
Meaning: Large right, medium left, small right, button
```

#### Step 3: Configure API
| Field | Value |
|-------|-------|
| URL | `http://192.168.1.10:1880/syndimm/trigger` |
| Method | `POST` |
| Content-Type | `application/json` |
| Authorization | *(leave empty)* |
| Custom Headers | `X-Source: syndimm-encoder` |
| Body | `{"event": "password_matched", "password_id": 1, "timestamp": "auto"}` |

#### Step 4: How it works
```
User action:  →→→→→→→→→→→→→→→→→→→→ ←←←←←←←←←← →→→→→ [BUTTON]
Encoder:      R20                   L10        R5    B
Password:     R20-L10-R5-B ✓ MATCH!

HTTP Request:
POST /syndimm/trigger HTTP/1.1
Host: 192.168.1.10:1880
Content-Type: application/json
X-Source: syndimm-encoder

{"event": "password_matched", "password_id": 1, "timestamp": "auto"}

Response: 200 OK
Node-RED flow executes
```

---

### Example 7: Philips Hue Light Control (PUT)

**Scenario**: Turn on/off Hue light via local API.

**Device**: Hue Bridge at 192.168.1.2

#### Step 1: Get Hue API Key
1. Press button on Hue Bridge
2. Send POST to `http://<bridge>/api` with body `{"devicetype":"syndimm"}`
3. Get username from response

#### Step 2: Define Password
```
Password: L2-R2-L2-R2-B
Meaning: Small alternating pattern with button
```

#### Step 3: Configure API
| Field | Value |
|-------|-------|
| URL | `http://192.168.1.2/api/YOUR-HUE-USERNAME/lights/1/state` |
| Method | `PUT` |
| Content-Type | `application/json` |
| Authorization | *(leave empty - key in URL)* |
| Custom Headers | *(leave empty)* |
| Body | `{"on": true, "bri": 254}` |

#### Step 4: How it works
```
User action:  ←← →→ ←← →→ [BUTTON]
Encoder:      L2 R2 L2 R2 B
Password:     L2-R2-L2-R2-B ✓ MATCH!

HTTP Request:
PUT /api/YOUR-HUE-USERNAME/lights/1/state HTTP/1.1
Host: 192.168.1.2
Content-Type: application/json

{"on": true, "bri": 254}

Response: 200 OK
Light turns on at full brightness
```

---

### Example 8: Delete Temporary Access (DELETE)

**Scenario**: Revoke temporary door access code.

**Service**: Access control API

#### Step 1: Define Password
```
Password: L10-R10-L10-B
Meaning: Strong pattern for destructive action
```

#### Step 2: Configure API
| Field | Value |
|-------|-------|
| URL | `https://api.accesscontrol.com/v1/access/temporary/visitor-code-123` |
| Method | `DELETE` |
| Content-Type | *(leave empty)* |
| Authorization | `Bearer admin_access_token_here` |
| Custom Headers | `X-Confirm: true` |
| Body | *(leave empty)* |

#### Step 3: How it works
```
User action:  ←←←←←←←←←← →→→→→→→→→→ ←←←←←←←←←← [BUTTON]
Encoder:      L10        R10        L10        B
Password:     L10-R10-L10-B ✓ MATCH!

HTTP Request:
DELETE /v1/access/temporary/visitor-code-123 HTTP/1.1
Host: api.accesscontrol.com
Authorization: Bearer admin_access_token_here
X-Confirm: true

Response: 204 No Content
Temporary access code deleted
```

---

### Example 9: XML SOAP Request (POST with XML)

**Scenario**: Call legacy SOAP web service.

**Service**: Enterprise SOAP service

#### Step 1: Define Password
```
Password: R5-L5-R5-L5-B
Meaning: Balanced alternating pattern
```

#### Step 2: Configure API
| Field | Value |
|-------|-------|
| URL | `https://enterprise.server.com/soap/DoorService` |
| Method | `POST` |
| Content-Type | `application/xml` |
| Authorization | `Basic dXNlcm5hbWU6cGFzc3dvcmQ=` |
| Custom Headers | `SOAPAction: "urn:DoorService:Unlock"` |
| Body | `<?xml version="1.0"?><soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/"><soap:Body><Unlock><DoorId>FRONT-001</DoorId></Unlock></soap:Body></soap:Envelope>` |

#### Step 3: How it works
```
HTTP Request:
POST /soap/DoorService HTTP/1.1
Host: enterprise.server.com
Content-Type: application/xml
Authorization: Basic dXNlcm5hbWU6cGFzc3dvcmQ=
SOAPAction: "urn:DoorService:Unlock"

<?xml version="1.0"?>
<soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/">
  <soap:Body>
    <Unlock>
      <DoorId>FRONT-001</DoorId>
    </Unlock>
  </soap:Body>
</soap:Envelope>

Response: 200 OK with SOAP response
```

---

### Example 10: Zapier Webhook with JSON Data (POST)

**Scenario**: Trigger Zapier automation with custom data.

**Service**: Zapier Webhooks

#### Step 1: Create Zapier Webhook
1. Create new Zap
2. Choose "Webhooks by Zapier" as trigger
3. Select "Catch Hook"
4. Copy webhook URL

#### Step 2: Define Password
```
Password: L3-R6-L9-B
Meaning: Increasing pattern (3, 6, 9)
```

#### Step 3: Configure API
| Field | Value |
|-------|-------|
| URL | `https://hooks.zapier.com/hooks/catch/123456/abcdef/` |
| Method | `POST` |
| Content-Type | `application/json` |
| Authorization | *(leave empty)* |
| Custom Headers | *(leave empty)* |
| Body | `{"trigger": "syndimm", "action": "unlock", "location": "front_door", "timestamp": "2025-12-25T10:30:00Z"}` |

---

## Advanced Configuration

### Custom Headers Format

Enter one header per line in the format `Header-Name: Header-Value`:

```
X-API-Key: your-secret-api-key
X-Device-ID: syndimm-encoder-01
X-Request-Source: physical-encoder
X-Correlation-ID: unique-request-id
Cache-Control: no-cache
```

### Authorization Types

| Type | Format | Example |
|------|--------|---------|
| Bearer Token | `Bearer <token>` | `Bearer eyJhbGciOiJIUzI1NiIs...` |
| Basic Auth | `Basic <base64>` | `Basic dXNlcm5hbWU6cGFzc3dvcmQ=` |
| API Key | `ApiKey <key>` | `ApiKey sk_live_xxxxxxxxxxxxx` |
| Custom | Any string | `Token abc123xyz` |

### Content-Type Options

| MIME Type | Use Case |
|-----------|----------|
| `application/json` | Most REST APIs, Home Assistant |
| `application/x-www-form-urlencoded` | HTML forms, Tasmota |
| `application/xml` | SOAP services, legacy APIs |
| `text/plain` | Simple text endpoints |
| `text/xml` | XML without encoding |
| `multipart/form-data` | File uploads (not typical for Safe mode) |

---

## Troubleshooting

### Common Issues

| Problem | Possible Cause | Solution |
|---------|---------------|----------|
| API test fails | Wrong URL | Check IP address and port |
| 401 Unauthorized | Missing/wrong auth | Verify Authorization header |
| 404 Not Found | Wrong endpoint path | Check API documentation |
| Timeout | Network issue | Verify WiFi connection |
| Password not matching | Wrong sequence | Check password format |
| No response | AP Mode active | Connect to WiFi network |

### Debug Tips

1. **Check WiFi**: Safe mode API calls only work when connected to WiFi (not in AP mode)
2. **Test locally first**: Use browser or curl to test URL before configuring
3. **Check Serial output**: Enable debug to see detailed logs
4. **Verify password**: Count your encoder clicks carefully
5. **LittleFS check**: Ensure LittleFS is initialized properly

### Serial Debug Output
```
[SafeLock] BUTON
[SafeLock] >>> SIFRE #0 ESLESTI! <<<
[SafeAPI] Password #0 matched, triggering API...
[SafeAPI] API enabled: 1
[SafeAPI] Cihaz IP: 192.168.1.100
[SafeAPI] API istegi gonderiliyor...
[SafeAPI] Sending request...
[SafeAPI] Header: X-API-Key
[SafeAPI] HTTP 200 - OK
[SafeAPI] Sonuc: 0
```

---

## Storage Architecture

```
ESP32-C6 Flash Memory
│
├── NVS Partition (~16KB)
│   └── safelock namespace
│       ├── pwd0: "L3-R5-L2-B"     (password pattern)
│       ├── active0: true          (enabled flag)
│       ├── hasApi0: true          (has API config)
│       ├── pwd1: "R10-L5-R3"
│       ├── active1: false
│       ├── hasApi1: false
│       └── ... (5 passwords total)
│
└── LittleFS Partition (1-4MB)
    └── /safe/
        ├── api_0.json    (~1-10KB per file)
        ├── api_1.json
        ├── api_2.json
        ├── api_3.json
        └── api_4.json
```

### API Config JSON Structure
```json
{
  "enabled": true,
  "method": 1,
  "url": "https://api.example.com/action",
  "contentType": "application/json",
  "authorization": "Bearer eyJ0eXAi...",
  "customHeaders": "X-Key: value\nX-Device: syndimm",
  "body": "{\"action\": \"trigger\"}"
}
```

### Method Values
| Value | Method |
|-------|--------|
| 0 | GET |
| 1 | POST |
| 2 | PUT |
| 3 | DELETE |

---

## Quick Reference Card

### Password Syntax
```
L = Left rotation     R = Right rotation     B = Button press
Number = Click count (1-50)                  - = Separator
Min 3 steps, Max 6 steps

Examples:
L3-R5-L2      (auto-detect, no button)
R10-L5-R3-B   (ends with button press)
```

### API Quick Setup
```
1. Enter password pattern
2. Click "API Settings" to expand
3. Enter URL (required)
4. Select Method (GET/POST/PUT/DELETE)
5. Add Content-Type if needed
6. Add Authorization if needed
7. Add Body for POST/PUT
8. Click Save
9. Click Test API to verify
```

---

**Version**: 1.3.0  
**Last Updated**: December 2025  
**Compatible With**: SynDimm ESP32-C6 Firmware

