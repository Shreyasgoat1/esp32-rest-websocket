# ESP32 Embedded REST API Web Server with WebSocket

An ESP32-based IoT web server developed using **C++, Arduino/PlatformIO, FreeRTOS, ESPAsyncWebServer, ArduinoJson, and WebSocket**. The system provides REST APIs for sensor monitoring, device configuration, and relay control, while WebSocket enables real-time sensor updates to connected clients.

## Features

* REST API server using HTTP
* `GET /sensors` — returns live sensor readings as JSON
* `POST /config` — updates device configuration
* `PUT /relay` — controls relay ON/OFF
* `GET /heap` — monitors ESP32 memory
* JSON parsing and serialization using ArduinoJson
* WebSocket server at `/ws`
* Real-time sensor data updates without polling
* Multiple WebSocket client support
* Browser-based monitoring dashboard
* FreeRTOS task-based architecture
* Mutex for safe sharing of sensor/configuration data
* Heap and task stack monitoring
* Python client for REST API and WebSocket testing
* Simulated temperature and humidity sensor values

## Project Structure

```text
ESP32-REST-WebSocket/
│
├── src/
│   └── main.cpp
│
├── python/
│   └── test_client.py
│
├── certs/
│   └── README.txt
│
├── platformio.ini
├── README.md
└── .gitignore
```

### File Description

| File               | Description                              |
| ------------------ | ---------------------------------------- |
| `main.cpp`         | Main ESP32 firmware                      |
| `test_client.py`   | Python REST and WebSocket testing client |
| `platformio.ini`   | PlatformIO configuration and libraries   |
| `certs/README.txt` | HTTPS certificate instructions           |
| `README.md`        | Project documentation                    |
| `.gitignore`       | Ignores build and private files          |

## Installation

### 1. Requirements

* ESP32 development board
* USB cable
* VS Code
* PlatformIO
* Wi-Fi connection
* Python 3.x

### 2. Clone the Repository

```bash
git clone YOUR_GITHUB_REPOSITORY_URL
cd ESP32-REST-WebSocket
```

### 3. Configure Wi-Fi

Open:

```text
src/main.cpp
```

Change:

```cpp
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
```

Enter your Wi-Fi credentials.

### 4. Build the Project

```bash
pio run
```

### 5. Upload to ESP32

Connect the ESP32 to your computer and run:

```bash
pio run --target upload
```

### 6. Open Serial Monitor

```bash
pio device monitor
```

Use:

```text
115200 baud
```

After connecting to Wi-Fi, the ESP32 displays its IP address.

Example:

```text
Wi-Fi connected
IP address: 192.168.1.105
HTTP server started
FreeRTOS tasks started
```

## Running Algorithm

The overall working process is:

```text
Start
  │
  ▼
Initialize ESP32
  │
  ▼
Connect to Wi-Fi
  │
  ▼
Initialize GPIO and Mutex
  │
  ▼
Start REST API Server
  │
  ▼
Start WebSocket Server
  │
  ▼
Create FreeRTOS Tasks
  │
  ├───────────────┐
  │               │
  ▼               ▼
Sensor Task   WebSocket Task
  │               │
  ▼               ▼
Read/Generate   Get Sensor
Sensor Data     Data
  │               │
  └───────┬───────┘
          ▼
     Shared Data
       + Mutex
          │
          ▼
   Send JSON through
      WebSocket
          │
          ▼
    Browser/Python
       Client
```

### REST API Algorithm

```text
Client sends HTTP request
          │
          ▼
ESP32 receives request
          │
          ▼
Check HTTP method and endpoint
          │
          ▼
Parse JSON if required
          │
          ▼
Validate input
          │
          ├── Invalid → 400 Error
          │
          ▼
Update/read device data
          │
          ▼
Create JSON response
          │
          ▼
Send HTTP response
```

### WebSocket Algorithm

```text
Browser/Python Client
          │
          ▼
Connect to /ws
          │
          ▼
ESP32 accepts connection
          │
          ▼
Sensor Task updates data
          │
          ▼
WebSocket Task creates JSON
          │
          ▼
Broadcast data to clients
          │
          ▼
Clients display live data
```

## Technologies Used

* **ESP32** — Main microcontroller
* **C++** — Firmware programming
* **Arduino Framework** — Embedded development framework
* **PlatformIO** — Build and dependency management
* **FreeRTOS** — Multitasking and task management
* **ESPAsyncWebServer** — Asynchronous HTTP/WebSocket server
* **ArduinoJson** — JSON parsing and serialization
* **Wi-Fi** — Network communication
* **HTTP REST API** — Device control and data access
* **WebSocket** — Real-time communication
* **Python** — API testing
* **Requests** — HTTP client library
* **websocket-client** — Python WebSocket client
* **OpenSSL** — Self-signed certificate generation

## Current Progress

* [x] ESP32 Wi-Fi connection
* [x] REST API server
* [x] `GET /sensors`
* [x] `POST /config`
* [x] `PUT /relay`
* [x] `GET /heap`
* [x] JSON communication
* [x] WebSocket server
* [x] Multiple WebSocket clients
* [x] Browser dashboard
* [x] FreeRTOS task separation
* [x] Mutex-based synchronization
* [x] Heap monitoring
* [x] Stack monitoring
* [x] Python REST API client
* [x] Python WebSocket client
* [x] Error handling
* [x] Simulated sensor readings
* [ ] Real sensor integration
* [ ] HTTPS implementation
* [ ] Authentication
* [ ] OTA firmware update

## Feature Improvements

### 1. Real Sensor Integration

Replace simulated temperature and humidity values with a real **DHT11, DHT22, or BME280** sensor.

### 2. HTTPS Support

Implement secure communication using **TLS/HTTPS** with ESP-IDF `esp_https_server`.

### 3. Authentication

Add username/password or token-based authentication to protect REST API endpoints.

### 4. Persistent Configuration

Store sampling intervals and thresholds in ESP32 **NVS** so configuration remains after reboot.

### 5. MQTT Integration

Add MQTT communication for connecting the ESP32 to cloud or IoT platforms.

### 6. OTA Updates

Add Over-The-Air firmware updates so new firmware can be installed without USB.

### 7. Advanced Web Dashboard

Add:

* Live temperature graphs
* Humidity graphs
* Relay control
* Configuration panel
* Device status
* Network information

### 8. Security Improvements

Add:

* HTTPS
* Authentication
* Secure credential storage
* Input validation
* Certificate management

### 9. Hardware Expansion

Add additional sensors such as:

* Light sensor
* Soil moisture sensor
* Ultrasonic sensor
* Pressure sensor
* Air-quality sensor


