#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include "platform.h"
#include "platform_internal.h"

#ifndef _WIN32
/* ---------- POSIX ---------- */

#include <time.h>
#include <unistd.h>
#include <errno.h>

uint64_t luna_platform_monotonic_ns(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

uint64_t luna_platform_wall_ns(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        return 0;
    }
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

void luna_platform_sleep_ns(uint64_t ns) {
    struct timespec req;
    req.tv_sec = (time_t)(ns / 1000000000ULL);
    req.tv_nsec = (long)(ns % 1000000000ULL);

    /* nanosleep puede ser interrumpido; reintentar */
    while (nanosleep(&req, &req) == -1 && errno == EINTR) {
        /* continuar durmiendo el tiempo restante */
    }
}

#else
/* ---------- Windows ---------- */

#include <windows.h>

static uint64_t perf_frequency = 0;

static void init_perf_frequency(void) {
    if (perf_frequency == 0) {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        perf_frequency = (uint64_t)freq.QuadPart;
    }
}

uint64_t luna_platform_monotonic_ns(void) {
    init_perf_frequency();
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    uint64_t c = (uint64_t)counter.QuadPart;
    /* Convertir a nanosegundos: c * 1e9 / freq */
    return (c / perf_frequency) * 1000000000ULL +
           ((c % perf_frequency) * 1000000000ULL) / perf_frequency;
}

uint64_t luna_platform_wall_ns(void) {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    uint64_t t = ((uint64_t)ft.dwHighDateTime << 32) | (uint64_t)ft.dwLowDateTime;
    /* FILETIME: 100-nanosecond intervals since 1601-01-01.
     * Unix epoch: 1970-01-01. Diferencia: 11644473600 segundos. */
    t -= 116444736000000000ULL;
    return t * 100ULL;  /* convertir de 100ns a 1ns */
}

void luna_platform_sleep_ns(uint64_t ns) {
    /* Sleep() tiene resolución de ~1ms; para sleeps cortos usar
     * waitable timers si se necesita precisión. */
    DWORD ms = (DWORD)(ns / 1000000ULL);
    if (ms == 0 && ns > 0) ms = 1;  /* al menos 1ms */
    Sleep(ms);
}

#endif
