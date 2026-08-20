#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include "esp_heap_caps.h"

const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

#define RELAY_PIN 26

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

struct SensorData {
    float temperature;
    float humidity;
    bool relayState;
};

struct DeviceConfig {
    uint32_t samplingIntervalMs;
    float temperatureThreshold;
    float humidityThreshold;
};

SensorData sensorData = {25.0f, 50.0f, false};
DeviceConfig deviceConfig = {2000, 35.0f, 70.0f};

SemaphoreHandle_t dataMutex;
TaskHandle_t sensorTaskHandle = nullptr;
TaskHandle_t serverTaskHandle = nullptr;
TaskHandle_t websocketTaskHandle = nullptr;

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 IoT Gateway</title>
<style>
body{font-family:Arial;background:#f2f2f2;margin:30px}
.card{background:white;padding:20px;max-width:500px;margin:auto;border-radius:10px}
h1{text-align:center}.value{font-size:22px;margin:15px}
button{padding:10px 20px;margin:5px}#status{font-weight:bold}
</style>
</head>
<body>
<div class="card">
<h1>ESP32 IoT Gateway</h1>
<div id="status">Connecting...</div>
<div class="value">Temperature: <span id="temperature">--</span> °C</div>
<div class="value">Humidity: <span id="humidity">--</span> %</div>
<div class="value">Relay: <span id="relay">--</span></div>
<button onclick="setRelay(true)">Relay ON</button>
<button onclick="setRelay(false)">Relay OFF</button>
</div>
<script>
let socket;
function connectWebSocket(){
  socket=new WebSocket("ws://"+window.location.host+"/ws");
  socket.onopen=()=>document.getElementById("status").innerText="WebSocket Connected";
  socket.onclose=()=>{
    document.getElementById("status").innerText="Disconnected - reconnecting...";
    setTimeout(connectWebSocket,2000);
  };
  socket.onerror=()=>document.getElementById("status").innerText="WebSocket Error";
  socket.onmessage=(event)=>{
    const data=JSON.parse(event.data);
    document.getElementById("temperature").innerText=Number(data.temperature).toFixed(2);
    document.getElementById("humidity").innerText=Number(data.humidity).toFixed(2);
    document.getElementById("relay").innerText=data.relay?"ON":"OFF";
  };
}
async function setRelay(state){
  const response=await fetch("/relay",{
    method:"PUT",
    headers:{"Content-Type":"application/json"},
    body:JSON.stringify({state})
  });
  console.log(await response.json());
}
connectWebSocket();
</script>
</body>
</html>
)rawliteral";

SensorData getSensorSnapshot() {
    SensorData copy = sensorData;
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        copy = sensorData;
        xSemaphoreGive(dataMutex);
    }
    return copy;
}

DeviceConfig getConfigSnapshot() {
    DeviceConfig copy = deviceConfig;
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        copy = deviceConfig;
        xSemaphoreGive(dataMutex);
    }
    return copy;
}

String createSensorJson() {
    SensorData data = getSensorSnapshot();
    DeviceConfig config = getConfigSnapshot();

    JsonDocument doc;
    doc["temperature"] = data.temperature;
    doc["humidity"] = data.humidity;
    doc["relay"] = data.relayState;
    doc["sampling_ms"] = config.samplingIntervalMs;
    doc["temperature_threshold"] = config.temperatureThreshold;
    doc["humidity_threshold"] = config.humidityThreshold;
    doc["free_heap"] = ESP.getFreeHeap();
    doc["minimum_free_heap"] = ESP.getMinFreeHeap();
    doc["uptime_ms"] = millis();

    String output;
    serializeJson(doc, output);
    return output;
}

void onWebSocketEvent(AsyncWebSocket* server,
                      AsyncWebSocketClient* client,
                      AwsEventType type,
                      void* arg,
                      uint8_t* data,
                      size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected\n", client->id());
            client->text(createSensorJson());
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            break;
        case WS_EVT_DATA:
            Serial.printf("WebSocket data received from client #%u\n", client->id());
            break;
        case WS_EVT_ERROR:
            Serial.printf("WebSocket error on client #%u\n", client->id());
            break;
        case WS_EVT_PONG:
            break;
    }
}

void handleGetSensors(AsyncWebServerRequest* request) {
    request->send(200, "application/json", createSensorJson());
}

void handleConfig(AsyncWebServerRequest* request, JsonVariant& json) {
    JsonObject obj = json.as<JsonObject>();

    if (obj.isNull()) {
        request->send(400, "application/json", "{\"error\":\"JSON object required\"}");
        return;
    }

    DeviceConfig current = getConfigSnapshot();

    uint32_t samplingInterval = obj["sampling_ms"] | current.samplingIntervalMs;
    float tempThreshold = obj["temperature_threshold"] | current.temperatureThreshold;
    float humidityThreshold = obj["humidity_threshold"] | current.humidityThreshold;

    if (samplingInterval < 100 || samplingInterval > 60000) {
        request->send(400, "application/json", "{\"error\":\"sampling_ms must be 100-60000\"}");
        return;
    }

    if (tempThreshold < -40 || tempThreshold > 100) {
        request->send(400, "application/json", "{\"error\":\"invalid temperature threshold\"}");
        return;
    }

    if (humidityThreshold < 0 || humidityThreshold > 100) {
        request->send(400, "application/json", "{\"error\":\"invalid humidity threshold\"}");
        return;
    }

    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        deviceConfig.samplingIntervalMs = samplingInterval;
        deviceConfig.temperatureThreshold = tempThreshold;
        deviceConfig.humidityThreshold = humidityThreshold;
        xSemaphoreGive(dataMutex);
    }

    JsonDocument doc;
    doc["success"] = true;
    doc["sampling_ms"] = samplingInterval;
    doc["temperature_threshold"] = tempThreshold;
    doc["humidity_threshold"] = humidityThreshold;

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void handleRelay(AsyncWebServerRequest* request, JsonVariant& json) {
    JsonObject obj = json.as<JsonObject>();

    if (obj.isNull() || !obj["state"].is<bool>()) {
        request->send(400, "application/json", "{\"error\":\"state boolean required\"}");
        return;
    }

    bool newState = obj["state"].as<bool>();
    digitalWrite(RELAY_PIN, newState ? HIGH : LOW);

    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        sensorData.relayState = newState;
        xSemaphoreGive(dataMutex);
    }

    JsonDocument doc;
    doc["success"] = true;
    doc["relay"] = newState;

    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
}

void sensorTask(void* parameter) {
    float temperature = 25.0f;
    float humidity = 50.0f;

    for (;;) {
        temperature += 0.1f;
        if (temperature > 30.0f) temperature = 25.0f;

        humidity += 0.2f;
        if (humidity > 80.0f) humidity = 50.0f;

        if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            sensorData.temperature = temperature;
            sensorData.humidity = humidity;
            xSemaphoreGive(dataMutex);
        }

        Serial.printf("Sensor stack remaining: %u words\n",
                      uxTaskGetStackHighWaterMark(nullptr));

        DeviceConfig config = getConfigSnapshot();
        vTaskDelay(pdMS_TO_TICKS(config.samplingIntervalMs));
    }
}

void websocketPushTask(void* parameter) {
    for (;;) {
        ws.textAll(createSensorJson());
        ws.cleanupClients();

        Serial.printf("WebSocket stack remaining: %u words\n",
                      uxTaskGetStackHighWaterMark(nullptr));

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void serverTask(void* parameter) {
    for (;;) {
        ws.cleanupClients();

        Serial.printf("Free heap: %u bytes | Minimum free heap: %u bytes | Largest free block: %u bytes\n",
                      ESP.getFreeHeap(),
                      ESP.getMinFreeHeap(),
                      heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));

        Serial.printf("Server stack remaining: %u words\n",
                      uxTaskGetStackHighWaterMark(nullptr));

        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

void setupRoutes() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send_P(200, "text/html", INDEX_HTML);
    });

    server.on("/sensors", HTTP_GET, handleGetSensors);

    server.on("/heap", HTTP_GET, [](AsyncWebServerRequest* request) {
        JsonDocument doc;
        doc["free_heap"] = ESP.getFreeHeap();
        doc["minimum_free_heap"] = ESP.getMinFreeHeap();
        doc["largest_free_block"] =
            heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);

        String output;
        serializeJson(doc, output);
        request->send(200, "application/json", output);
    });

    auto* configHandler =
        new AsyncCallbackJsonWebHandler("/config", handleConfig);
    server.addHandler(configHandler);

    auto* relayHandler =
        new AsyncCallbackJsonWebHandler("/relay", handleRelay);
    server.addHandler(relayHandler);

    server.onNotFound([](AsyncWebServerRequest* request) {
        JsonDocument doc;
        doc["error"] = "Endpoint not found";
        doc["path"] = request->url();

        String output;
        serializeJson(doc, output);
        request->send(404, "application/json", output);
    });
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\nESP32 REST API + WebSocket Server");

    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);

    dataMutex = xSemaphoreCreateMutex();

    if (dataMutex == nullptr) {
        Serial.println("ERROR: Mutex creation failed");
        while (true) delay(1000);
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("Wi-Fi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    ws.onEvent(onWebSocketEvent);
    server.addHandler(&ws);

    setupRoutes();
    server.begin();

    Serial.println("HTTP server started");

    xTaskCreatePinnedToCore(sensorTask, "SensorTask", 4096, nullptr, 2,
                             &sensorTaskHandle, 1);

    xTaskCreatePinnedToCore(websocketPushTask, "WebSocketTask", 4096, nullptr, 1,
                             &websocketTaskHandle, 1);

    xTaskCreatePinnedToCore(serverTask, "ServerTask", 4096, nullptr, 1,
                             &serverTaskHandle, 0);

    Serial.println("FreeRTOS tasks started");
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}
