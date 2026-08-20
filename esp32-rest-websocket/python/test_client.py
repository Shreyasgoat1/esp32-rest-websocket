import json
import time
import requests
import websocket

ESP32_IP = "192.168.1.105"
BASE_URL = f"http://{ESP32_IP}"
WS_URL = f"ws://{ESP32_IP}/ws"


def request_with_retry(method, url, retries=3, **kwargs):
    for attempt in range(retries):
        try:
            response = requests.request(method, url, timeout=5, **kwargs)
            response.raise_for_status()
            return response
        except requests.RequestException as exc:
            print(f"Request failed ({attempt + 1}/{retries}): {exc}")
            if attempt < retries - 1:
                time.sleep(2)
    return None


def print_json_response(response):
    if response is None:
        return
    try:
        print(json.dumps(response.json(), indent=4))
    except ValueError:
        print(response.text)


def get_sensors():
    print("\n=== GET /sensors ===")
    response = request_with_retry("GET", f"{BASE_URL}/sensors")
    print_json_response(response)


def update_config():
    print("\n=== POST /config ===")
    payload = {
        "sampling_ms": 3000,
        "temperature_threshold": 32,
        "humidity_threshold": 75
    }
    response = request_with_retry(
        "POST",
        f"{BASE_URL}/config",
        json=payload,
        headers={"Content-Type": "application/json"}
    )
    print_json_response(response)


def set_relay(state):
    print(f"\n=== PUT /relay -> {state} ===")
    response = request_with_retry(
        "PUT",
        f"{BASE_URL}/relay",
        json={"state": state},
        headers={"Content-Type": "application/json"}
    )
    print_json_response(response)


def websocket_client():
    print("\n=== WebSocket ===")
    for attempt in range(3):
        try:
            print(f"Connecting to {WS_URL}")
            ws = websocket.create_connection(WS_URL, timeout=10)
            print("WebSocket connected.")

            while True:
                message = ws.recv()
                if not message:
                    print("Connection closed.")
                    break

                try:
                    data = json.loads(message)
                    print("\nLive sensor data:")
                    print(json.dumps(data, indent=4))
                except json.JSONDecodeError:
                    print("Received non-JSON data:", message)

        except Exception as exc:
            print(f"WebSocket error ({attempt + 1}/3): {exc}")
            if attempt < 2:
                time.sleep(2)
            else:
                print("Unable to connect to WebSocket.")


def main():
    print("ESP32 REST API + WebSocket Test Client")
    get_sensors()
    update_config()
    set_relay(True)
    time.sleep(2)
    get_sensors()
    set_relay(False)
    websocket_client()


if __name__ == "__main__":
    main()
