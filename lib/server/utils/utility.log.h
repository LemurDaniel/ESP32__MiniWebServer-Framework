// ESP32 MiniWebServer Framework - https://github.com/LemurDaniel/ESP32__MiniWebServer-Framework
// Copyright © 2026, Daniel Landau
// MIT License

// NOTE:
//   Compile-time log levels for the framework, every line carries a [LEVEL] and a [Component] prefix.
//
//   Set the level in platformio.ini (default INFO):
//     build_flags = -D ESPWEB_LOG_LEVEL=ESPWEB_LOG_LEVEL_DEBUG
//
//   ESPWEB_LOG_INFO("MiniServer", "Listening on port %d", port);
//   => [INFO ] [MiniServer] Listening on port 80

//
//   Chapters:
//     LOG LEVELS   The numeric levels and the active default.
//     LOG MACROS   One macro per level, removed by the compiler below the active level.
//

#pragma once

#include <Arduino.h>

// ===========================================================================
// =============================   LOG LEVELS   ==============================
// ===========================================================================
//
//   ESPWEB_LOG_LEVEL_NONE    Disables every log line.
//   ESPWEB_LOG_LEVEL_ERROR   Failures that break a request or a subsystem.
//   ESPWEB_LOG_LEVEL_WARN    Unexpected states the framework recovers from.
//   ESPWEB_LOG_LEVEL_INFO    Lifecycle events (startup, WiFi, registrations).
//   ESPWEB_LOG_LEVEL_DEBUG   Per-request details.
//

#define ESPWEB_LOG_LEVEL_NONE 0
#define ESPWEB_LOG_LEVEL_ERROR 1
#define ESPWEB_LOG_LEVEL_WARN 2
#define ESPWEB_LOG_LEVEL_INFO 3
#define ESPWEB_LOG_LEVEL_DEBUG 4

#ifndef ESPWEB_LOG_LEVEL
#define ESPWEB_LOG_LEVEL ESPWEB_LOG_LEVEL_INFO
#endif

// ===========================================================================
// =============================   LOG MACROS   ==============================
// ===========================================================================
//
//   ESPWEB_LOG         Writes one prefixed line when the level is active.
//   ESPWEB_LOG_ERROR   Logs at ERROR, component and format must be string literals.
//   ESPWEB_LOG_WARN    Logs at WARN.
//   ESPWEB_LOG_INFO    Logs at INFO.
//   ESPWEB_LOG_DEBUG   Logs at DEBUG.
//
// The level check is a compile-time constant: inactive lines are removed by the compiler,
// but their format strings and arguments are still type-checked.

#define ESPWEB_LOG(level, label, component, format, ...)                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        if (ESPWEB_LOG_LEVEL >= level)                                                                                 \
        {                                                                                                              \
            Serial.printf("[" label "] [" component "] " format "\n", ##__VA_ARGS__);                                  \
        }                                                                                                              \
    } while (0)

#define ESPWEB_LOG_ERROR(component, format, ...)                                                                       \
    ESPWEB_LOG(ESPWEB_LOG_LEVEL_ERROR, "ERROR", component, format, ##__VA_ARGS__)
#define ESPWEB_LOG_WARN(component, format, ...)                                                                        \
    ESPWEB_LOG(ESPWEB_LOG_LEVEL_WARN, "WARN ", component, format, ##__VA_ARGS__)
#define ESPWEB_LOG_INFO(component, format, ...)                                                                        \
    ESPWEB_LOG(ESPWEB_LOG_LEVEL_INFO, "INFO ", component, format, ##__VA_ARGS__)
#define ESPWEB_LOG_DEBUG(component, format, ...)                                                                       \
    ESPWEB_LOG(ESPWEB_LOG_LEVEL_DEBUG, "DEBUG", component, format, ##__VA_ARGS__)
