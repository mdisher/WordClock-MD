// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Matt Disher
#ifndef WIFI_PROVISIONING_H
#define WIFI_PROVISIONING_H

#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include "Settings.h"

// =====================================================================
//  WiFiProvisioning
//  Non-blocking STA connection with a timeout, falling back to a SoftAP
//  captive portal (AP_STA so the setup page can still scan for networks).
//  Owns the DNSServer catch-all that makes phones auto-open the portal.
//  The web UI lives in WebPortal; this class only manages the radio + DNS.
// =====================================================================
class WiFiProvisioning
{
public:
    WiFiProvisioning();

    void begin(Settings *settings);

    bool startSTA();    // begin an STA connect attempt; false if no creds
    void startPortal(); // bring up AP + captive DNS
    void update(uint32_t now);

    bool isConnected() const;
    bool isConnecting() const { return connecting; }
    bool connectTimedOut() const { return timedOut; }
    bool isPortalActive() const { return portalActive; }

    IPAddress localIP() const { return WiFi.localIP(); }
    IPAddress apIP() const { return softApIP; }
    String connectedSSID() const { return WiFi.SSID(); }
    int rssi() const { return WiFi.RSSI(); }

private:
    Settings *settings;
    DNSServer dns;
    bool connecting;
    bool timedOut;
    bool portalActive;
    uint32_t connectStartMs;
    uint32_t lastReconnectMs;
    IPAddress softApIP;
};

#endif // WIFI_PROVISIONING_H
