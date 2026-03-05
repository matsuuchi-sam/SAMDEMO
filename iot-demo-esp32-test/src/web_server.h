#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

#define WIFI_AP_SSID "SAMDEMO-ESP32"
#define WIFI_AP_PASS ""

class WebDashboard {
public:
    void begin();
    void broadcast(const char *json);
    bool hasCommand(char *buf, size_t bufSize);
private:
    AsyncWebServer server_{80};
    AsyncWebSocket ws_{"/ws"};
    static void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                          AwsEventType type, void *arg, uint8_t *data, size_t len);
    static char cmdBuf_[256];
    static volatile bool cmdReady_;
};

#endif
