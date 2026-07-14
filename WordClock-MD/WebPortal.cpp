// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Matt Disher
#include "WebPortal.h"
#include "WebPage.h"
#include "SerialHelper.h"
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <Update.h>

static inline int clampi(int v, int lo, int hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

WebPortal::WebPortal(Settings *settings, WiFiProvisioning *wifi, TimeManager *time,
                     ClockDisplayHAL *hal, Animator *animator)
    : settings(settings), wifi(wifi), time(time), hal(hal), animator(animator),
      server(WEB_PORT), restartPending(false), restartAtMs(0), otaActive(false),
      testMode(TEST_OFF), testIndex(0), testLastStepMs(0) {}

void WebPortal::begin()
{
    setupRoutes();
    setupCaptiveDetection();
    server.begin();
    SERIAL_PRINTLN(F("[Web] server started"));
}

void WebPortal::loop(uint32_t now)
{
    (void)now; // restart timing handled in consumeRestartRequest(); OTA in handler
}

void WebPortal::scheduleRestart(uint32_t delayMs)
{
    restartPending = true;
    restartAtMs = millis() + delayMs;
}

bool WebPortal::consumeRestartRequest(uint32_t now)
{
    if (restartPending && (int32_t)(now - restartAtMs) >= 0)
    {
        restartPending = false;
        return true;
    }
    return false;
}

// ------------------------------------------------------------------ status
void WebPortal::handleStatus(AsyncWebServerRequest *req)
{
    AsyncResponseStream *res = req->beginResponseStream("application/json");
    JsonDocument d;
    bool conn = wifi->isConnected();
    d["connected"] = conn;
    d["portal"] = wifi->isPortalActive();
    d["ip"] = conn ? wifi->localIP().toString()
                   : (wifi->isPortalActive() ? wifi->apIP().toString() : String("0.0.0.0"));
    d["ssid"] = conn ? wifi->connectedSSID() : String("");
    d["rssi"] = conn ? wifi->rssi() : 0;

    bool tv = time->isTimeValid();
    d["timeValid"] = tv;
    struct tm t = time->getLocalTimeStruct();
    char buf[6];
    if (tv)
        snprintf(buf, sizeof(buf), "%02d:%02d", t.tm_hour, t.tm_min);
    else
        strcpy(buf, "--:--");
    d["time"] = buf;

    d["brightness"] = hal->getBrightness();
    d["budgetMa"] = hal->getBudgetMa();
    d["estMa"] = hal->estimateCurrentMa();
    d["effB"] = hal->lastEffectiveBrightness();
    d["heap"] = ESP.getFreeHeap();

    serializeJson(d, *res);
    req->send(res);
}

// ---------------------------------------------------------------- settings
void WebPortal::handleGetSettings(AsyncWebServerRequest *req)
{
    AsyncResponseStream *res = req->beginResponseStream("application/json");
    JsonDocument d;
    const SettingsData &s = settings->data;
    d["brightness"] = s.brightness;
    d["budgetMa"] = s.ledBudgetMa;
    d["budgetMax"] = MAX_LED_BUDGET_MA;
    d["colorMode"] = s.colorMode;
    d["singleColor"] = s.singleColor;
    d["gradientVariant"] = s.gradientVariant;
    d["gradientSpeed"] = s.gradientSpeed;
    d["animationEnabled"] = s.animationEnabled;
    d["animationId"] = s.animationId;
    d["tz"] = s.tz;
    d["hostname"] = s.hostname;
    d["ssid"] = s.ssid;

    JsonArray words = d["words"].to<JsonArray>();
    for (uint16_t i = 0; i < ClockDisplayHAL::wordCount(); i++)
        words.add(ClockDisplayHAL::wordName(i));

    serializeJson(d, *res);
    req->send(res);
}

void WebPortal::applySettingsJson(const JsonObject &o)
{
    SettingsData &s = settings->data;

    if (o["brightness"].is<int>())
    {
        s.brightness = (uint8_t)clampi(o["brightness"].as<int>(), 0, 255);
        hal->setBrightness(s.brightness);
    }
    if (o["budgetMa"].is<int>())
    {
        s.ledBudgetMa = (uint16_t)clampi(o["budgetMa"].as<int>(), 100, MAX_LED_BUDGET_MA);
        hal->setBudgetMa(s.ledBudgetMa);
    }
    if (o["colorMode"].is<int>())
        s.colorMode = (uint8_t)clampi(o["colorMode"].as<int>(), 0, 2);
    if (o["singleColor"].is<int>() || o["singleColor"].is<unsigned int>())
        s.singleColor = o["singleColor"].as<uint32_t>() & 0x00FFFFFF;
    if (o["gradientVariant"].is<int>())
        s.gradientVariant = (uint8_t)clampi(o["gradientVariant"].as<int>(), 0, 2);
    if (o["gradientSpeed"].is<int>())
        s.gradientSpeed = (uint8_t)clampi(o["gradientSpeed"].as<int>(), 0, 255);
    if (o["animationEnabled"].is<int>())
        s.animationEnabled = o["animationEnabled"].as<int>() ? 1 : 0;
    if (o["animationId"].is<int>())
        s.animationId = (uint8_t)o["animationId"].as<int>();

    if (o["tz"].is<const char *>())
    {
        const char *tz = o["tz"];
        if (tz && strncmp(tz, s.tz, sizeof(s.tz)) != 0)
        {
            strncpy(s.tz, tz, sizeof(s.tz) - 1);
            s.tz[sizeof(s.tz) - 1] = '\0';
            time->setTimezone(s.tz); // apply immediately (live TZ change)
        }
    }
    if (o["hostname"].is<const char *>())
    {
        const char *hn = o["hostname"];
        if (hn && hn[0])
        {
            strncpy(s.hostname, hn, sizeof(s.hostname) - 1);
            s.hostname[sizeof(s.hostname) - 1] = '\0';
        }
    }

    if (o["save"].is<int>() && o["save"].as<int>() != 0)
        settings->save();
}

// -------------------------------------------------------------------- scan
void WebPortal::handleScan(AsyncWebServerRequest *req)
{
    AsyncResponseStream *res = req->beginResponseStream("application/json");
    JsonDocument d;

    int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_RUNNING)
    {
        d["scanning"] = true;
    }
    else if (n == WIFI_SCAN_FAILED)
    {
        WiFi.scanNetworks(true); // start async scan
        d["scanning"] = true;
    }
    else
    {
        JsonArray arr = d["networks"].to<JsonArray>();
        for (int i = 0; i < n && i < 20; i++)
        {
            JsonObject net = arr.add<JsonObject>();
            net["ssid"] = WiFi.SSID(i);
            net["rssi"] = WiFi.RSSI(i);
            net["secure"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
        }
        WiFi.scanDelete();
        d["scanning"] = false;
    }
    serializeJson(d, *res);
    req->send(res);
}

// ------------------------------------------------------------------ routes
void WebPortal::setupRoutes()
{
    // Main page (serves both dashboard and captive-portal setup).
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *req) {
        req->send(200, "text/html", INDEX_HTML);
    });

    server.on("/api/status", HTTP_GET,
              [this](AsyncWebServerRequest *req) { handleStatus(req); });
    server.on("/api/settings", HTTP_GET,
              [this](AsyncWebServerRequest *req) { handleGetSettings(req); });
    server.on("/api/scan", HTTP_GET,
              [this](AsyncWebServerRequest *req) { handleScan(req); });

    // POST /api/settings (JSON body)
    auto *setH = new AsyncCallbackJsonWebHandler(
        "/api/settings", [this](AsyncWebServerRequest *req, JsonVariant &json) {
            applySettingsJson(json.as<JsonObject>());
            req->send(200, "application/json", "{\"ok\":true}");
        });
    server.addHandler(setH);

    // POST /api/wifi (JSON {ssid,pass}) -> save + reboot to connect
    auto *wifiH = new AsyncCallbackJsonWebHandler(
        "/api/wifi", [this](AsyncWebServerRequest *req, JsonVariant &json) {
            JsonObject o = json.as<JsonObject>();
            const char *ssid = o["ssid"] | "";
            const char *pass = o["pass"] | "";
            if (ssid[0])
            {
                settings->setWiFi(ssid, pass);
                settings->save();
                req->send(200, "application/json", "{\"ok\":true}");
                scheduleRestart(1200);
            }
            else
            {
                req->send(400, "application/json", "{\"ok\":false}");
            }
        });
    server.addHandler(wifiH);

    // POST /api/test (JSON {mode,index}) -> diagnostic word test
    auto *testH = new AsyncCallbackJsonWebHandler(
        "/api/test", [this](AsyncWebServerRequest *req, JsonVariant &json) {
            JsonObject o = json.as<JsonObject>();
            const char *mode = o["mode"] | "off";
            if (!strcmp(mode, "single"))
            {
                testMode = TEST_SINGLE;
                testIndex = (uint16_t)clampi(o["index"] | 0, 0, ClockDisplayHAL::wordCount() - 1);
            }
            else if (!strcmp(mode, "cycle"))
            {
                testMode = TEST_CYCLE;
                testIndex = 0;
                testLastStepMs = millis();
            }
            else if (!strcmp(mode, "all"))
            {
                testMode = TEST_ALL;
            }
            else
            {
                testMode = TEST_OFF;
            }
            req->send(200, "application/json", "{\"ok\":true}");
        });
    server.addHandler(testH);

    // POST /api/preview -> play the configured animation now
    server.on("/api/preview", HTTP_POST, [this](AsyncWebServerRequest *req) {
        animator->start(settings->data.animationId, millis());
        req->send(200, "application/json", "{\"ok\":true}");
    });

    // POST /api/reset -> factory reset + reboot
    server.on("/api/reset", HTTP_POST, [this](AsyncWebServerRequest *req) {
        settings->resetDefaults();
        settings->save();
        req->send(200, "application/json", "{\"ok\":true}");
        scheduleRestart(1000);
    });

    // POST /api/ota -> OTA firmware upload (core Update library).
    // AsyncWebServer takes TWO lambdas here: the first runs once the whole
    // request is received (send the result + schedule the reboot); the second
    // is the body handler, invoked repeatedly with chunks of the uploaded .bin
    // as they stream in (index==0 first chunk, final==true last) -- we feed
    // each chunk straight into the Update flash writer.
    server.on(
        "/api/ota", HTTP_POST,
        [this](AsyncWebServerRequest *req) {
            bool ok = !Update.hasError();
            AsyncWebServerResponse *r = req->beginResponse(
                ok ? 200 : 500, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false}");
            r->addHeader("Connection", "close");
            req->send(r);
            if (ok)
                scheduleRestart(1500);
        },
        [this](AsyncWebServerRequest *req, String filename, size_t index,
               uint8_t *data, size_t len, bool final) {
            if (index == 0)
            {
                otaActive = true;
                SERIAL_PRINTF("[OTA] start: %s\n", filename.c_str());
                if (!Update.begin(UPDATE_SIZE_UNKNOWN))
                    Update.printError(Serial);
            }
            if (len && Update.write(data, len) != len)
                Update.printError(Serial);
            if (final)
            {
                if (Update.end(true))
                    SERIAL_PRINTF("[OTA] success: %u bytes\n", (unsigned)(index + len));
                else
                    Update.printError(Serial);
                otaActive = false;
            }
        });
}

void WebPortal::setupCaptiveDetection()
{
    // Any unknown URL: in portal mode, bounce phones to our page so the OS
    // captive-portal popup opens. Otherwise serve the SPA (client routing).
    server.onNotFound([this](AsyncWebServerRequest *req) {
        if (wifi->isPortalActive())
        {
            String url = "http://" + wifi->apIP().toString() + "/";
            req->redirect(url);
        }
        else
        {
            req->send(200, "text/html", INDEX_HTML);
        }
    });
}

// ------------------------------------------------------------- test render
void WebPortal::renderTest(uint32_t now)
{
    if (testMode == TEST_OFF)
        return;

    hal->clear();
    const uint32_t col = 0x00FFFFFF; // white; clamp keeps current safe

    switch (testMode)
    {
    case TEST_ALL:
        hal->fill(col);
        break;
    case TEST_CYCLE:
        if (now - testLastStepMs >= 800)
        {
            testLastStepMs = now;
            testIndex = (testIndex + 1) % ClockDisplayHAL::wordCount();
        }
        hal->displayWord(ClockDisplayHAL::wordName(testIndex), col);
        break;
    case TEST_SINGLE:
    default:
        hal->displayWord(ClockDisplayHAL::wordName(testIndex), col);
        break;
    }
    hal->show();
}
