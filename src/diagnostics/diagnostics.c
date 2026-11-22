#include "../../include/luna/diagnostics.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* xstrdup(const char* s) {
    if (!s) return NULL;
    size_t n = strlen(s);
    char* p = (char*)malloc(n + 1);
    if (p) memcpy(p, s, n + 1);
    return p;
}

static char* xvasprintf(const char* fmt, va_list ap) {
    va_list cp;
    va_copy(cp, ap);
    int n = vsnprintf(NULL, 0, fmt, cp);
    va_end(cp);
    if (n < 0) return NULL;
    char* buf = (char*)malloc((size_t)n + 1);
    if (!buf) return NULL;
    vsnprintf(buf, (size_t)n + 1, fmt, ap);
    return buf;
}

static void diag_free_owned(LunaDiagnostic* d) {
    free((void*)d->code);
    free((void*)d->message);
    free((void*)d->label);
    free((void*)d->help);
    for (int i = 0; i < d->note_count; i++) free((void*)d->notes[i].text);
    memset(d, 0, sizeof(*d));
}

void luna_diag_sink_init(LunaDiagnosticSink* sink, int max_errors) {
    memset(sink, 0, sizeof(*sink));
    sink->max_errors = max_errors > 0 ? max_errors : 20;
}

void luna_diag_sink_free(LunaDiagnosticSink* sink) {
    for (int i = 0; i < sink->count; i++) diag_free_owned(&sink->items[i]);
    free(sink->items);
    memset(sink, 0, sizeof(*sink));
}

bool luna_diag_has_errors(const LunaDiagnosticSink* sink) {
    return sink->error_count > 0;
}

void luna_diag_emit(LunaDiagnosticSink* sink, LunaDiagnostic diag) {
    if (sink->oom) {
        diag_free_owned(&diag);
        return;
    }
    if (diag.severity == LUNA_DIAG_ERROR && sink->error_count >= sink->max_errors) {
        diag_free_owned(&diag);
        return;
    }

    if (sink->count == sink->capacity) {
        int nc = sink->capacity ? sink->capacity * 2 : 8;
        LunaDiagnostic* ni = (LunaDiagnostic*)realloc(sink->items, (size_t)nc * sizeof(*ni));
        if (!ni) {
            sink->oom = true;
            diag_free_owned(&diag);
            return;
        }
        sink->items = ni;
        sink->capacity = nc;
    }

    sink->items[sink->count++] = diag;
    if (diag.severity == LUNA_DIAG_ERROR) sink->error_count++;
}

LunaDiagnostic luna_diag_make(LunaDiagSeverity severity, const char* code,
                              LunaSpan span, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char* msg = xvasprintf(fmt, ap);
    va_end(ap);

    LunaDiagnostic d;
    memset(&d, 0, sizeof(d));
    d.severity = severity;
    d.code = xstrdup(code);
    d.message = msg ? msg : xstrdup("<out of memory>");
    d.primary = span;
    return d;
}

void luna_diag_add_note(LunaDiagnostic* diag, LunaSpan span, const char* fmt, ...) {
    if (diag->note_count >= 8) return;

    va_list ap;
    va_start(ap, fmt);
    char* msg = xvasprintf(fmt, ap);
    va_end(ap);

    int i = diag->note_count++;
    diag->notes[i].text = msg ? msg : xstrdup("<out of memory>");
    diag->notes[i].span = span;
}

void luna_diag_set_help(LunaDiagnostic* diag, const char* fmt, ...) {
    free((void*)diag->help);

    va_list ap;
    va_start(ap, fmt);
    diag->help = xvasprintf(fmt, ap);
    va_end(ap);
}

typedef struct {
    int line;
    int col;
} LineCol;

static LineCol offset_to_linecol(const char* src, size_t len, uint32_t off) {
    LineCol lc = { 1, 1 };
    if (off > len) off = (uint32_t)len;
    for (uint32_t i = 0; i < off; i++) {
        if (src[i] == '\n') {
            lc.line++;
            lc.col = 1;
        } else if ((src[i] & 0xC0) != 0x80) {
            lc.col++;
        }
    }
    return lc;
}

static void line_bounds(const char* src, size_t len, uint32_t off,
                        uint32_t* out_start, uint32_t* out_end) {
    if (off > len) off = (uint32_t)len;
    uint32_t start = off;
    while (start > 0 && src[start - 1] != '\n') start--;
    uint32_t end = off;
    while (end < len && src[end] != '\n' && src[end] != '\r') end++;
    *out_start = start;
    *out_end = end;
}

static const char* severity_name(LunaDiagSeverity severity) {
    switch (severity) {
        case LUNA_DIAG_ERROR: return "error";
        case LUNA_DIAG_WARNING: return "warning";
        case LUNA_DIAG_NOTE: return "note";
        case LUNA_DIAG_HINT: return "hint";
    }
    return "?";
}

static const char* severity_color(LunaDiagSeverity severity) {
    switch (severity) {
        case LUNA_DIAG_ERROR: return "\x1b[1;31m";
        case LUNA_DIAG_WARNING: return "\x1b[1;33m";
        case LUNA_DIAG_NOTE: return "\x1b[1;34m";
        case LUNA_DIAG_HINT: return "\x1b[1;36m";
    }
    return "";
}

static void print_snippet(FILE* out, const char* src, size_t len,
                          LunaSpan span, const char* label, bool color) {
    uint32_t ls, le;
    line_bounds(src, len, span.start, &ls, &le);
    LineCol lc = offset_to_linecol(src, len, span.start);

    int width = 1;
    for (int t = lc.line; t >= 10; t /= 10) width++;

    fprintf(out, "%*d | ", width, lc.line);
    fwrite(src + ls, 1, le - ls, out);
    fputc('\n', out);

    fprintf(out, "%*s | ", width, "");
    for (uint32_t i = ls; i < span.start && i < le; i++) {
        fputc(src[i] == '\t' ? '\t' : ' ', out);
    }

    uint32_t carets = span.end > span.start ? span.end - span.start : 1;
    if (span.start + carets > le) carets = le > span.start ? le - span.start : 1;
    if (color) fputs("\x1b[1;31m", out);
    if (carets == 0) carets = 1;
    for (uint32_t i = 0; i < carets; i++) fputc('^', out);
    if (label && *label) fprintf(out, " %s", label);
    if (color) fputs("\x1b[0m", out);
    fputc('\n', out);
}

void luna_diag_render(const LunaDiagnostic* diag, const char* source,
                      size_t source_len, const char* filename,
                      void* out_stream, bool use_color) {
    FILE* out = (FILE*)out_stream;
    if (!out) return;

    if (use_color) fputs(severity_color(diag->severity), out);
    fputs(severity_name(diag->severity), out);
    if (use_color) fputs("\x1b[0m", out);
    if (diag->code) fprintf(out, "[%s]", diag->code);
    fprintf(out, ": %s\n", diag->message ? diag->message : "");

    LineCol lc = offset_to_linecol(source, source_len, diag->primary.start);
    fprintf(out, "  --> %s:%d:%d\n", filename ? filename : "<string>", lc.line, lc.col);
    print_snippet(out, source, source_len, diag->primary, diag->label, use_color);

    for (int i = 0; i < diag->note_count; i++) {
        if (diag->notes[i].span.start || diag->notes[i].span.end) {
            print_snippet(out, source, source_len, diag->notes[i].span,
                          diag->notes[i].text, use_color);
        } else {
            fprintf(out, "   = note: %s\n", diag->notes[i].text);
        }
    }
    if (diag->help) fprintf(out, "   = help: %s\n", diag->help);
}

int luna_diag_render_all(const LunaDiagnosticSink* sink, const char* source,
                         size_t source_len, const char* filename,
                         void* out_stream, bool use_color) {
    for (int i = 0; i < sink->count; i++) {
        luna_diag_render(&sink->items[i], source, source_len, filename,
                         out_stream, use_color);
    }
    if (sink->error_count >= sink->max_errors && sink->max_errors > 0) {
        fprintf((FILE*)out_stream,
                "   = note: se muestran solo los primeros %d errores\n",
                sink->max_errors);
    }
    return sink->error_count;
}
