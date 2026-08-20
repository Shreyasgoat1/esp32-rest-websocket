# ESP32 Embedded REST API Web Server with WebSocket

ESP32 IoT gateway using Arduino/PlatformIO, C++, ESPAsyncWebServer, ArduinoJson and FreeRTOS.

## Features

- REST API
  - `GET /sensors`
  - `POST /config`
  - `PUT /relay`
  - `GET /heap`
- JSON request/response handling with ArduinoJson
- HTTP status/error handling
- Async WebSocket endpoint `/ws`
- Multiple WebSocket clients
- Browser dashboard
- FreeRTOS sensor, WebSocket and maintenance tasks
- Mutex-protected shared state
- Heap and stack monitoring
- Simulated temperature/humidity values
- Relay GPIO control

## Project structure

```text
esp32-rest-websocket/
├── platformio.ini
├── src/
│   └── main.cpp
├── python/
│   └── test_client.py
├── certs/
│   └── README.txt
└── README.md
```

## Installation

1. Install VS Code and PlatformIO.
2. Open this project folder.
3. Change `WIFI_SSID` and `WIFI_PASSWORD` in `src/main.cpp`.
4. Connect the ESP32.
5. Build and upload.
6. Open the serial monitor at 115200 baud.

Required libraries are declared in `platformio.ini`.

## Running

After upload, the serial monitor prints the ESP32 IP address.

Open:

```text
http://ESP32_IP/
```

Example:

```text
http://192.168.1.105/
```

The browser dashboard connects to `/ws` and receives live updates.

## REST API

### GET /sensors

```bash
curl http://192.168.1.105/sensors
```

### POST /config

```bash
curl -X POST http://192.168.1.105/config \
-H "Content-Type: application/json" \
-d "{\"sampling_ms\":3000,\"temperature_threshold\":32,\"humidity_threshold\":75}"
```

### PUT /relay

```bash
curl -X PUT http://192.168.1.105/relay \
-H "Content-Type: application/json" \
-d "{\"state\":true}"
```

Turn off:

```bash
curl -X PUT http://192.168.1.105/relay \
-H "Content-Type: application/json" \
-d "{\"state\":false}"
```

### GET /heap

```bash
curl http://192.168.1.105/heap
```

## Python client

Install:

```bash
pip install requests websocket-client
```

Change `ESP32_IP` in `python/test_client.py`, then run:

```bash
python python/test_client.py
```

## WebSocket

Endpoint:

```text
ws://ESP32_IP/ws
```

The ESP32 broadcasts sensor JSON approximately every 2 seconds.

## Wiring

### Relay module

```text
ESP32 GPIO 26 -> Relay IN
ESP32 3.3V    -> Relay VCC (only if supported by the module)
ESP32 GND     -> Relay GND
```

Check your relay module's voltage/current requirements before connecting it.

Do not connect mains voltage directly to an ESP32 GPIO. Use an appropriately rated isolated relay module and proper electrical safety practices.

### Sensor

The current project uses simulated sensor values so the software can be tested without hardware. A DHT11/DHT22 can be added later.

## FreeRTOS architecture

### SensorTask

- Core 1
- Priority 2
- Stack 4096
- Updates temperature/humidity

### WebSocketTask

- Core 1
- Priority 1
- Stack 4096
- Broadcasts live JSON

### ServerTask

- Core 0
- Priority 1
- Stack 4096
- Performs lightweight maintenance and memory monitoring

A mutex protects shared sensor/configuration data.

Stack sizes should be tuned using `uxTaskGetStackHighWaterMark()` after stress testing rather than assuming 4096 bytes is optimal.

## Memory management

The firmware reports:

- `ESP.getFreeHeap()`
- `ESP.getMinFreeHeap()`
- `heap_caps_get_largest_free_block()`
- `uxTaskGetStackHighWaterMark()`

Avoid unnecessarily large local arrays and large temporary JSON documents.

## HTTPS

This Arduino/ESPAsyncWebServer version is intentionally HTTP-only.

For a production HTTPS implementation, use ESP-IDF's `esp_https_server` rather than simply changing port 80 to 443.

Generate a development self-signed certificate with OpenSSL:

```bash
openssl genrsa -out server.key 2048

openssl req -new -x509 \
-key server.key \
-out server.crt \
-days 365 \
-subj "/CN=esp32.local"
```

A self-signed certificate is not automatically trusted by normal browsers.

HTTPS consists conceptually of:

```text
TCP connection
      ↓
TLS handshake
      ↓
certificate verification / key exchange
      ↓
encrypted TLS session
      ↓
HTTP
```

Keep the private key (`server.key`) secret.

## Development order

1. REST API
2. WebSocket
3. FreeRTOS tasks and synchronization
4. Memory/stack testing
5. HTTPS using ESP-IDF
6. Python automated testing

## Current sensor behavior

Temperature and humidity are simulated in software. Replace the simulation in `sensorTask()` with a real sensor driver when hardware is available.

## License

Use and modify for educational/project purposes.
