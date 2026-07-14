// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Matt Disher
#include "WiFiProvisioning.h"
#include "SerialHelper.h"

WiFiProvisioning::WiFiProvisioning()
    : settings(nullptr), connecting(false), timedOut(false),
      portalActive(false), connectStartMs(0), lastReconnectMs(0) {}

void WiFiProvisioning::begin(Settings *s)
{
    settings = s;
    WiFi.persistent(false); // we manage creds in our own NVS blob
    WiFi.setSleep(false);   // keep the web UI responsive
}

bool WiFiProvisioning::startSTA()
{
    if (!settings || !settings->hasWiFiCreds())
        return false;

    portalActive = false;
    dns.stop();
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(settings->data.hostname);
    WiFi.begin(settings->data.ssid, settings->data.pass);

    connecting = true;
    timedOut = false;
    connectStartMs = millis();
    SERIAL_PRINTF("[WiFi] connecting to \"%s\"...\n", settings->data.ssid);
    return true;
}

void WiFiProvisioning::startPortal()
{
    connecting = false;
    timedOut = false;

    // AP_STA lets the setup page scan for networks while the AP is up.
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_SSID, strlen(AP_PASSWORD) ? AP_PASSWORD : nullptr);
    softApIP = WiFi.softAPIP();

    dns.setErrorReplyCode(DNSReplyCode::NoError);
    dns.start(DNS_PORT, "*", softApIP); // catch-all -> our IP

    portalActive = true;
    SERIAL_PRINTF("[WiFi] captive portal up: SSID \"%s\", http://%s\n",
                  AP_SSID, softApIP.toString().c_str());
}

bool WiFiProvisioning::isConnected() const
{
    return WiFi.status() == WL_CONNECTED;
}

void WiFiProvisioning::update(uint32_t now)
{
    if (portalActive)
    {
        dns.processNextRequest();
        return;
    }

    if (connecting)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            connecting = false;
            timedOut = false;
            SERIAL_PRINTF("[WiFi] connected, IP %s\n",
                          WiFi.localIP().toString().c_str());
        }
        else if (now - connectStartMs >= WIFI_CONNECT_TIMEOUT_MS)
        {
            connecting = false;
            timedOut = true;
            SERIAL_PRINTLN(F("[WiFi] connect timed out"));
        }
        return;
    }

    // In RUN: quietly reconnect in the background if the link drops. The clock
    // keeps ticking on the RTC regardless.
    if (WiFi.status() != WL_CONNECTED && now - lastReconnectMs >= WIFI_RETRY_INTERVAL_MS)
    {
        lastReconnectMs = now;
        SERIAL_PRINTLN(F("[WiFi] link down, attempting reconnect"));
        WiFi.reconnect();
    }
}
