#ifndef LUNA_DIAGNOSTICS_H
#define LUNA_DIAGNOSTICS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t start;
    uint32_t end;
} LunaSpan;

typedef enum {
    LUNA_DIAG_ERROR,
    LUNA_DIAG_WARNING,
    LUNA_DIAG_NOTE,
    LUNA_DIAG_HINT,
} LunaDiagSeverity;

typedef enum {
    LUNA_ERROR_SYNTAX,
    LUNA_ERROR_INDENTATION,
    LUNA_ERROR_NAME,
    LUNA_ERROR_TYPE,
    LUNA_ERROR_VALUE,
    LUNA_ERROR_RUNTIME,
    LUNA_ERROR_ZERO_DIVISION,
    LUNA_ERROR_INDEX,
    LUNA_ERROR_KEY,
    LUNA_ERROR_ATTRIBUTE,
    LUNA_ERROR_IMPORT,
} LunaErrorType;

typedef struct {
    LunaDiagSeverity severity;
    LunaErrorType type;
    const char* message;
    LunaSpan primary;
    const char* label;

    int note_count;
    struct {
        const char* text;
        LunaSpan span;
    } notes[8];

    const char* help;
} LunaDiagnostic;

typedef struct {
    LunaDiagnostic* items;
    int count;
    int capacity;
    int error_count;
    int max_errors;
    bool oom;
} LunaDiagnosticSink;

#define LUNA_SPAN_NONE ((LunaSpan){ 0, 0 })

void luna_diag_set_color(bool enabled);
bool luna_diag_color_enabled(void);
void luna_diag_set_max_errors(int max_errors);
int luna_diag_max_errors(void);

void luna_diag_sink_init(LunaDiagnosticSink* sink, int max_errors);
void luna_diag_sink_free(LunaDiagnosticSink* sink);
void luna_diag_emit(LunaDiagnosticSink* sink, LunaDiagnostic diag);
bool luna_diag_has_errors(const LunaDiagnosticSink* sink);

LunaDiagnostic luna_diag_make(LunaDiagSeverity severity, LunaErrorType type,
                              LunaSpan span, const char* fmt, ...);
void luna_diag_add_note(LunaDiagnostic* diag, LunaSpan span, const char* fmt, ...);
void luna_diag_set_help(LunaDiagnostic* diag, const char* fmt, ...);

void luna_diag_render(const LunaDiagnostic* diag, const char* source,
                      size_t source_len, const char* filename,
                      void* out_stream, bool use_color);
int luna_diag_render_all(const LunaDiagnosticSink* sink, const char* source,
                         size_t source_len, const char* filename,
                         void* out_stream, bool use_color);

#endif
