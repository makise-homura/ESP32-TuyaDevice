/************** Time and NTP **************/

#include <WiFiUdp.h>

WiFiUDP ntpUDP;

#include <NTPClient.h>

NTPClient timeClient(ntpUDP);

uint32_t time(void)
{
    timeClient.update();
    return timeClient.getEpochTime();
}

/************** LED **************/

#define LED_PIN 8

#define DEFAULT_LED_STATE off

class LED
{
    private:
        int _pin;
        bool _state;

    public:
        LED(int pin);
        void begin(void);
        void on(void);
        void off(void);
        bool state(void) const { return _state; }
};

LED::LED(int pin)
{
    _pin = pin;
}

void LED::begin(void)
{
    pinMode(_pin, OUTPUT);
    DEFAULT_LED_STATE();
}

void LED::on(void)
{
    Serial.println("LED is ON");
    digitalWrite(_pin, LOW);
    _state = true;
}

void LED::off(void)
{
    Serial.println("LED is OFF");
    digitalWrite(_pin, HIGH);
    _state = false;
}

LED led(LED_PIN);

/************** Button **************/

#define BUTTON_PIN 9

class Button
{
    private:
        int _pin;
        int _btnState;
        int _debouncedState;
        unsigned long _lastChange;
        unsigned long _debounceTimeout;

    public:
        Button(int pin, unsigned long debounceTimeout = 20);
        void begin(void);
        bool pressed();
};

Button::Button(int pin, unsigned long debounceTimeout)
{
    _pin = pin;
    _btnState = HIGH;
    _debouncedState = HIGH;
    _lastChange = millis();
    _debounceTimeout = debounceTimeout;
}

void Button::begin(void)
{
    pinMode(_pin, INPUT_PULLUP);
}

bool Button::pressed()
{
    int newState = digitalRead(_pin);
    if(_btnState != newState)
    {
        _btnState = newState;
        _lastChange = millis();
    }
    else
    {
        if(_btnState != _debouncedState && millis() - _lastChange > _debounceTimeout)
        {
            _debouncedState = _btnState;
            return _btnState == LOW;
        }
    }
    return false;
}

Button button(BUTTON_PIN);

/************** Display **************/

#include <U8g2lib.h>

#define DISPLAY_TYPE U8G2_SSD1306_72X40_ER_F_SW_I2C
#define DISPLAY_SCL_PIN 6
#define DISPLAY_SDA_PIN 5
#define DISPLAY_FONT u8g2_font_5x8_mr
#define DISPLAY_ROW_SIZE 8

class Display
{
    private:
        DISPLAY_TYPE *_display;

    public:
        Display(int scl, int sda);
        void begin(void);
        void print(int row, String s);
};

Display::Display(int scl, int sda)
{
    _display = new DISPLAY_TYPE(U8G2_R0, scl, sda);
}

void Display::begin(void)
{
    _display->begin();
    _display->clearBuffer();
    _display->setFont(DISPLAY_FONT);
}

void Display::print(int row, String s)
{
    _display->setCursor(0, DISPLAY_ROW_SIZE * (1 + row));
    _display->println(s);
    _display->sendBuffer();
}

Display display(DISPLAY_SCL_PIN, DISPLAY_SDA_PIN);

/************** Wi-Fi connection **************/

#include <WiFi.h>

#define WIFI_DEFAULT_SSID "ESP32_WiFi"
#define WIFI_DEFAULT_IP "192.168.1.1"
#define WIFI_DEFAULT_NETMASK "255.255.255.0"

#include <Preferences.h>

class WiFiConnection
{
    private:
        wl_status_t _lastStatus;
        bool _configured;
        Preferences _preferences;

    public:
        WiFiConnection(void);
        void begin(String &SSID, String &password);
        void writeAuth(String SSID, String password);
        void reset(void);
        bool statusChanged(wl_status_t &newStatus);
        bool configured(void) const { return _configured; }
};

WiFiConnection::WiFiConnection(void)
{
    _lastStatus = WL_IDLE_STATUS;
    _configured = false;
}

void WiFiConnection::begin(String &SSID, String &password)
{
    _preferences.begin("WiFiData", true);
    _configured = _preferences.getBool("Configured", false);
    _preferences.end();

    if(!_configured)
    {
        SSID = WIFI_DEFAULT_SSID;
        password = "";
        WiFi.softAP(SSID);
        WiFi.softAPConfig(WIFI_DEFAULT_IP, WIFI_DEFAULT_IP, WIFI_DEFAULT_NETMASK);
    }
    else
    {
        _preferences.begin("WiFiData", true);
        SSID = _preferences.getString("SSID", "");
        password = _preferences.getString("password", "");
        _preferences.end();

        WiFi.setHostname("TuyaDevice");
        WiFi.begin(SSID.c_str(), (password.length() > 0) ? password.c_str() : NULL);
        _lastStatus = WiFi.status();
    }
}

void WiFiConnection::writeAuth(String SSID, String password)
{
    _preferences.begin("WiFiData", false);
    _preferences.putString("SSID", SSID);
    _preferences.putString("password", password);
    _preferences.putBool("Configured", true);
    _preferences.end();
}

void WiFiConnection::reset(void)
{
    _preferences.begin("WiFiData", false);
    _preferences.putBool("Configured", false);
    _preferences.end();
}

bool WiFiConnection::statusChanged(wl_status_t &newStatus)
{
    newStatus = WiFi.status();
    if(_lastStatus != newStatus)
    {
        _lastStatus = newStatus;
        return true;
    }
    return false;
}

WiFiConnection wifiConnection;

/************** Web server **************/

#include <WebServer.h>

class AuthServer
{
    private:
        WebServer _webServer;

    public:
        AuthServer(void): _webServer(80) {}

        void begin(void (*printForm)(void), void (*acceptForm)(void));
        void send(String data);

        bool hasArg(String arg) {return _webServer.hasArg(arg);}
        String arg(String arg) {return _webServer.arg(arg);}
        void handleClient(void) {_webServer.handleClient();}
};

void AuthServer::begin(void (*printForm)(void), void (*acceptForm)(void))
{
    _webServer.begin();
    _webServer.on("/", printForm);
    _webServer.on("/conf", HTTP_POST, acceptForm);
}

void AuthServer::send(String data)
{
    _webServer.send(200, "text/html", data);
}

AuthServer authServer;

void Web_printForm()
{
    Serial.println("HTTP Server: sent config page.");
    authServer.send("<!DOCTYPE html><html><head><title>ESP32 Configuration</title></head><body><form method=\"post\" action=\"/conf\">Configure your Wi-Fi network:<table><tr><td>SSID:</td><td><input type=\"text\" name=\"ssid\"></td></tr><tr><td>Password:</td><td><input type=\"text\" name=\"password\"></td></tr><tr><td colspan=2><input type=\"submit\" value=\"Submit\"></td></tr></table></body></html>");
}

void Web_acceptForm()
{
    Serial.println("HTTP Server: received a form.");
    if(!authServer.hasArg("ssid") || !authServer.hasArg("password"))
    {
        Serial.println("HTTP Server: form doesn't have required fields.");
        authServer.send("<!DOCTYPE html><html><head><title>ESP32 Configuration</title></head><body>You should pass SSID and password.<br /><a href=\"/\">Try again</a></body></html>");
        return;
    }
    String SSID = authServer.arg("ssid");
    String password = authServer.arg("password");
    if(authServer.arg("ssid").isEmpty())
    {
        Serial.println("HTTP Server: SSID is empty.");
        authServer.send("<!DOCTYPE html><html><head><title>ESP32 Configuration</title></head><body>SSID must not be empty.<br /><a href=\"/\">Try again</a></body></html>");
        return;
    }
    Serial.println("HTTP Server: SSID is " + SSID + ", password is " + password);
    wifiConnection.writeAuth(SSID, password);
    Serial.println("HTTP Server: config is written, rebooting.");
    authServer.send("<!DOCTYPE html><html><head><title>ESP32 Configuration</title></head><body>Configuration written successfully.</body></html>");
    ESP.restart();
}

/************** Internet **************/

#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ESP32Tuya.h>

class Inet
{
    private:
        bool _ok;

    public:
        Inet(void) {_ok = false;}
        void lost(void) {_ok = false;}
        bool begin(void);
        bool ok(void) const {return _ok;}
};

bool Inet::begin(void)
{
    WiFiClientSecure _https;
    _https.setCACert(TuyaClient::getCert("tuya").c_str());
    HTTPClient _http;
    _http.begin(_https, "https://www.tuya.com");
    int httpCode = _http.GET();
    _http.end();
    _ok = (httpCode == 200);
    return _ok;
}

Inet inet;

/************** Tuya client **************/

#include <qrcode.h>

// Put your values here:
#define TUYA_PID "xxxxxxxxxxxxxxxx"
#define TUYA_UUID "uuidxxxxxxxxxxxxxxxx"
#define TUYA_AUTHKEY "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"

WiFiClientSecure https;
TuyaClient tuya(https, time, onMessage, TUYA_PID, TUYA_UUID, TUYA_AUTHKEY);

void onMessage(char *topic, uint8_t *payload, unsigned int length)
{
    if(!tuya.onMessage(topic, payload, length)) Serial.println("Failed to process message.");
}

void onQRCode(String data)
{
    Serial.println("Open this URL: " + data + ", or scan this QR code:");
    esp_qrcode_config_t qr_config = {esp_qrcode_print_console, 10, ESP_QRCODE_ECC_LOW};
    esp_qrcode_generate(&qr_config, data.c_str());
}

void onReset(String data)
{
    Serial.println("Reset: " + data + ". Reconnecting...");
    display.print(0, "Tuya failed");
    while(!tuya.begin())
    {
        Serial.println("Failed to connect, trying again...");
    }
    Serial.println("Tuya client is ready.");
    display.print(0, "Tuya OK    ");
    JSONVar state;
    state["101"] = led.state();
    tuya.publish(JSON.stringify(state));
}

void onData(String data)
{
    Serial.println("Data points received: " + data);
    JSONVar json = JSON.parse(data);
    if(!json.hasOwnProperty("101"))
    {
        Serial.println("Payload does not contain data point 101.");
        return;
    }
    JSONVar state = json["101"];
    if(JSON.typeof(state) != "boolean")
    {
        Serial.println("Data point 101 has wrong type.");
        return;
    }
    if(state) led.on(); else led.off();
    tuya.publish(data);
}

void onActivated(String data)
{
    Serial.println("Activation is successful!");
}

void setup()
{
    Serial.begin(115200);
    Serial.println("************* STARTED *************");
    display.begin();
    String SSID, password;
    wifiConnection.begin(SSID, password);

    if(!wifiConnection.configured())
    {
        Serial.println(String("Not configured. Connect to ") + SSID + ", IP " + WIFI_DEFAULT_IP);
        display.print(0, "Connect to:");
        display.print(1, " Wi-Fi:");
        display.print(2, String("  ") + SSID);
        display.print(3, " Web (HTTP):");
        display.print(4, String("  ") + WIFI_DEFAULT_IP);
        authServer.begin(Web_printForm, Web_acceptForm);
    }
    else
    {
        Serial.println("Configured. Connecting to " + SSID + ", password \"" + password + "\"...");
        display.print(0, "Connecting to:");
        display.print(1, SSID);
        button.begin();
        led.begin();
        timeClient.begin();
        tuya.onQRCode = onQRCode;
        tuya.onReset = onReset;
        tuya.onData = onData;
        tuya.onActivated = onActivated;
    }
}

bool tuyaConnected = false;

void loop()
{
    if(!wifiConnection.configured())
    {
        authServer.handleClient();
    }
    else
    {
        if(button.pressed())
        {
            wifiConnection.reset();
            Serial.println("Reset to factory defaults, rebooting.");
            ESP.restart();
        }

        wl_status_t status;
        if(wifiConnection.statusChanged(status))
        {
            switch(status)
            {
                case WL_IDLE_STATUS:
                case WL_DISCONNECTED:
                    break;
                case WL_CONNECTED:
                    display.print(0, "Connected     ");
                    display.print(2, WiFi.localIP().toString());
                    Serial.println("WLAN: Connected! IP: " + WiFi.localIP().toString());
                    break;
                case WL_CONNECTION_LOST:
                    display.print(0, "Disconnected  ");
                    Serial.println("WLAN: Connection lost, reconnecting...");
                    break;
                case WL_NO_SSID_AVAIL:
                    display.print(0, "Net not found.");
                    Serial.println("WLAN: Network with this SSID not found.");
                    break;
                case WL_CONNECT_FAILED:
                    display.print(0, "Can't connect.");
                    Serial.println("WLAN: Connection failed. Maybe wrong password?");
                    break;
                default:
                    display.print(0, "Wrong status. ");
                    Serial.println(String("WLAN: Got wrong status ") + status);
                    break;
            }
            if(status == WL_CONNECTED)
            {
                Serial.println("Syncing time...");
                while(!timeClient.isTimeSet() && !timeClient.forceUpdate())
                {
                    Serial.println("Time server is inaccessible, trying again...");
                    delay(1000);
                }
                Serial.println(String("Current time: ") + time());
                display.print(0, "Time OK    ");

                Serial.println("Checking for internet...");
                while(!inet.begin())
                {
                    Serial.println("Internet is inaccessible, trying again...");
                    delay(1000);
                }
                Serial.println("Internet is available!");
                display.print(0, "Internet OK");

                Serial.println("Starting Tuya client...");
                while(!tuya.begin())
                {
                    Serial.println("Failed to connect, trying again...");
                }
                Serial.println("Tuya client is ready.");
                display.print(0, "Tuya OK    ");
                JSONVar state;
                state["101"] = led.state();
                tuya.publish(JSON.stringify(state));
                tuyaConnected = true;
            }
            else if(status != WL_IDLE_STATUS && status != WL_DISCONNECTED)
            {
                Serial.println("Shutting down Tuya client due to lost connection.");
                tuya.disconnect();
                tuyaConnected = false;
                inet.lost();
                delay(1000);
                WiFi.reconnect();
            }
        }
        if(tuyaConnected) tuya.loop();
    }
    delay(100);
}
