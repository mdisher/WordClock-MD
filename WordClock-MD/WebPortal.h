#ifndef WEB_PORTAL_H
#define WEB_PORTAL_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "Settings.h"
#include "WiFiProvisioning.h"
#include "TimeManager.h"
#include "ClockDisplayHAL.h"
#include "Animator.h"

// =====================================================================
//  WebPortal
//  One AsyncWebServer serving both the captive-portal Wi-Fi setup and the
//  full settings dashboard (same self-contained mobile-first page). Also
//  hosts the JSON API, a diagnostic/test mode for verifying the word map,
//  and OTA firmware upload (via the core Update library).
// =====================================================================
class WebPortal
{
public:
    WebPortal(Settings *settings, WiFiProvisioning *wifi, TimeManager *time,
              ClockDisplayHAL *hal, Animator *animator);

    void begin();
    void loop(uint32_t now); // handles deferred restart timing

    // The .ino polls these to coordinate with the render loop / reboot.
    bool consumeRestartRequest(uint32_t now);
    bool isTestActive() const { return testMode != TEST_OFF; }
    void renderTest(uint32_t now); // draws the current test frame + show()
    bool otaInProgress() const { return otaActive; }

private:
    Settings *settings;
    WiFiProvisioning *wifi;
    TimeManager *time;
    ClockDisplayHAL *hal;
    Animator *animator;

    AsyncWebServer server;

    // deferred restart (lets an HTTP response flush before ESP.restart())
    bool restartPending;
    uint32_t restartAtMs;
    void scheduleRestart(uint32_t delayMs);

    // OTA
    bool otaActive;

    // diagnostic/test mode
    enum TestMode : uint8_t
    {
        TEST_OFF = 0,
        TEST_SINGLE, // show one word (testIndex)
        TEST_CYCLE,  // step through words automatically
        TEST_ALL,    // all words on
    };
    TestMode testMode;
    uint16_t testIndex;
    uint32_t testLastStepMs;

    // route setup
    void setupRoutes();
    void setupCaptiveDetection();

    // handlers / helpers
    void handleStatus(AsyncWebServerRequest *req);
    void handleGetSettings(AsyncWebServerRequest *req);
    void handleScan(AsyncWebServerRequest *req);
    void applySettingsJson(const JsonObject &o);
};

#endif // WEB_PORTAL_H
