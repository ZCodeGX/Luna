#ifndef _WIN32
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#endif

#include "stdlib_internal.h"

#ifndef _WIN32

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "../runtime/userdata.h"

static const char GML_PYTHON[] =
"import json\n"
"import os\n"
"import select\n"
"import sys\n"
"import tkinter as tk\n"
"\n"
"windows = {}\n"
"widgets = {}\n"
"root = None\n"
"buffer = \"\"\n"
"\n"
"\n"
"def emit(obj):\n"
"    sys.stdout.write(json.dumps(obj) + \"\\n\")\n"
"    sys.stdout.flush()\n"
"\n"
"\n"
"def text_of(widget, kind):\n"
"    if kind == \"text\":\n"
"        return widget.get(\"1.0\", \"end-1c\")\n"
"    if kind == \"entry\":\n"
"        return widget.get()\n"
"    return widget.cget(\"text\")\n"
"\n"
"\n"
"def on_close(wid):\n"
"    emit({\"type\": \"close\", \"window\": wid})\n"
"    win = windows.pop(wid, None)\n"
"    if win is not None:\n"
"        win.destroy()\n"
"    if not windows and root is not None:\n"
"        root.quit()\n"
"\n"
"\n"
"def handle(msg):\n"
"    global root\n"
"    cmd = msg.get(\"cmd\")\n"
"    if cmd == \"quit\":\n"
"        emit({\"type\": \"quit\"})\n"
"        if root is not None:\n"
"            root.quit()\n"
"        return False\n"
"    if cmd == \"window\":\n"
"        wid = int(msg[\"id\"])\n"
"        if root is None:\n"
"            root = tk.Tk()\n"
"            root.withdraw()\n"
"        if not windows:\n"
"            win = root\n"
"            root.deiconify()\n"
"        else:\n"
"            win = tk.Toplevel(root)\n"
"        win.title(msg.get(\"title\", \"Luna\"))\n"
"        width = int(msg.get(\"width\", 640))\n"
"        height = int(msg.get(\"height\", 480))\n"
"        win.geometry(str(width) + \"x\" + str(height))\n"
"        windows[wid] = win\n"
"        win.protocol(\"WM_DELETE_WINDOW\", lambda: on_close(wid))\n"
"        emit({\"type\": \"ready\", \"window\": wid})\n"
"        return True\n"
"    if cmd == \"widget\":\n"
"        wid = int(msg[\"id\"])\n"
"        parent = windows.get(int(msg[\"window\"]))\n"
"        if parent is None:\n"
"            return True\n"
"        kind = msg.get(\"kind\", \"label\")\n"
"        text = msg.get(\"text\", \"\")\n"
"        x = int(msg.get(\"x\", 0))\n"
"        y = int(msg.get(\"y\", 0))\n"
"        width = int(msg.get(\"width\", 120))\n"
"        height = int(msg.get(\"height\", 28))\n"
"        if kind == \"button\":\n"
"            widget = tk.Button(parent, text=text, command=lambda: emit({\"type\": \"click\", \"id\": wid}))\n"
"        elif kind == \"entry\":\n"
"            widget = tk.Entry(parent)\n"
"            widget.insert(0, text)\n"
"            widget.bind(\"<Return>\", lambda e: emit({\"type\": \"enter\", \"id\": wid, \"text\": widget.get()}))\n"
"        elif kind == \"text\":\n"
"            widget = tk.Text(parent)\n"
"            widget.insert(\"1.0\", text)\n"
"        elif kind == \"check\":\n"
"            widget = tk.Checkbutton(parent, text=text, command=lambda: emit({\"type\": \"toggle\", \"id\": wid, \"text\": text}))\n"
"        else:\n"
"            widget = tk.Label(parent, text=text)\n"
"        widget.place(x=x, y=y, width=width, height=height)\n"
"        widgets[wid] = (widget, kind)\n"
"        return True\n"
"    if cmd == \"set\":\n"
"        item = widgets.get(int(msg[\"id\"]))\n"
"        if item is not None:\n"
"            widget, kind = item\n"
"            value = msg.get(\"text\", \"\")\n"
"            if kind == \"text\":\n"
"                widget.delete(\"1.0\", \"end\")\n"
"                widget.insert(\"1.0\", value)\n"
"            elif kind == \"entry\":\n"
"                widget.delete(0, \"end\")\n"
"                widget.insert(0, value)\n"
"            else:\n"
"                widget.configure(text=value)\n"
"        return True\n"
"    if cmd == \"get\":\n"
"        item = widgets.get(int(msg[\"id\"]))\n"
"        value = \"\" if item is None else text_of(item[0], item[1])\n"
"        emit({\"type\": \"value\", \"id\": int(msg[\"id\"]), \"text\": value})\n"
"        return True\n"
"    return True\n"
"\n"
"\n"
"def poll():\n"
"    global root, buffer\n"
"    if root is None:\n"
"        return\n"
"    try:\n"
"        ready, _, _ = select.select([sys.stdin], [], [], 0)\n"
"        if ready:\n"
"            chunk = os.read(sys.stdin.fileno(), 4096)\n"
"            if not chunk:\n"
"                emit({\"type\": \"quit\"})\n"
"                root.quit()\n"
"                return\n"
"            buffer += chunk.decode(\"utf-8\")\n"
"            while \"\\n\" in buffer:\n"
"                line, buffer = buffer.split(\"\\n\", 1)\n"
"                if not line:\n"
"                    continue\n"
"                msg = json.loads(line)\n"
"                if not handle(msg):\n"
"                    return\n"
"    except Exception as exc:\n"
"        emit({\"type\": \"error\", \"message\": str(exc)})\n"
"    if windows:\n"
"        root.after(16, poll)\n"
"\n"
"\n"
"root = tk.Tk()\n"
"root.withdraw()\n"
"poll()\n"
"root.mainloop()\n"
;

typedef struct {
    char* data;
    size_t len;
    size_t cap;
    int failed;
} GmlBuf;

typedef struct {
    pid_t pid;
    int to_child;
    int from_child;
    int next_id;
    int closed;
} GmlApp;

static void buf_init(GmlBuf* b) {
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
    b->failed = 0;
}

static void buf_free(GmlBuf* b) {
    free(b->data);
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

static int buf_reserve(GmlBuf* b, size_t extra) {
    if (b->failed) return -1;
    if (b->len + extra + 1 <= b->cap) return 0;
    size_t cap = b->cap ? b->cap : 256;
    while (cap < b->len + extra + 1) {
        if (cap > (size_t)-1 / 2) { b->failed = 1; return -1; }
        cap *= 2;
    }
    char* data = (char*)realloc(b->data, cap);
    if (!data) { b->failed = 1; return -1; }
    b->data = data;
    b->cap = cap;
    return 0;
}

static int buf_append(GmlBuf* b, const char* s, size_t n) {
    if (buf_reserve(b, n) != 0) return -1;
    memcpy(b->data + b->len, s, n);
    b->len += n;
    b->data[b->len] = '\0';
    return 0;
}

static int buf_puts(GmlBuf* b, const char* s) {
    return buf_append(b, s, strlen(s));
}

static int buf_printf(GmlBuf* b, const char* fmt, ...) {
    char tmp[256];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);
    if (n < 0) { b->failed = 1; return -1; }
    if ((size_t)n < sizeof(tmp)) return buf_append(b, tmp, (size_t)n);
    char* dyn = (char*)malloc((size_t)n + 1);
    if (!dyn) { b->failed = 1; return -1; }
    va_start(ap, fmt);
    vsnprintf(dyn, (size_t)n + 1, fmt, ap);
    va_end(ap);
    int rc = buf_append(b, dyn, (size_t)n);
    free(dyn);
    return rc;
}

static int buf_json_string(GmlBuf* b, const char* s, size_t n) {
    if (buf_puts(b, "\"") != 0) return -1;
    for (size_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)s[i];
        switch (c) {
            case '"': if (buf_puts(b, "\\\"") != 0) return -1; break;
            case '\\': if (buf_puts(b, "\\\\") != 0) return -1; break;
            case '\n': if (buf_puts(b, "\\n") != 0) return -1; break;
            case '\r': if (buf_puts(b, "\\r") != 0) return -1; break;
            case '\t': if (buf_puts(b, "\\t") != 0) return -1; break;
            default:
                if (c < 0x20) {
                    if (buf_printf(b, "\\u%04x", c) != 0) return -1;
                } else if (buf_append(b, (const char*)&c, 1) != 0) return -1;
        }
    }
    return buf_puts(b, "\"");
}

static void app_shutdown(GmlApp* app) {
    if (!app || app->closed) return;
    app->closed = 1;
    if (app->to_child >= 0) {
        const char* quit = "{\"cmd\":\"quit\"}\n";
        size_t n = strlen(quit);
        size_t written = 0;
        while (written < n) {
            ssize_t r = write(app->to_child, quit + written, n - written);
            if (r <= 0) break;
            written += (size_t)r;
        }
        close(app->to_child);
        app->to_child = -1;
    }
    if (app->pid > 0) {
        int status = 0;
        for (int i = 0; i < 100; i++) {
            pid_t r = waitpid(app->pid, &status, WNOHANG);
            if (r == app->pid || (r < 0 && errno != EINTR)) break;
            struct timespec ts;
            ts.tv_sec = 0;
            ts.tv_nsec = 20000000L;
            nanosleep(&ts, NULL);
        }
        pid_t r = waitpid(app->pid, &status, WNOHANG);
        if (r == 0) {
            kill(app->pid, SIGTERM);
            waitpid(app->pid, &status, 0);
        }
        app->pid = -1;
    }
    if (app->from_child >= 0) {
        close(app->from_child);
        app->from_child = -1;
    }
}

static void app_finalizer(LunaState* L, void* ud) {
    (void)L;
    app_shutdown((GmlApp*)ud);
}

static GmlApp* check_app(LunaState* L) {
    LunaValue v = lib_arg(L, 0);
    if (v.type != LUNA_TUSERDATA) {
        lib_error(L, "gml: se esperaba una aplicación");
        return NULL;
    }
    GmlApp* app = (GmlApp*)luna_userdata_data(luna_val_userdata(v));
    if (!app || app->closed) {
        lib_error(L, "gml: aplicación cerrada");
        return NULL;
    }
    return app;
}

static int64_t arg_int(LunaState* L, int i, int64_t fallback) {
    LunaValue v = lib_arg(L, i);
    if (v.type == LUNA_TINTEGER) return luna_val_int(v);
    if (v.type == LUNA_TFLOAT) return (int64_t)luna_val_float(v);
    return fallback;
}

static char* arg_text(LunaState* L, int i, const char* fallback) {
    LunaValue v = lib_arg(L, i);
    if (v.type == LUNA_TSTRING) {
        LunaString* s = luna_val_string(v);
        char* out = (char*)malloc(s->len + 1);
        if (!out) return NULL;
        memcpy(out, s->data, s->len);
        out[s->len] = '\0';
        return out;
    }
    if (v.type == LUNA_TINTEGER) {
        char* out = (char*)malloc(64);
        if (out) snprintf(out, 64, "%lld", (long long)luna_val_int(v));
        return out;
    }
    if (v.type == LUNA_TFLOAT) {
        char* out = (char*)malloc(64);
        if (out) snprintf(out, 64, "%.17g", luna_val_float(v));
        return out;
    }
    if (v.type == LUNA_TBOOLEAN) return lib_strdup(luna_val_bool(v) ? "true" : "false");
    if (v.type == LUNA_TNIL) return lib_strdup("");
    return lib_strdup(fallback ? fallback : "");
}

static int send_command(GmlApp* app, GmlBuf* cmd) {
    if (!app || app->closed || cmd->failed || !cmd->data) {
        if (app && !app->closed) app->closed = 1;
        return -1;
    }
    if (buf_puts(cmd, "\n") != 0) return -1;
    size_t written = 0;
    while (written < cmd->len) {
        ssize_t r = write(app->to_child, cmd->data + written, cmd->len - written);
        if (r < 0) {
            if (errno == EINTR) continue;
            app_shutdown(app);
            return -1;
        }
        written += (size_t)r;
    }
    return 0;
}

static int h_app(LunaState* L) {
    int in_pipe[2];
    int out_pipe[2];
    if (pipe(in_pipe) != 0) return lib_error(L, "gml: pipe falló: %s", strerror(errno));
    if (pipe(out_pipe) != 0) {
        close(in_pipe[0]);
        close(in_pipe[1]);
        return lib_error(L, "gml: pipe falló: %s", strerror(errno));
    }
    signal(SIGPIPE, SIG_IGN);
    pid_t pid = fork();
    if (pid < 0) {
        close(in_pipe[0]); close(in_pipe[1]);
        close(out_pipe[0]); close(out_pipe[1]);
        return lib_error(L, "gml: fork falló: %s", strerror(errno));
    }
    if (pid == 0) {
        close(in_pipe[1]);
        close(out_pipe[0]);
        if (dup2(in_pipe[0], STDIN_FILENO) < 0) _exit(127);
        if (dup2(out_pipe[1], STDOUT_FILENO) < 0) _exit(127);
        close(in_pipe[0]);
        close(out_pipe[1]);
        execlp("python3", "python3", "-c", GML_PYTHON, (char*)NULL);
        _exit(127);
    }
    close(in_pipe[0]);
    close(out_pipe[1]);
    int flags = fcntl(out_pipe[0], F_GETFL, 0);
    if (flags >= 0) fcntl(out_pipe[0], F_SETFL, flags | O_NONBLOCK);

    GmlApp* app = (GmlApp*)luna_newuserdata_finalized(L, sizeof(GmlApp), app_finalizer);
    if (!app) {
        close(in_pipe[1]); close(out_pipe[0]);
        kill(pid, SIGTERM); waitpid(pid, NULL, 0);
        return -1;
    }
    app->pid = pid;
    app->to_child = in_pipe[1];
    app->from_child = out_pipe[0];
    app->next_id = 1;
    app->closed = 0;
    return 1;
}

static int new_widget(LunaState* L, const char* kind, int with_text) {
    GmlApp* app = check_app(L);
    if (!app) return -1;
    int64_t window = arg_int(L, 1, 0);
    char* text = with_text ? arg_text(L, 2, "") : lib_strdup("");
    int64_t x = arg_int(L, with_text ? 3 : 2, 0);
    int64_t y = arg_int(L, with_text ? 4 : 3, 0);
    int64_t w = arg_int(L, with_text ? 5 : 4, 120);
    int64_t h = arg_int(L, with_text ? 6 : 5, 28);
    if (!text) return lib_error(L, "gml: sin memoria");
    int id = app->next_id++;
    GmlBuf cmd;
    buf_init(&cmd);
    buf_printf(&cmd, "{\"cmd\":\"widget\",\"id\":%d,\"window\":%lld,\"kind\":", id, (long long)window);
    buf_json_string(&cmd, kind, strlen(kind));
    buf_puts(&cmd, ",\"text\":");
    buf_json_string(&cmd, text, strlen(text));
    buf_printf(&cmd, ",\"x\":%lld,\"y\":%lld,\"width\":%lld,\"height\":%lld}", (long long)x, (long long)y, (long long)w, (long long)h);
    free(text);
    int rc = send_command(app, &cmd);
    buf_free(&cmd);
    if (rc != 0) return lib_error(L, "gml: no se pudo crear el widget");
    luna_pushinteger(L, id);
    return 1;
}

static int h_window(LunaState* L) {
    GmlApp* app = check_app(L);
    if (!app) return -1;
    char* title = arg_text(L, 1, "Luna");
    int64_t width = arg_int(L, 2, 640);
    int64_t height = arg_int(L, 3, 480);
    if (!title) return lib_error(L, "gml: sin memoria");
    int id = app->next_id++;
    GmlBuf cmd;
    buf_init(&cmd);
    buf_printf(&cmd, "{\"cmd\":\"window\",\"id\":%d,\"title\":", id);
    buf_json_string(&cmd, title, strlen(title));
    buf_printf(&cmd, ",\"width\":%lld,\"height\":%lld}", (long long)width, (long long)height);
    free(title);
    int rc = send_command(app, &cmd);
    buf_free(&cmd);
    if (rc != 0) return lib_error(L, "gml: no se pudo crear la ventana");
    luna_pushinteger(L, id);
    return 1;
}

static int h_button(LunaState* L) { return new_widget(L, "button", 1); }
static int h_label(LunaState* L) { return new_widget(L, "label", 1); }
static int h_entry(LunaState* L) { return new_widget(L, "entry", 1); }
static int h_text(LunaState* L) { return new_widget(L, "text", 1); }
static int h_checkbox(LunaState* L) { return new_widget(L, "check", 1); }

static int h_set_text(LunaState* L) {
    GmlApp* app = check_app(L);
    if (!app) return -1;
    int64_t id = arg_int(L, 1, 0);
    char* text = arg_text(L, 2, "");
    if (!text) return lib_error(L, "gml: sin memoria");
    GmlBuf cmd;
    buf_init(&cmd);
    buf_printf(&cmd, "{\"cmd\":\"set\",\"id\":%lld,\"text\":", (long long)id);
    buf_json_string(&cmd, text, strlen(text));
    buf_puts(&cmd, "}");
    free(text);
    int rc = send_command(app, &cmd);
    buf_free(&cmd);
    if (rc != 0) return lib_error(L, "gml: no se pudo actualizar el widget");
    luna_pushboolean(L, 1);
    return 1;
}

static int read_line(GmlApp* app, double timeout, GmlBuf* line) {
    int ms = timeout < 0 ? -1 : (int)(timeout * 1000);
    for (;;) {
        struct pollfd pfd;
        pfd.fd = app->from_child;
        pfd.events = POLLIN;
        int ready = poll(&pfd, 1, ms);
        if (ready == 0) return 0;
        if (ready < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        char c;
        ssize_t n = read(app->from_child, &c, 1);
        if (n < 0) {
            if (errno == EINTR || errno == EAGAIN) continue;
            return -1;
        }
        if (n == 0) return -1;
        if (c == '\n') return 1;
        if (buf_append(line, &c, 1) != 0) return -1;
        if (line->len > 1024 * 1024) return -1;
    }
}

static int push_event(LunaState* L, const char* text, size_t len) {
    int base = luna_gettop(L);
    if (luna_json_parse_text(L, text, len) != 0) {
        luna_settop(L, base);
        return 0;
    }
    int src = luna_gettop(L);
    luna_newtable(L);
    int event = luna_gettop(L);
    const char* fields[] = { "type", "id", "window", "text", "message" };
    for (int i = 0; i < 5; i++) {
        luna_getfield(L, src, fields[i]);
        luna_setfield(L, event, fields[i]);
    }
    luna_settop(L, event);
    return 1;
}

static int h_poll(LunaState* L) {
    GmlApp* app = check_app(L);
    if (!app) return -1;
    double timeout = (double)arg_int(L, 1, 0);
    GmlBuf line;
    buf_init(&line);
    int r = read_line(app, timeout, &line);
    if (r <= 0) {
        buf_free(&line);
        if (r < 0) return lib_error(L, "gml: backend desconectado");
        luna_pushnil(L);
        return 1;
    }
    int pushed = push_event(L, line.data ? line.data : "", line.len);
    buf_free(&line);
    if (pushed) return 1;
    luna_pushnil(L);
    return 1;
}

static int h_get_text(LunaState* L) {
    GmlApp* app = check_app(L);
    if (!app) return -1;
    int64_t id = arg_int(L, 1, 0);
    GmlBuf cmd;
    buf_init(&cmd);
    buf_printf(&cmd, "{\"cmd\":\"get\",\"id\":%lld}", (long long)id);
    int rc = send_command(app, &cmd);
    buf_free(&cmd);
    if (rc != 0) return lib_error(L, "gml: no se pudo consultar el widget");

    int base = luna_gettop(L);
    double deadline = 2.0;
    while (deadline > 0) {
        GmlBuf line;
        buf_init(&line);
        int r = read_line(app, 0.1, &line);
        if (r < 0) { buf_free(&line); luna_settop(L, base); return lib_error(L, "gml: backend desconectado"); }
        if (r == 0) { deadline -= 0.1; continue; }
        if (push_event(L, line.data ? line.data : "", line.len)) {
            int event = luna_gettop(L);
            luna_getfield(L, event, "type");
            const char* type = luna_tostring(L, -1, NULL);
            int is_value = type && strcmp(type, "value") == 0;
            luna_settop(L, event);
            luna_getfield(L, event, "id");
            int64_t event_id = luna_tointeger(L, -1, NULL);
            luna_settop(L, event);
            if (is_value && event_id == id) {
                luna_getfield(L, event, "text");
                luna_settop(L, event + 1);
                buf_free(&line);
                return 1;
            }
            luna_settop(L, base);
        }
        buf_free(&line);
    }
    luna_settop(L, base);
    return lib_error(L, "gml: timeout leyendo el widget");
}

static int h_show(LunaState* L) {
    GmlApp* app = check_app(L);
    if (!app) return -1;
    luna_pushboolean(L, 1);
    return 1;
}

static int h_quit(LunaState* L) {
    GmlApp* app = check_app(L);
    if (!app) return -1;
    app_shutdown(app);
    luna_pushboolean(L, 1);
    return 1;
}

static void register_gml(LunaState* L, LunaTable* exports) {
    lib_export_function(L, exports, "app", h_app);
    lib_export_function(L, exports, "window", h_window);
    lib_export_function(L, exports, "button", h_button);
    lib_export_function(L, exports, "label", h_label);
    lib_export_function(L, exports, "entry", h_entry);
    lib_export_function(L, exports, "text", h_text);
    lib_export_function(L, exports, "checkbox", h_checkbox);
    lib_export_function(L, exports, "set_text", h_set_text);
    lib_export_function(L, exports, "get_text", h_get_text);
    lib_export_function(L, exports, "poll", h_poll);
    lib_export_function(L, exports, "show", h_show);
    lib_export_function(L, exports, "quit", h_quit);
}

static const LunaModule g_gml_module = { "gml_native", register_gml, NULL };

void luna_open_gml_lib(LunaState* L) {
    luna_register_module(L, &g_gml_module);
}

#else

static int gml_unavailable(LunaState* L) {
    return lib_error(L, "gml requiere un sistema POSIX con python3 y tkinter");
}

static void register_gml(LunaState* L, LunaTable* exports) {
    lib_export_function(L, exports, "app", gml_unavailable);
    lib_export_function(L, exports, "window", gml_unavailable);
    lib_export_function(L, exports, "button", gml_unavailable);
    lib_export_function(L, exports, "label", gml_unavailable);
    lib_export_function(L, exports, "entry", gml_unavailable);
    lib_export_function(L, exports, "text", gml_unavailable);
    lib_export_function(L, exports, "checkbox", gml_unavailable);
    lib_export_function(L, exports, "set_text", gml_unavailable);
    lib_export_function(L, exports, "get_text", gml_unavailable);
    lib_export_function(L, exports, "poll", gml_unavailable);
    lib_export_function(L, exports, "show", gml_unavailable);
    lib_export_function(L, exports, "quit", gml_unavailable);
}

static const LunaModule g_gml_module = { "gml_native", register_gml, NULL };

void luna_open_gml_lib(LunaState* L) {
    luna_register_module(L, &g_gml_module);
}

#endif
