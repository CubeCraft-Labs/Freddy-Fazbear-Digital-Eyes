#pragma once

// Generated locally by firmware/tools/configure_ota.py; never commit credentials.
#if __has_include("../private/OtaSecrets.h")
#include "../private/OtaSecrets.h"
#else
#define FREDDY_WIFI_SSID ""
#define FREDDY_WIFI_PASSWORD ""
#define FREDDY_OTA_KEY ""
#endif
