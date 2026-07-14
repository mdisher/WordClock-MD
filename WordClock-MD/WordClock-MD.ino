// =====================================================================
//  WordClock-MD  -- modernized ESP32-S3 word clock
//  Board: ESP32-S3 SuperMini (HW-747 v0.0.2), Arduino IDE
//
//  Non-blocking (millis-driven) throughout. Captive-portal Wi-Fi setup,
//  mobile web dashboard + OTA, POSIX-TZ NTP with automatic DST and daily
//  resync, three color modes, on-the-hour animations, random-letter status
//  indicator, and a firmware current clamp that keeps the strip within a
//  configurable milliamp budget so it's safe on the 5V/3A supply.
//
//  See README.md for board settings, libraries, and wiring.
// =====================================================================
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <esp_random.h>

#include "Config.h"
#include "SerialHelper.h"
#include "Settings.h"
#include "ClockDisplayHAL.h"
#include "ColorEngine.h"
#include "TimeManager.h"
#include "WiFiProvisioning.h"
#include "Animator.h"
#include "StatusIndicator.h"
#include "WordClock.h"
#include "WebPortal.h"

// ---- render cadence (~30 fps; each renderer also self-throttles) ----
static const uint32_t FRAME_INTERVAL_MS = 33;

// ---- module instances ----
Settings settings;
ClockDisplayHAL display(DATA_PIN, DEFAULT_BRIGHTNESS, DEFAULT_LED_BUDGET_MA);
ColorEngine colorEngine(&settings);
TimeManager timeManager;
WiFiProvisioning wifi;
Animator animator(&display);
StatusIndicator statusLed(&display);
WordClock wordClock(&display, &colorEngine, &timeManager);
WebPortal web(&settings, &wifi, &timeManager, &display, &animator);

enum AppState
{
    ST_BOOT,
    ST_CONNECTING,
    ST_PORTAL,
    ST_RUN,
};
static AppState appState = ST_BOOT;
static uint32_t lastFrameMs = 0;

static void onConnected(uint32_t now)
{
    SERIAL_PRINTF("[App] connected as %s (%s)\n",
                  settings.data.hostname, wifi.localIP().toString().c_str());

    timeManager.begin(settings.data.tz);
    timeManager.forceResync();

    if (MDNS.begin(settings.data.hostname))
    {
        MDNS.addService("http", "tcp", WEB_PORT);
        SERIAL_PRINTF("[App] mDNS: http://%s.local\n", settings.data.hostname);
    }

    statusLed.setState(STATUS_NTP_WAIT); // until first valid time
    appState = ST_RUN;
}

// Decide what occupies the LED grid this frame, in priority order.
static void renderTick(uint32_t now)
{
    if (now - lastFrameMs < FRAME_INTERVAL_MS)
        return;
    lastFrameMs = now;

    // 1) OTA in progress -> status owns the display (white chase).
    if (web.otaInProgress())
    {
        statusLed.setState(STATUS_OTA);
        statusLed.renderGrid(now);
        return;
    }

    // 2) Diagnostic/test mode from the dashboard.
    if (web.isTestActive())
    {
        web.renderTest(now);
        return;
    }

    // 3) On-the-hour animation.
    if (animator.isActive())
    {
        animator.update(now);
        return;
    }

    // 4) Boot / portal / connecting / waiting-for-NTP: status animation.
    if (statusLed.ownsDisplay())
    {
        statusLed.renderGrid(now);
        return;
    }

    // 5) Normal clock (RUN + valid time).
    if (appState == ST_RUN && timeManager.isTimeValid())
    {
        wordClock.compose(now);

        if (settings.data.animationEnabled && wordClock.consumeHourTrigger())
        {
            animator.start(settings.data.animationId, now);
            return; // animator takes over next frame
        }

        if (statusLed.hasOverlay())
            statusLed.applyOverlay(now);

        display.show();
    }
}

void setup()
{
    initSerial();
    SERIAL_PRINTLN(F("[App] WordClock-MD booting"));

    randomSeed(esp_random()); // decorrelate animation/status randomness per boot

    settings.begin();

    display.setup();
    display.setBrightness(settings.data.brightness);
    display.setBudgetMa(settings.data.ledBudgetMa);

    statusLed.setup();
    statusLed.setState(STATUS_BOOT);

    wifi.begin(&settings);

    // Bring the radio + TCP/IP (lwIP) stack up BEFORE starting the async web
    // server. AsyncServer::begin() calls into lwIP and asserts on a null mutex
    // if the stack hasn't been initialized yet (which WiFi.mode() does).
    if (settings.hasWiFiCreds() && wifi.startSTA())
    {
        appState = ST_CONNECTING;
        statusLed.setState(STATUS_CONNECTING);
    }
    else
    {
        wifi.startPortal();
        appState = ST_PORTAL;
        statusLed.setState(STATUS_PORTAL);
    }

    web.begin();
    wordClock.begin();
}

void loop()
{
    uint32_t now = millis();

    wifi.update(now);
    web.loop(now);
    statusLed.update(now); // onboard GPIO48 LED (self-throttled)

    // Deferred reboot requested by the web UI (creds saved / reset / OTA).
    if (web.consumeRestartRequest(now))
    {
        SERIAL_PRINTLN(F("[App] restarting..."));
        delay(60); // let the socket flush; not in a hot path
        ESP.restart();
    }

    switch (appState)
    {
    case ST_CONNECTING:
        if (wifi.isConnected())
        {
            onConnected(now);
        }
        else if (wifi.connectTimedOut())
        {
            SERIAL_PRINTLN(F("[App] connect failed -> captive portal"));
            wifi.startPortal();
            appState = ST_PORTAL;
            statusLed.setState(STATUS_PORTAL);
        }
        break;

    case ST_PORTAL:
        // Stay in the portal until the user submits creds (web schedules a reboot).
        break;

    case ST_RUN:
        timeManager.update(now);
        if (!wifi.isConnected())
            statusLed.setState(STATUS_WIFI_LOST);
        else if (!timeManager.isTimeValid())
            statusLed.setState(STATUS_NTP_WAIT);
        else
            statusLed.setState(STATUS_RUN_OK);
        break;

    case ST_BOOT:
    default:
        break;
    }

    renderTick(now);
}
