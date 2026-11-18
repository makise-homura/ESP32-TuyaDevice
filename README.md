# Arduino-ESP32-TuyaDevice

An Arduino library to help converting your ESP32 to fully functional IoT device that can be controlled from SmartLife without any Python intermediates.

## Brief

Using this library, you may control you ESP32 (e.g., turning some GPIOs on/off or so) using Tuya SmartLife application the same way as if it is a standalone Wi-Fi IoT device.

This library is a deep rework of [Tuya-ledapp](https://github.com/JayeshRajam/Tuya-ledapp) by [JayeshRajam](https://github.com/JayeshRajam).
The original code does support controlling ESP32 from Tuya SmartLife, but it also requires an intermediate PC running a Python script.
That script is performing all Tuya-specific operations, and ESP32 there is nothing more than just a physical interface to a LED.

The goal of this code is to eliminate the need of an intermediate PC due to putting all logic into the ESP32.

## Requirements

* ESP32 board (e.g. ESP32-C3 Super Mini)
* Wi-Fi network with internet access
* A smartphone with Tuya SmartLife application

### To build and flash firmware

* A PC runnung Arduino IDE 2.x.
* Installed Espressif esp32 board pack for Arduino IDE.
* Installed Arduino libraries: Arduino/Arduino_JSON (required) and Nick O'Leary/PubSubClient (required).

## Preparations

First, read the [article](https://www.cnx-software.com/2021/12/13/blink-an-led-on-esp32-board-with-tuya-link-sdk/) about creating your own app.
You should perform the steps in *Software setup* chapter until you get the two licenses for your devices (you may have need only one if you configure just one device).
To this moment, you should know your Product ID (you always can find it on *AI Product*/*Development* page on [Tuya Platform website](https://platform.tuya.com)),
and have two pairs of UUID and authentication key in the XLSX file you downloaded. Also you should have configured a panel for your app with desired controls,
and know data point numbers for your app.

**Note**: if you have troubles with not being able to confirm your identity, check that you created your project using a data center not in China.

## Typical client architecture

User side of a client consists of:
* creating a `TuyaClient` instance (usually as a global variable);
* initial call to `TuyaClient::begin()` (usually in `begin()`);
* periodical call to `TuyaClient::loop()` (usually in `loop()`);
* optional calls to `TuyaClient::disconnect()` and `TuyaClient::begin()` again, if client is to be stopped and started again;
* optional calls to `TuyaClient::publish()` to pass data to the SmartLife side;
* a callback for a low-level "MQTT message received" event;
* callbacks for high-level requests from client: "display QR code", "react to activation", "react to reset", "react to data point change".

### Creating and managing an instance

To create an instance, do the following:

```
#include <ESP32Tuya.h>

TuyaClient tuya(https, time, onMessage, TUYA_PID, TUYA_UUID, TUYA_AUTHKEY);
```

* `https`: a `NetworkClientSecure` instance (usually `WiFiClientSecure`) used to communicate with the internet
* `time`: a function (`uint32_t (*)(void)`) used to get the current timestamp, just like `time()` in C stdlib
* `TUYA_PID`, `TUYA_UUID`, `TUYA_AUTHKEY`: auth data you received from Tuya

Then, set all needed callbacks, call `tuya.begin()` (usually in `begin()` after internet is reachable), and then periodically call `tuya.loop()` (usually in `loop()`).

If you need to stop client, call `tuya.disconnect()` (e.g. on internet connection loss), and `tuya.begin()` to restart it again.

`begin()` and `loop()` return `true` on success, `false` otherwise (i.e. in case of connection lost or could not have been established).

`disconnect()` does not return anything.

### Publishing data

If you want to pass changes in data points to the SmartLife side, call:

```
tuya.publish(data);
```

Here, `data` is stringified JSON object of one entry, which key is data point number, and value is a new value.

Examples:

```
tuya.publish("{\"101\":false}"); // Signal that the LED (data point 101) is turned OFF
tuya.publish("{\"102\":true}");  // Signal that the GPIO2 (data point 103) reading is HIGH
tuya.publish("{\"134\":2}");     // Signal that the two-bit GPIO3/4 pair (data point 134) reading is binary 10 (decimal 2)
```

`publish()` returns `true` on success, `false` on failure.

### Callback for a low-level "MQTT message received" event

This callback lets user to perform some duties before and after arrived MQTT packet is handled.

Inside it, user shall pass the received packet further to client to handle.

```
void onMessage(char *topic, uint8_t *payload, unsigned int length)
{
    /* Do something before arrived packet is handled by client (e.g. filter out some packets) */

    // Mandatory: pass message to inner handler
    bool is_ok = tuya.onMessage(topic, payload, length);

    /* React to result (is_ok) and do something after packet has been handled (e.g. log the result) */
}
```

* `topic`: a topic to which this packet is designated. It is `d/ai/...` while activating, `smart/device/in/...` otherwise.
* `payload`: binary payload of the packet.
* `length`: length of the payload.

This callback, as well as call to `TuyaClient::onMessage()` inside, is mandatory for client to be functioning.

Payload structure:
```
char[3] protocol = {'2', '.', '2'};
uint32_t crc;  // CRC (low-endian) of all remaining payload (sequence + source + data), network byte order
uint32_t sequence; // Sequence number, network byte order
uint32_t source; // Source identifier, network byte order
uint8_t[] data; // JSON object, PKCS#7-padded and encrypted by secret key (TUYA_AUTHKEY while activating, localKey from ctivation data otherwise)
```

JSON object in `data` contains `data` and `protocol` entries (along with others, like a timestamp). The latter may have the following values:
* `5`: MQ_DPS_DATA_ON, data points received, `data = {"dps": {"DATA_POINT_NUMBER": DATA_POINT_VALUE}}`
* `11`: MQ_RESET_ON, activation reset
* `46`: MQ_ACTIVE_TOKEN_ON, activation successful, `data` is activation data.

### Callbacks for high-level requests

There are four optional callbacks from client: "display QR code", "react to activation", "react to reset", "react to data point change".

#### Display QR code

```
void onQRCode(String data)
```

Called when the activation QR code is supposed to be shown to the user. `data` is the URL to make QR code from.

Action expected: show QR code (client will wait until it receives the correct token).

#### React to reset

```
void onReset(String data)
```

Called when device activation data is reset (e.g. device is deleted from SmartLife).

No specific action is expected, but for example, we may reconnect like this:

```
while(!tuya.begin())
{
    Serial.println("Failed to connect, trying again...");
}
Serial.println("Tuya client is ready.");
```

#### React to activation

```
void onActivated(String data)
```

Called after successful activation.

`data` is a stringified JSON object containing activation data and data point schema. Example (after parsing, IDs and keys redacted out):

```
{
    "capability": 1,
    "devId": "xxxxxxxxxxxxxxxxxxxxxx",
    "localKey": "xxxxxxxxxxxxxxxx",
    "resetFactory": false,
    "schema": "[{\"mode\":\"rw\",\"property\":{\"type\":\"bool\"},\"id\":101,\"type\":\"obj\"}]"
    "schemaId": "xxxxxxxx",
    "secKey": "xxxxxxxxxxxxxxxx"
}
```

#### React to data point change

```
void onData(String data)
```

Called when ESP32 side is supposed to react on changes on the SmartLife side (e.g. user changed something in panel elements).

No specific action is expected.

`data` is a stringified JSON object of one entry, which key is data point number, and value is a new value, for example: `{"101": false}`.

It is expected that to confirm changes, ESP32 will push new data back:

```
tuya.publish(data);
```

## Minimal example

```
uint32_t time() { return readTimeStamp(); }

WiFiClientSecure https;

TuyaClient tuya(https, time, onMessage, TUYA_PID, TUYA_UUID, TUYA_AUTHKEY);

void onMessage(char *topic, uint8_t *payload, unsigned int length)
{
    tuya.onMessage(topic, payload, length);
}

void onQRCode(String url)
{
    printQRCode(url);
}

void onReset(String data)
{
    while(!tuya.begin()) delay(100);
}

void onData(String data)
{
    parseJSONAndReactToChanges(data);

    tuya.publish(data);
}

void begin()
{
    connectWiFiAndInternet();

    tuya.onQRCode = onQRCode;
    tuya.onReset = onReset;
    tuya.onData = onData;
    /* tuya.onActivated is not set: we have nothing to do there */

    if(!tuya.begin()) produceErrorMessageAndStop();
}

void loop()
{
    tuya.loop();

    if(shutdownRequested())
    {
        tyua.disconnect();

        stopExecutionOrStartAgain();
    }
}
```

The `readTimeStamp()`, `printQRCode()`, `parseJSONAndReactToChanges()`, `connectWiFiAndInternet()`, `produceErrorMessageAndStop()`, `shutdownRequested()`, and `stopExecutionOrStartAgain()` functions are subject to implementation.

## Certificates

To correctly work with HTTPS, one should use the root CA certificates.

They are supplied by a public static method `TuyaClient::getCert(String name)`. `name` might be `"tuya"` or `"iot_dns"`.

## Logging

The client implementation uses standard ESP32 logging facility. You may control it through Arduino IDE Tools menu, setting Core Debug Level.

The recommended logging level for production build is Info, but feel free to switch to Debug or even Verbose if you need dumps and raw JSON data.
