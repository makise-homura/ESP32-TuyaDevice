#include "ESP32Tuya.h"
#include <esp32-hal-log.h>
#include <WiFi.h>
#include <mbedtls/base64.h>
#include <rom/crc.h>
#include <mbedtls/md5.h>
#include <mbedtls/aes.h>
#include <HTTPClient.h>

String TuyaClient::getCert(String name)
{
    String tuya_cacert_pem =
        "-----BEGIN CERTIFICATE-----\n"
        "MIIDxTCCAq2gAwIBAgIBADANBgkqhkiG9w0BAQsFADCBgzELMAkGA1UEBhMCVVMx\n"
        "EDAOBgNVBAgTB0FyaXpvbmExEzARBgNVBAcTClNjb3R0c2RhbGUxGjAYBgNVBAoT\n"
        "EUdvRGFkZHkuY29tLCBJbmMuMTEwLwYDVQQDEyhHbyBEYWRkeSBSb290IENlcnRp\n"
        "ZmljYXRlIEF1dGhvcml0eSAtIEcyMB4XDTA5MDkwMTAwMDAwMFoXDTM3MTIzMTIz\n"
        "NTk1OVowgYMxCzAJBgNVBAYTAlVTMRAwDgYDVQQIEwdBcml6b25hMRMwEQYDVQQH\n"
        "EwpTY290dHNkYWxlMRowGAYDVQQKExFHb0RhZGR5LmNvbSwgSW5jLjExMC8GA1UE\n"
        "AxMoR28gRGFkZHkgUm9vdCBDZXJ0aWZpY2F0ZSBBdXRob3JpdHkgLSBHMjCCASIw\n"
        "DQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAL9xYgjx+lk09xvJGKP3gElY6SKD\n"
        "E6bFIEMBO4Tx5oVJnyfq9oQbTqC023CYxzIBsQU+B07u9PpPL1kwIuerGVZr4oAH\n"
        "/PMWdYA5UXvl+TW2dE6pjYIT5LY/qQOD+qK+ihVqf94Lw7YZFAXK6sOoBJQ7Rnwy\n"
        "DfMAZiLIjWltNowRGLfTshxgtDj6AozO091GB94KPutdfMh8+7ArU6SSYmlRJQVh\n"
        "GkSBjCypQ5Yj36w6gZoOKcUcqeldHraenjAKOc7xiID7S13MMuyFYkMlNAJWJwGR\n"
        "tDtwKj9useiciAF9n9T521NtYJ2/LOdYq7hfRvzOxBsDPAnrSTFcaUaz4EcCAwEA\n"
        "AaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAQYwHQYDVR0OBBYE\n"
        "FDqahQcQZyi27/a9BUFuIMGU2g/eMA0GCSqGSIb3DQEBCwUAA4IBAQCZ21151fmX\n"
        "WWcDYfF+OwYxdS2hII5PZYe096acvNjpL9DbWu7PdIxztDhC2gV7+AJ1uP2lsdeu\n"
        "9tfeE8tTEH6KRtGX+rcuKxGrkLAngPnon1rpN5+r5N9ss4UXnT3ZJE95kTXWXwTr\n"
        "gIOrmgIttRD02JDHBHNA7XIloKmf7J6raBKZV8aPEjoJpL1E/QYVN8Gb5DKj7Tjo\n"
        "2GTzLH4U/ALqn83/B2gX2yKQOC16jdFU8WnjXzPKej17CuPKf1855eJ1usV2GDPO\n"
        "LPAvTK33sefOT6jEm0pUBsV/fdUID+Ic/n4XuKxe9tQWskMJDE32p2u0mYRlynqI\n"
        "4uJEvlz36hz1\n"
        "-----END CERTIFICATE-----\n";

    String iot_dns_cacert_pem =
        "-----BEGIN CERTIFICATE-----\n"
        "MIIF3jCCA8agAwIBAgIQAf1tMPyjylGoG7xkDjUDLTANBgkqhkiG9w0BAQwFADCB\n"
        "iDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0pl\n"
        "cnNleSBDaXR5MR4wHAYDVQQKExVUaGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNV\n"
        "BAMTJVVTRVJUcnVzdCBSU0EgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMTAw\n"
        "MjAxMDAwMDAwWhcNMzgwMTE4MjM1OTU5WjCBiDELMAkGA1UEBhMCVVMxEzARBgNV\n"
        "BAgTCk5ldyBKZXJzZXkxFDASBgNVBAcTC0plcnNleSBDaXR5MR4wHAYDVQQKExVU\n"
        "aGUgVVNFUlRSVVNUIE5ldHdvcmsxLjAsBgNVBAMTJVVTRVJUcnVzdCBSU0EgQ2Vy\n"
        "dGlmaWNhdGlvbiBBdXRob3JpdHkwggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIK\n"
        "AoICAQCAEmUXNg7D2wiz0KxXDXbtzSfTTK1Qg2HiqiBNCS1kCdzOiZ/MPans9s/B\n"
        "3PHTsdZ7NygRK0faOca8Ohm0X6a9fZ2jY0K2dvKpOyuR+OJv0OwWIJAJPuLodMkY\n"
        "tJHUYmTbf6MG8YgYapAiPLz+E/CHFHv25B+O1ORRxhFnRghRy4YUVD+8M/5+bJz/\n"
        "Fp0YvVGONaanZshyZ9shZrHUm3gDwFA66Mzw3LyeTP6vBZY1H1dat//O+T23LLb2\n"
        "VN3I5xI6Ta5MirdcmrS3ID3KfyI0rn47aGYBROcBTkZTmzNg95S+UzeQc0PzMsNT\n"
        "79uq/nROacdrjGCT3sTHDN/hMq7MkztReJVni+49Vv4M0GkPGw/zJSZrM233bkf6\n"
        "c0Plfg6lZrEpfDKEY1WJxA3Bk1QwGROs0303p+tdOmw1XNtB1xLaqUkL39iAigmT\n"
        "Yo61Zs8liM2EuLE/pDkP2QKe6xJMlXzzawWpXhaDzLhn4ugTncxbgtNMs+1b/97l\n"
        "c6wjOy0AvzVVdAlJ2ElYGn+SNuZRkg7zJn0cTRe8yexDJtC/QV9AqURE9JnnV4ee\n"
        "UB9XVKg+/XRjL7FQZQnmWEIuQxpMtPAlR1n6BB6T1CZGSlCBst6+eLf8ZxXhyVeE\n"
        "Hg9j1uliutZfVS7qXMYoCAQlObgOK6nyTJccBz8NUvXt7y+CDwIDAQABo0IwQDAd\n"
        "BgNVHQ4EFgQUU3m/WqorSs9UgOHYm8Cd8rIDZsswDgYDVR0PAQH/BAQDAgEGMA8G\n"
        "A1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQEMBQADggIBAFzUfA3P9wF9QZllDHPF\n"
        "Up/L+M+ZBn8b2kMVn54CVVeWFPFSPCeHlCjtHzoBN6J2/FNQwISbxmtOuowhT6KO\n"
        "VWKR82kV2LyI48SqC/3vqOlLVSoGIG1VeCkZ7l8wXEskEVX/JJpuXior7gtNn3/3\n"
        "ATiUFJVDBwn7YKnuHKsSjKCaXqeYalltiz8I+8jRRa8YFWSQEg9zKC7F4iRO/Fjs\n"
        "8PRF/iKz6y+O0tlFYQXBl2+odnKPi4w2r78NBc5xjeambx9spnFixdjQg3IM8WcR\n"
        "iQycE0xyNN+81XHfqnHd4blsjDwSXWXavVcStkNr/+XeTWYRUc+ZruwXtuhxkYze\n"
        "Sf7dNXGiFSeUHM9h4ya7b6NnJSFd5t0dCy5oGzuCr+yDZ4XUmFF0sbmZgIn/f3gZ\n"
        "XHlKYC6SQK5MNyosycdiyA5d9zZbyuAlJQG03RoHnHcAP9Dc1ew91Pq7P8yF1m9/\n"
        "qS3fuQL39ZeatTXaw2ewh0qpKJ4jjv9cJ2vhsE/zB+4ALtRZh8tSQZXq9EfX7mRB\n"
        "VXyNWQKV3WKdwrnuWih0hKWbt5DHDAff9Yk2dDLWKMGwsAvgnEzDHNb842m1R0aB\n"
        "L6KCq9NjRHDEjf8tM7qtj3u1cIiuPhnPQCjY/MiQu12ZIvVS5ljFH4gxQ+6IHdfG\n"
        "jjxDah2nGN59PRbxYvnKkKj9\n"
        "-----END CERTIFICATE-----\n";

    if (name == "tuya") return tuya_cacert_pem;
    if (name == "iot_dns") return iot_dns_cacert_pem;
    return "";
}

String TuyaClient::_hexDump(const uint8_t *data, unsigned int length, String delim)
{
    String ret = "";
    for(int i = 0; i < length; i++)
    {
        String x(data[i], HEX);
        if(x.length() == 1) x = "0" + x;
        ret += delim + x;
    }
    return ret.substring(delim.length());
}

String TuyaClient::_md5(String str)
{
    uint8_t output[16];
    mbedtls_md5((const uint8_t*)str.c_str(), str.length(), output);
    return _hexDump(output, 16, "");
}

uint8_t *TuyaClient::_encrypt(String key, String data, size_t &size)
{
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, (const uint8_t*)key.c_str(), 128);

    size = (data.length() / 16 + 1) * 16;
    uint8_t pad = size - data.length();
    uint8_t inData[size];
    uint8_t *outData = (uint8_t*)malloc(size);

    memset(inData, pad, size);
    memcpy(inData, data.c_str(), data.length());

    for(size_t i = 0; i < size; i += 16)
    {
        mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, inData + i, outData + i);
    }

    return outData;
}

uint8_t *TuyaClient::_decrypt(String key, const uint8_t *data, size_t size)
{
    if(size % 16)
    {
        log_w("ERROR: length of data to decrypt (%d) is not a multiple of 16!", size);
        return NULL;
    }

    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_dec(&aes, (const uint8_t*)key.c_str(), 128);

    uint8_t *outData = (uint8_t*)malloc(size);

    for(size_t i = 0; i < size; i += 16)
    {
        mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, data + i, outData + i);
    }

    uint8_t pad = outData[size - 1];
    outData[size - pad] = '\0';
    return outData;
}

String TuyaClient::_encryptHex(String key, String data)
{
    size_t size;
    uint8_t *outData = _encrypt(key, data, size);
    String out = _hexDump(outData, size, "");
    free(outData);
    out.toUpperCase();
    return "data=" + out;
}


String TuyaClient::_decryptBase64(String key, String data)
{
    size_t size;
    mbedtls_base64_decode(NULL, 0, &size, (const uint8_t*)data.c_str(), data.length());
    uint8_t buffer[size];
    mbedtls_base64_decode(buffer, size, &size, (const uint8_t*)data.c_str(), data.length());
    uint8_t *outData = _decrypt(key, buffer, size);
    if(!outData) return "";
    String out = (char*)outData;
    free(outData);
    return out;
}

void TuyaClient::_encodePacket(String key, int sequence, int source, String data, char **buffer, size_t &size)
{
    struct header_t header;
    strncpy(header.protocol, "2.2", 3);
    header.sequence = htonl(sequence);
    header.source = htonl(source);
    uint8_t *outData = _encrypt(key, data, size);
    header.crc32 = crc32_le(0, ((uint8_t*)&header) + 7, 8);
    header.crc32 = crc32_le(header.crc32, outData, size);
    header.crc32 = htonl(header.crc32);
    *buffer = (char*)malloc(size + sizeof(header));
    memcpy(*buffer, &header, sizeof(header));
    memcpy(*buffer + sizeof(header), outData, size);
    size += sizeof(header);
    free(outData);
}

JSONVar TuyaClient::_decodePacket(String key, uint8_t *data, unsigned int length, uint32_t &sequence, uint32_t &source, uint32_t &crc32)
{
    struct header_t *header = (struct header_t*)data;
    crc32 = ntohl(header->crc32);
    sequence = ntohl(header->sequence);
    source = ntohl(header->source);
    if(strncmp("2.2", header->protocol, 3))
    {
        log_w("ERROR: protocol (%3s) is not 2.2!", header->protocol);
        return JSONVar();
    }
    uint32_t actualCrc = crc32_le(0, data + 7, length - 7);
    if(actualCrc != crc32)
    {
        log_w("ERROR: CRC32 check failed (actual %08x != %08x in header)!", actualCrc, crc32);
        return JSONVar();
    }
    data += 15;
    length -= 15;
    uint8_t *json = _decrypt(key, data, length);
    if(!json) return JSONVar();
    JSONVar ret = JSON.parse((char*)json);
    free(json);
    return ret;
}

TuyaClient::~TuyaClient(void)
{
    disconnect();
}


bool TuyaClient::onMessage(char *topic, uint8_t *payload, unsigned int length)
{
    log_d("MQTT message received with topic %s and length %d bytes", topic, length);
    if(_topicIn != topic)
    {
        log_w("Topics do not match, ignoring message.");
        return false;
    }

    uint32_t sequence, source, crc32;
    JSONVar json = _decodePacket(_secretKey, payload, length, sequence, source, crc32);
    _sequence = sequence;
    log_v("Message contents: %s", JSON.stringify(json));
    int protocol = json["protocol"];
    switch(protocol)
    {
        case 5: /* MQ_DPS_DATA_ON */
            if(_clientId == _devId)
            {
                log_d("DPS message received: %s", JSON.stringify(json));
                if(onData) onData(JSON.stringify(json["data"]["dps"]));
                return true;
            }
            break;
        case 11: /* MQ_RESET_ON */
            if(_clientId == _devId)
            {
                log_d("Activation reset: %s", JSON.stringify(json));
                _preferences.begin("TuyaData", false);
                _preferences.remove("Activated");
                _preferences.end();
                disconnect();
                if(onReset) onReset(JSON.stringify(json["data"]));
                return true;
            }
            break;
        case 46: /* MQ_ACTIVE_TOKEN_ON */
            if(_clientId == "acon_" + _devId)
            {
                log_d("Received token: %s", JSON.stringify(json));
                String region = json["data"]["region"];
                String token = json["data"]["token"];
                uint32_t t = _timefunc();

                JSONVar options;
                options["isFK"] = false;
                JSONVar data;
                data["token"] = token;
                data["productKey"] = _productId;
                data["softVer"] = _version;
                data["protocolVer"] = "2.2";
                data["baselineVer"] = "40.00";
                data["options"] = JSON.stringify(options);
                data["cadVer"] = "1.0.3";
                data["cdVer"] = "1.0.0";
                data["t"] = t;
                String urlbase = _endpoint["atop"]["url"];
                String sbuf = String("a=tuya.device.active||et=1||t=") + t + "||uuid=" + _uuid + "||v=4.3||" + _authKey;
                String url = urlbase + "?a=tuya.device.active&et=1&t=" + t + "&uuid=" + _uuid + "&v=4.3&sign=" + _md5(sbuf);
                log_v("sbuf: %s", sbuf);
                log_v("url: %s", url);

                HTTPClient http;
                http.begin(_client, url);
                http.addHeader("User-Agent", "TUYA_IOT_SDK");
                http.addHeader("Content-Type", "application/x-www-form-urlencoded; charset=UTF-8");
                String payload = _encryptHex(_authKey, JSON.stringify(data));
                log_v("HTTP: Sending data: %s", payload);
                int responseCode = http.POST(payload);
                String responseText = http.getString();
                log_v("HTTP: Response: %d, payload: %s", responseCode, responseText);
                if(responseCode != 200)
                {
                    log_e("HTTP: Can not reach activation endpoint %s.", url);
                    return false;
                }
                JSONVar response = JSON.parse(responseText);
                http.end();
                if(!response.hasOwnProperty("result"))
                {
                    log_e("Bad payload format of activation response.");
                    return false;
                }
                String decrypted = _decryptBase64(_authKey, response["result"]);
                JSONVar decryptedJson = JSON.parse(decrypted);
                if(!decryptedJson["success"])
                {
                    log_e("Activation failed: %s", decrypted);
                    return false;
                }
                _region = region;
                _activated = JSON.stringify(decryptedJson["result"]);
                log_i("Activation is successful.");
                log_d("Activation result: %s", _activated);
                return true;
            }
            break;
    }
    log_w("Not expected a message of this type (%d) now, skipping.", protocol);
    return false;
}

bool TuyaClient::publish(String dps)
{
    log_i("Publishing data points: %s", dps);
    JSONVar dpsData = JSON.parse(dps);
    JSONVar data;
    data["dps"] = dpsData;
    data["devId"] = _devId;
    JSONVar payload;
    payload["protocol"] = 4;
    payload["t"] = _timefunc();
    payload["data"] = data;
    String payloadStr = JSON.stringify(payload);
    ++_sequence;
    size_t size;
    uint8_t *buffer;
    _encodePacket(_secretKey, _sequence, 1, payloadStr, (char**)&buffer, size);
    bool ret = PubSubClient::publish(_topicOut.c_str(), buffer, size, false);
    free(buffer);
    return ret;
}

TuyaClient::TuyaClient(NetworkClientSecure &client, uint32_t (*timefunc)(void), MQTT_CALLBACK_SIGNATURE, String productId, String uuid, String authKey, String version, String regist): _client(client)
{
    _timefunc = timefunc;
    this->callback = callback;

    _productId = productId;
    _uuid = uuid;
    _authKey = authKey;
    _version = version;
    _regist = regist;

    onData = NULL;
    onReset = NULL;
    onQRCode = NULL;
    onActivated = NULL;
}

bool TuyaClient::_connect(void)
{
    _sequence = 0;
    _password = _md5(_signKey).substring(8,24);
    log_d("MQTT connect: devId = %s, secretKey = %s, signKey = %s, password = %s, clientId = %s", _devId, _secretKey, _signKey, _password, _clientId);
    if(connect(_clientId.c_str(), _clientId.c_str(), _password.c_str()))
    {
        log_d("Connected, subscribing to %s", _topicIn);
        return subscribe(_topicIn.c_str(), 1);
    }
    log_e("Connect failed, state: %d", state());
    return false;
}

bool TuyaClient::begin(void)
{
    setClient(_client);
    setCallback(callback);
    setKeepAlive(100);
    setBufferSize(16384);
    if(!_endpoint.hasOwnProperty("mqtt"))
    {
        log_i("No endpoint yet, requesting...");
        JSONVar configHttps;
        configHttps["key"] = "httpsUrl";
        configHttps["need_ca"] = true;
        JSONVar configMqtts;
        configMqtts["key"] = "mqttsUrl";
        configMqtts["need_ca"] = true;
        JSONVar configArray;
        configArray[0] = configHttps;
        configArray[1] = configMqtts;
        JSONVar config;
        config["config"] = configArray;
        if(_region != "")
        {
            config["region"] = _region;
            config["env"] = _regist;
        }

        HTTPClient http;
        String url = "https://h1.iot-dns.com/v1/url_config";
        String cert = getCert("iot_dns");
        _client.setCACert(cert.c_str());
        http.begin(_client, url);
        http.addHeader("Content-Type", "application/json");
        String payload = JSON.stringify(config);
        log_d("Sending JSON: %s", payload);
        int responseCode = http.POST(payload);
        String responseText = http.getString();
        log_v("HTTP: Response: %d, payload: %s", responseCode, responseText);
        if(responseCode != 200)
        {
            log_e("HTTP: Can not reach endpoint config site %s.", url);
            return false;
        }
        JSONVar response = JSON.parse(responseText);
        http.end();
        if(!response.hasOwnProperty("mqttsUrl") || !response.hasOwnProperty("httpsUrl"))
        {
            log_e("Bad contents of endpoint query response.");
            return false;
        }

        JSONVar endpointMqtts;
        String mqttSrv = response["mqttsUrl"]["addr"];
        String atopSrv = response["httpsUrl"]["addr"];
        int delim = mqttSrv.indexOf(":");
        if(delim < 0)
        {
            log_e("Endpoint does not contain port number.");
            return false;
        }
        endpointMqtts["host"] = mqttSrv.substring(0, delim);
        endpointMqtts["port"] = mqttSrv.substring(delim + 1).toInt();
        JSONVar endpointHttps;
        endpointHttps["url"] = atopSrv;
        JSONVar endpoint;
        endpoint["mqtt"] = endpointMqtts;
        endpoint["atop"] = endpointHttps;
        log_d("Endpoint received: %s", JSON.stringify(endpoint));
        _endpoint = endpoint;
    }
    String server = _endpoint["mqtt"]["host"];
    int port = _endpoint["mqtt"]["port"];
    log_i("MQTT Server: %s:%d", server, port);
    setServer(server.c_str(), port);
    String cert = getCert("tuya");
    _client.setCACert(cert.c_str());

    _preferences.begin("TuyaData", false);
    _activated = _preferences.getString("Activated", "{}");
    _region = _preferences.getString("Region", "");
    _preferences.end();
    if(_activated == "{}")
    {
        log_i("Not activated yet, requesting activation...");
        _devId = _uuid;
        _secretKey = _authKey;
        _signKey = _authKey;
        _clientId = "acon_" + _devId;
        _topicIn = "d/ai/" + _devId;
        _topicOut = "";
        if(!_connect())
        {
            log_e("Failed to start activation MQTT instance.");
            return false;
        }
        String url = "https://smartapp.tuya.com/s/p?p=" + _productId + "&uuid=" + _uuid + "&v=2.0";
        if(onQRCode) onQRCode(url);
        while(_activated == "{}") loop();
        disconnect();
        _preferences.begin("TuyaData", false);
        _preferences.putString("Activated", _activated);
        _preferences.putString("Region", _region);
        _preferences.end();
        if(onActivated) onActivated(_activated);
    }
    JSONVar activationData = JSON.parse(_activated);
    String devId = activationData["devId"];
    String localKey = activationData["localKey"];
    String secKey = activationData["secKey"];
    _devId = devId;
    _secretKey = localKey;
    _signKey = secKey;
    _clientId = _devId;
    _topicIn = "smart/device/in/" + _devId;
    _topicOut = "smart/device/out/" + _devId;
    if(!_connect())
    {
        log_e("Failed to start main MQTT instance.");
        return false;
    }
    return true;
}
