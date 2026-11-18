# Full-featured example

This example converts your ESP32-C3 Super Mini or other compatible ESP32 board to, uh... Smart LED?

Anyway, you can control the onboard LED by your Tuya SmartLife application. Based on this example, you may implement remote GPIO controller or so.

## Requirements

* Arduino 2.x with Espressif esp32 boards and oliver/U8g2 and Fabrice Weinberg/NTPClient libraries.
* Something (a PC, a smartphone) with any web browser and an ability to connect to Wi-Fi networks.
* Something that can communicate with ESP32 board by serial port (e.g. Arduino IDE's Serial Monitor, PuTTY in serial mode, or so).
* (recommended) 0.42" OLED I2C display connected to it (some Super Mini boards already have it on board)
* (highly recommended) Working camera on a smartphone with Tuya SmartLife application.
* A panel with a single boolean element representing onboard LED on ESP32-C3 Super Mini that has data point `101` created in corresponding Tuya Platform project.

## Building and flashing

Check out the example code. If you're up to do the same what this code does (turn on/off the onboard LED), and configured LED data point
to be `101` as well, you have nothing to do except setting `TUYA_PID`, `TUYA_UUID`, and `TUYA_AUTHKEY` to the values you've received from Tuya Platform.
After you've done this, just build it with Arduino IDE, program the flash, and voila.

**Note**: if you have no program space when flashing a built binary, select "Minimal SPIFFS" partition scheme in Arduino IDE.

## Configuring Wi-Fi

On the first startup, you have to tell your ESP32 how do you connect to the internet. Initially it does not know your Wi-Fi network SSID/password, so it starts an AP with a web server.
Connect to `ESP32_WiFi` network and go to [http://192.168.1.1](http://192.168.1.1) (note `http`, not `https`!).
You'll see a simple form; enter your network SSID and password into it, and submit the form.
After form submission, data will be saved to non-volatile memory, and then ESP32 will be rebooted and will start trying to connect to network you've provided.

If you wath other default values, just change `WIFI_DEFAULT_SSID`, `WIFI_DEFAULT_IP`, and `WIFI_DEFAULT_NETMASK` in the code.

## Resetting the WiFi configuration

If you've made a mistake in Wi-Fi configuration, or just moved from one place to another with a different network, you may reset SSID and password by pressing a button
(many ESP32-C3 have it marked as `BOOT`) connected to pin 9 (you may also change it by modifying `BUTTON_PIN` in the code). ESP32 will be rebooted, and then you may repeat
Wi-Fi configuration described in the chapter above.

## Adding device to Tuya SmartLife

After ESP32 manages to connect to the Wi-Fi network you provided, it syncs time (for HTTPS to work correctly) and checks if Tuya website is reachable.
If everything is OK, it starts the MQTT client that interacts with your SmartLife application.
Initially, the device is not added. Client can figure this out, and if it is so, it prints out the registration URL and the corresponding QR code to the serial console.
If you missed it, don't worry, just reboot ESP32 by pressing the `RST` button; your SSID and password won't be lost.

If you see the QR code, you re on the right way.
Open your SmartLife application, tap the `+` character in the top right corner, choose `Add device` in the pull-down menu, dismiss a message about Bluetooth,
tap the QR code scanner icon at the top right corner, and scan the code; and then device will be added to SmartLife.

If you're not able to scan a QR code from the console, you may just copy the link above it and use any online QR code generator to obtain a code you can scan.

If you can't scan a QR code at all (like, your smartphone has no working camera?), you have to screenshot the code, upload it to your phone, and then choose this file instead of scanning the code.

## Further operations

Once a device is added, you may open it in the application, and you will see the panel you've constructed before.
Switching a slider in the element will turn it on or off, and the LED on ESP32 board will react to this.

To achieve this, the client does the following:
* Once started, it checks if it knows the endpoint for Tuya API.
* If not, it will reach out to [iot-dns.com](https://h1.iot-dns.com/v1/url_config) to get the HTTPS and MQTTS endpoints.
* Then it checks if it has the activation data (it means the device is added to SmartLife).
* If not, it will request the activation by starting MQTT client with specific parameters, subscribing to `d/ai/...` topic, and calling `onQRCode()` callback to show QR code to the user.
* Once the device is added, a MQTT message from Tuya containing activation data is received by client, processed and saved into non-volatile memory.
* `onActivated()` callback is called after that, if registered by the user.
* If activation data is OK (either from previous step, or a check above), it will then start MQTT client with standard parameters (they include activation data).
* When you change LED state in the SmartLife application, a MQTT message indicating this is received by client and processed by `onData()` callback, so LED takes the chosen state.
* After that, this callback pushes changed data back to Tuya using `publish()` call, to let SmartLife know that its request is processed successfully.
