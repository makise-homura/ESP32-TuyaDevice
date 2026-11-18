#ifndef ESP32_TUYA_H
#define ESP32_TUYA_H

#include <Preferences.h>
#include <Arduino_JSON.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

class TuyaClient: private PubSubClient
{
    private:
        String _productId;
        String _uuid;
        String _authKey;
        String _version;
        String _regist;

        String _devId;
        String _secretKey;
        String _signKey;
        int _sequence;
        String _password;
        String _clientId;
        String _topicIn;
        String _topicOut;
        JSONVar _endpoint;
        String _activated;
        String _region;

        Preferences _preferences;
        WiFiClientSecure &_client;
        uint32_t (*_timefunc)(void);

        MQTT_CALLBACK_SIGNATURE;

        struct __attribute__((packed)) header_t
        {
            char protocol[3];
            uint32_t crc32;
            uint32_t sequence;
            uint32_t source;
        };

        static String _hexDump(const uint8_t *data, unsigned int length, String delim = " ");
        static String _md5(String str);
        static uint8_t *_encrypt(String key, String data, size_t &size);
        static uint8_t *_decrypt(String key, const uint8_t *data, size_t size);
        static String _encryptHex(String key, String data);
        static String _decryptBase64(String key, String data);
        static void _encodePacket(String key, int sequence, int source, String data, char **buffer, size_t &size);
        static JSONVar _decodePacket(String key, uint8_t *data, unsigned int length, uint32_t &sequence, uint32_t &source, uint32_t &crc32);

        bool _connect(void);

    public:
        TuyaClient(WiFiClientSecure &client, uint32_t (*timefunc)(void), MQTT_CALLBACK_SIGNATURE, String productId, String uuid, String authKey, String version = "1.0.0", String regist = "pro");
        ~TuyaClient(void);

        bool begin(void);
        bool onMessage(char *topic, uint8_t *payload, unsigned int length);
        bool publish(String dps);
        using PubSubClient::loop;
        using PubSubClient::disconnect;

        void (*onData)(String data);
        void (*onReset)(String data);
        void (*onQRCode)(String data);
        void (*onActivated)(String data);
};

#endif
