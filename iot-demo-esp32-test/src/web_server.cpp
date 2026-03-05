#include "web_server.h"

char WebDashboard::cmdBuf_[256] = {0};
volatile bool WebDashboard::cmdReady_ = false;

void WebDashboard::begin() {
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);
    Serial.print("[WiFi] AP started: ");
    Serial.println(WiFi.softAPIP());

    if (!SPIFFS.begin(true)) {
        Serial.println("[SPIFFS] Mount failed");
    }

    ws_.onEvent(onWsEvent);
    server_.addHandler(&ws_);

    server_.on("/", HTTP_GET, [](AsyncWebServerRequest *req) {
        req->send(SPIFFS, "/index.html", "text/html");
    });
    server_.serveStatic("/", SPIFFS, "/");

    server_.begin();
    Serial.println("[HTTP] Server started on port 80");
}

void WebDashboard::broadcast(const char *json) {
    ws_.textAll(json);
}

bool WebDashboard::hasCommand(char *buf, size_t bufSize) {
    if (!cmdReady_) return false;
    size_t len = strlen(cmdBuf_);
    size_t copyLen = len < bufSize - 1 ? len : bufSize - 1;
    memcpy(buf, cmdBuf_, copyLen);
    buf[copyLen] = '\0';
    cmdReady_ = false;
    return true;
}

void WebDashboard::onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                              AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("[WS] Client #%u connected\n", client->id());
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("[WS] Client #%u disconnected\n", client->id());
    } else if (type == WS_EVT_DATA) {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;
        if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
            if (len < sizeof(cmdBuf_) - 1 && !cmdReady_) {
                memcpy(cmdBuf_, data, len);
                cmdBuf_[len] = '\0';
                cmdReady_ = true;
            }
        }
    }
}
