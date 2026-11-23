#ifndef _WIN32
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#endif

#include "stdlib_internal.h"

#ifndef _WIN32

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdarg.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "../runtime/lstring.h"
#include "../runtime/table.h"
#include "../runtime/userdata.h"

#define HTTP_DEFAULT_TIMEOUT 5.0
#define HTTP_MAX_HEADER_BYTES (1024 * 1024)
#define HTTP_MAX_BODY_BYTES (64 * 1024 * 1024)

typedef struct {
    char* key;
    char* value;
} HttpHeader;

typedef struct {
    char* data;
    size_t len;
    size_t cap;
} HttpBuf;

typedef struct {
    int fd;
} HttpServer;

typedef struct {
    int status;
    char* reason;
    char* body;
    size_t body_len;
    LunaTable* headers;
} HttpReply;

static void buf_init(HttpBuf* b) {
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

static void buf_free(HttpBuf* b) {
    free(b->data);
    buf_init(b);
}

static int buf_reserve(HttpBuf* b, size_t extra) {
    if (b->len + extra + 1 <= b->cap) return 0;
    size_t cap = b->cap ? b->cap : 4096;
    while (cap < b->len + extra + 1) cap *= 2;
    char* p = (char*)realloc(b->data, cap);
    if (!p) return -1;
    b->data = p;
    b->cap = cap;
    return 0;
}

static int buf_append(HttpBuf* b, const char* s, size_t n) {
    if (buf_reserve(b, n) != 0) return -1;
    memcpy(b->data + b->len, s, n);
    b->len += n;
    b->data[b->len] = '\0';
    return 0;
}

static int buf_append_str(HttpBuf* b, const char* s) {
    return buf_append(b, s, strlen(s));
}

static int buf_appendf(HttpBuf* b, const char* fmt, ...) {
    char tmp[1024];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);
    if (n < 0 || (size_t)n >= sizeof(tmp)) return -1;
    return buf_append(b, tmp, (size_t)n);
}

static char* str_dup_n(const char* s, size_t n) {
    char* p = (char*)malloc(n + 1);
    if (!p) return NULL;
    memcpy(p, s, n);
    p[n] = '\0';
    return p;
}

static void trim_spaces(char** s) {
    char* p = *s;
    while (*p && isspace((unsigned char)*p)) p++;
    char* end = p + strlen(p);
    while (end > p && isspace((unsigned char)end[-1])) end--;
    *end = '\0';
    *s = p;
}

static int contains_nocase(const char* haystack, const char* needle) {
    size_t n = strlen(needle);
    if (n == 0) return 1;
    for (const char* p = haystack; *p; p++) {
        size_t i = 0;
        while (i < n && p[i] && tolower((unsigned char)p[i]) == tolower((unsigned char)needle[i])) i++;
        if (i == n) return 1;
    }
    return 0;
}

static char* lower_copy(const char* s, size_t n) {
    char* out = (char*)malloc(n + 1);
    if (!out) return NULL;
    for (size_t i = 0; i < n; i++) out[i] = (char)tolower((unsigned char)s[i]);
    out[n] = '\0';
    return out;
}

static void headers_free(HttpHeader* headers, size_t count) {
    if (!headers) return;
    for (size_t i = 0; i < count; i++) {
        free(headers[i].key);
        free(headers[i].value);
    }
    free(headers);
}

static const char* header_get(HttpHeader* headers, size_t count, const char* name) {
    for (size_t i = 0; i < count; i++) {
        if (strcasecmp(headers[i].key, name) == 0) return headers[i].value;
    }
    return NULL;
}

static char* value_text(LunaValue v) {
    char buf[64];
    switch (v.type) {
        case LUNA_TNIL: return str_dup_n("", 0);
        case LUNA_TBOOLEAN: return str_dup_n(luna_val_bool(v) ? "true" : "false", luna_val_bool(v) ? 4 : 5);
        case LUNA_TINTEGER:
            snprintf(buf, sizeof(buf), "%lld", (long long)luna_val_int(v));
            return str_dup_n(buf, strlen(buf));
        case LUNA_TFLOAT:
            snprintf(buf, sizeof(buf), "%.14g", luna_val_float(v));
            return str_dup_n(buf, strlen(buf));
        case LUNA_TSTRING: {
            LunaString* s = luna_val_string(v);
            return str_dup_n(s->data, s->len);
        }
        default: return NULL;
    }
}

static int parse_head(HttpBuf* buf, char** first_line, HttpHeader** headers,
                      size_t* header_count, size_t* body_start) {
    char* data = buf->data;
    size_t end = 0;
    while (end + 3 < buf->len) {
        if (data[end] == '\r' && data[end + 1] == '\n' &&
            data[end + 2] == '\r' && data[end + 3] == '\n') break;
        end++;
    }
    if (end + 3 >= buf->len) return 1;
    if (end > HTTP_MAX_HEADER_BYTES) return -2;

    size_t head_end = end + 2;
    data[head_end] = '\0';
    *body_start = end + 4;

    char* line = data;
    char* next = strchr(line, '\n');
    if (!next) return -2;
    *next = '\0';
    if (next > line && next[-1] == '\r') next[-1] = '\0';
    *first_line = str_dup_n(line, (size_t)(next - line));
    if (!*first_line) return -1;

    size_t cap = 8, count = 0;
    HttpHeader* list = (HttpHeader*)calloc(cap, sizeof(HttpHeader));
    if (!list) { free(*first_line); *first_line = NULL; return -1; }

    char* p = next + 1;
    while (p && *p) {
        char* nl = strchr(p, '\n');
        if (nl) {
            *nl = '\0';
            if (nl > p && nl[-1] == '\r') nl[-1] = '\0';
        }
        char* colon = strchr(p, ':');
        if (colon && colon != p) {
            *colon = '\0';
            char* key = p;
            char* val = colon + 1;
            trim_spaces(&key);
            trim_spaces(&val);
            if (count == cap) {
                cap *= 2;
                HttpHeader* bigger = (HttpHeader*)realloc(list, cap * sizeof(HttpHeader));
                if (!bigger) { headers_free(list, count); free(*first_line); *first_line = NULL; return -1; }
                list = bigger;
            }
            list[count].key = str_dup_n(key, strlen(key));
            list[count].value = str_dup_n(val, strlen(val));
            if (!list[count].key || !list[count].value) {
                headers_free(list, count + 1);
                free(*first_line);
                *first_line = NULL;
                return -1;
            }
            count++;
        }
        p = nl ? nl + 1 : NULL;
    }

    *headers = list;
    *header_count = count;
    return 0;
}

static int read_more(int fd, HttpBuf* buf, double timeout, size_t* total) {
    char tmp[8192];
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;
    int ms = timeout < 0 ? -1 : (int)(timeout * 1000);
    int ready = poll(&pfd, 1, ms);
    if (ready == 0) return 1;
    if (ready < 0) return errno == EINTR ? 1 : -1;
    ssize_t n = recv(fd, tmp, sizeof(tmp), 0);
    if (n == 0) return 2;
    if (n < 0) {
        if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) return 1;
        return -1;
    }
    if (buf_append(buf, tmp, (size_t)n) != 0) return -1;
    *total += (size_t)n;
    if (*total > HTTP_MAX_BODY_BYTES + HTTP_MAX_HEADER_BYTES) return -2;
    return 0;
}

static int read_head(int fd, HttpBuf* buf, double timeout, char** first,
                     HttpHeader** headers, size_t* count, size_t* body_start) {
    size_t total = 0;
    for (;;) {
        int r = parse_head(buf, first, headers, count, body_start);
        if (r == 0) return 0;
        if (r == -2) return -1;
        int more = read_more(fd, buf, timeout, &total);
        if (more == 0 || more == 1) continue;
        return -1;
    }
}

static int ensure_bytes(int fd, HttpBuf* buf, size_t need, size_t from,
                        double timeout) {
    while (buf->len - from < need) {
        size_t total = buf->len;
        int r = read_more(fd, buf, timeout, &total);
        if (r == 0 || r == 1) continue;
        return -1;
    }
    return 0;
}

static int decode_chunked(int fd, HttpBuf* raw, size_t pos, HttpBuf* body,
                          double timeout) {
    for (;;) {
        if (ensure_bytes(fd, raw, 2, pos, timeout) != 0) return -1;
        char* crlf = NULL;
        for (;;) {
            char* p = raw->data + pos;
            char* limit = raw->data + raw->len - 1;
            while (p + 1 < limit) {
                if (p[0] == '\r' && p[1] == '\n') { crlf = p; break; }
                p++;
            }
            if (crlf) break;
            size_t ignored = raw->len;
            if (read_more(fd, raw, timeout, &ignored) != 0) return -1;
        }

        char* size_text = str_dup_n(raw->data + pos, (size_t)(crlf - raw->data - pos));
        if (!size_text) return -1;
        char* semi = strchr(size_text, ';');
        if (semi) *semi = '\0';
        trim_spaces(&size_text);
        char* endptr = NULL;
        unsigned long long chunk = strtoull(size_text, &endptr, 16);
        free(size_text);
        if (endptr == size_text || *endptr != '\0') return -1;

        pos = (size_t)(crlf - raw->data) + 2;
        if (chunk == 0) {
            for (;;) {
                if (pos + 3 < raw->len &&
                    raw->data[pos] == '\r' && raw->data[pos + 1] == '\n' &&
                    raw->data[pos + 2] == '\r' && raw->data[pos + 3] == '\n') return 0;
                size_t ignored = raw->len;
                if (read_more(fd, raw, timeout, &ignored) != 0) return -1;
            }
        }
        if (chunk > HTTP_MAX_BODY_BYTES) return -1;
        if (ensure_bytes(fd, raw, (size_t)chunk + 2, pos, timeout) != 0) return -1;
        if (buf_append(body, raw->data + pos, (size_t)chunk) != 0) return -1;
        pos += (size_t)chunk;
        if (raw->data[pos] != '\r' || raw->data[pos + 1] != '\n') return -1;
        pos += 2;
    }
}

static int collect_body(int fd, HttpBuf* raw, size_t pos, HttpHeader* headers,
                        size_t count, int no_body, HttpBuf* body, double timeout,
                        int until_eof) {
    if (no_body) return 0;
    const char* transfer = header_get(headers, count, "transfer-encoding");
    if (transfer && contains_nocase(transfer, "chunked")) {
        return decode_chunked(fd, raw, pos, body, timeout);
    }
    const char* length = header_get(headers, count, "content-length");
    if (length) {
        char* end = NULL;
        unsigned long long n = strtoull(length, &end, 10);
        if (end == length || *end != '\0' || n > HTTP_MAX_BODY_BYTES) return -1;
        int r = ensure_bytes(fd, raw, (size_t)n, pos, timeout);
        if (r != 0) return r;
        return buf_append(body, raw->data + pos, (size_t)n);
    }
    if (buf_append(body, raw->data + pos, raw->len - pos) != 0) return -1;
    if (!until_eof) return 0;
    for (;;) {
        char tmp[8192];
        ssize_t got = recv(fd, tmp, sizeof(tmp), 0);
        if (got == 0) return 0;
        if (got < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (buf_append(body, tmp, (size_t)got) != 0) return -1;
        if (body->len > HTTP_MAX_BODY_BYTES) return -1;
    }
}

static const char* reason_phrase(int status) {
    switch (status) {
        case 200: return "OK";
        case 201: return "Created";
        case 202: return "Accepted";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 304: return "Not Modified";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 409: return "Conflict";
        case 418: return "I'm a teapot";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 503: return "Service Unavailable";
        default: return "Status";
    }
}

typedef struct {
    char* host;
    int port;
    char* path;
} HttpUrl;

static void url_free(HttpUrl* u) {
    free(u->host);
    free(u->path);
    u->host = NULL;
    u->path = NULL;
}

static int parse_url(const char* url, HttpUrl* out) {
    const char* prefix = "http://";
    if (strncmp(url, "https://", 8) == 0) return -2;
    if (strncmp(url, prefix, strlen(prefix)) != 0) return -1;
    const char* rest = url + strlen(prefix);
    const char* slash = strchr(rest, '/');
    size_t auth_len = slash ? (size_t)(slash - rest) : strlen(rest);
    if (auth_len == 0) return -1;
    char* auth = str_dup_n(rest, auth_len);
    if (!auth) return -1;

    char* host = NULL;
    char* port_text = NULL;
    if (auth[0] == '[') {
        char* close = strchr(auth, ']');
        if (!close) { free(auth); return -1; }
        host = str_dup_n(auth + 1, (size_t)(close - auth - 1));
        if (close[1] == ':') port_text = str_dup_n(close + 2, strlen(close + 2));
        else if (close[1] != '\0') { free(auth); free(host); return -1; }
    } else {
        char* colon = strrchr(auth, ':');
        if (colon) {
            *colon = '\0';
            port_text = str_dup_n(colon + 1, strlen(colon + 1));
        }
        host = str_dup_n(auth, strlen(auth));
    }
    free(auth);
    if (!host || *host == '\0') { free(host); free(port_text); return -1; }

    int port = 80;
    if (port_text && *port_text) {
        char* end = NULL;
        long p = strtol(port_text, &end, 10);
        if (end == port_text || *end != '\0' || p <= 0 || p > 65535) {
            free(host); free(port_text); return -1;
        }
        port = (int)p;
    }
    free(port_text);

    const char* path = slash ? slash : "/";
    out->host = host;
    out->port = port;
    out->path = str_dup_n(path, strlen(path));
    if (!out->path) { url_free(out); return -1; }
    return 0;
}

static int set_timeout(int fd, double seconds) {
    struct timeval tv;
    tv.tv_sec = (time_t)seconds;
    tv.tv_usec = (suseconds_t)((seconds - (double)tv.tv_sec) * 1000000.0);
    if (tv.tv_sec < 0) tv.tv_sec = 0;
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0) return -1;
    return setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
}

static int connect_host(const char* host, int port, double timeout, int* out_fd) {
    char portbuf[16];
    snprintf(portbuf, sizeof(portbuf), "%d", port);
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo* list = NULL;
    if (getaddrinfo(host, portbuf, &hints, &list) != 0) return -1;

    for (struct addrinfo* ai = list; ai; ai = ai->ai_next) {
        int fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (fd < 0) continue;
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags >= 0) fcntl(fd, F_SETFL, flags | O_NONBLOCK);
        int rc = connect(fd, ai->ai_addr, ai->ai_addrlen);
        if (rc != 0 && errno != EINPROGRESS) { close(fd); continue; }
        if (rc != 0) {
            struct pollfd pfd;
            pfd.fd = fd;
            pfd.events = POLLOUT;
            int ms = timeout <= 0 ? -1 : (int)(timeout * 1000);
            int ready = poll(&pfd, 1, ms);
            int so_error = 0;
            socklen_t len = sizeof(so_error);
            if (ready <= 0 || getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_error, &len) != 0 || so_error != 0) {
                close(fd);
                continue;
            }
        }
        if (flags >= 0) fcntl(fd, F_SETFL, flags);
        set_timeout(fd, timeout);
        *out_fd = fd;
        freeaddrinfo(list);
        return 0;
    }
    freeaddrinfo(list);
    return -1;
}

static int send_all(int fd, const char* data, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        int flags = 0;
#ifdef MSG_NOSIGNAL
        flags = MSG_NOSIGNAL;
#endif
        ssize_t n = send(fd, data + sent, len - sent, flags);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) return -1;
        sent += (size_t)n;
    }
    return 0;
}

static const char* arg_string(LunaState* L, int index, size_t* len, int required,
                              int* failed) {
    LunaValue v = lib_arg(L, index);
    if (v.type == LUNA_TNIL && !required) return NULL;
    if (v.type != LUNA_TSTRING) {
        if (failed) *failed = 1;
        return NULL;
    }
    LunaString* s = luna_val_string(v);
    if (len) *len = s->len;
    return s->data;
}

static double arg_number(LunaState* L, int index, double fallback) {
    double out;
    if (luna_value_to_number(lib_arg(L, index), &out)) return out;
    return fallback;
}

static LunaTable* arg_table(LunaState* L, int index, int* failed) {
    LunaValue v = lib_arg(L, index);
    if (v.type == LUNA_TNIL) return NULL;
    if (v.type != LUNA_TTABLE) { if (failed) *failed = 1; return NULL; }
    return luna_val_table(v);
}

static int append_luna_headers(HttpBuf* out, LunaTable* table) {
    if (!table) return 0;
    for (uint32_t i = 0; i < table->size; i++) {
        LunaTableNode* node = &table->nodes[i];
        if (node->key.type != LUNA_TSTRING || node->value.type == LUNA_TNIL) continue;
        LunaString* key = luna_val_string(node->key);
        char* value = value_text(node->value);
        if (!value) continue;
        int bad = buf_append(out, key->data, key->len) != 0 ||
                  buf_append_str(out, ": ") != 0 ||
                  buf_append_str(out, value) != 0 ||
                  buf_append_str(out, "\r\n") != 0;
        free(value);
        if (bad) return -1;
    }
    return 0;
}

static int push_headers(LunaState* L, HttpHeader* headers, size_t count) {
    luna_newtable(L);
    int table = luna_gettop(L);
    for (size_t i = 0; i < count; i++) {
        size_t n = strlen(headers[i].key);
        char* key = lower_copy(headers[i].key, n);
        if (!key) return -1;
        luna_pushlstring(L, key, n);
        free(key);
        luna_pushlstring(L, headers[i].value, strlen(headers[i].value));
        luna_settable(L, table);
    }
    return 0;
}

static int h_request(LunaState* L) {
    int base = luna_gettop(L);
    int failed = 0;
    size_t method_len = 0, url_len = 0, body_len = 0;
    const char* method = arg_string(L, 0, &method_len, 1, &failed);
    const char* url_text = arg_string(L, 1, &url_len, 1, &failed);
    const char* body = arg_string(L, 3, &body_len, 0, &failed);
    LunaTable* extra = arg_table(L, 2, &failed);
    if (failed) return lib_error(L, "request: argumentos inválidos");
    double timeout = arg_number(L, 4, HTTP_DEFAULT_TIMEOUT);
    if (timeout <= 0) timeout = HTTP_DEFAULT_TIMEOUT;

    char* url_copy = str_dup_n(url_text, url_len);
    HttpUrl url;
    memset(&url, 0, sizeof(url));
    int parsed = url_copy ? parse_url(url_copy, &url) : -1;
    if (parsed != 0) {
        if (parsed == -2) { free(url_copy); url_free(&url); return lib_error(L, "https requiere TLS: usa un cliente con TLS"); }
        int detail = lib_error(L, "url http inválida");
        free(url_copy);
        url_free(&url);
        return detail;
    }
    free(url_copy);

    int fd = -1;
    if (connect_host(url.host, url.port, timeout, &fd) != 0) {
        url_free(&url);
        return lib_error(L, "no se pudo conectar con %s:%d", url.host, url.port);
    }

    HttpBuf req;
    buf_init(&req);
    int bad = buf_appendf(&req, "%.*s %s HTTP/1.1\r\n", (int)method_len, method, url.path) != 0 ||
              buf_appendf(&req, "Host: %s", url.host) != 0 ||
              (url.port != 80 && buf_appendf(&req, ":%d", url.port) != 0) ||
              buf_append_str(&req, "\r\n") != 0 ||
              buf_append_str(&req, "User-Agent: Luna/0.7.6\r\n") != 0 ||
              buf_append_str(&req, "Connection: close\r\n") != 0 ||
              append_luna_headers(&req, extra) != 0;
    if (!bad && body && body_len > 0) bad = buf_appendf(&req, "Content-Length: %zu\r\n", body_len) != 0;
    if (!bad) bad = buf_append_str(&req, "\r\n") != 0;
    if (!bad && body && body_len > 0) bad = buf_append(&req, body, body_len) != 0;
    url_free(&url);

    if (bad || send_all(fd, req.data, req.len) != 0) {
        buf_free(&req);
        close(fd);
        return lib_error(L, "no se pudo enviar la petición HTTP");
    }
    buf_free(&req);

    HttpBuf raw;
    buf_init(&raw);
    char* first = NULL;
    HttpHeader* headers = NULL;
    size_t hcount = 0, body_start = 0;
    if (read_head(fd, &raw, timeout, &first, &headers, &hcount, &body_start) != 0) {
        buf_free(&raw);
        close(fd);
        return lib_error(L, "no se pudo leer la respuesta HTTP");
    }

    int code = 0, reason_pos = 0;
    int matched = sscanf(first, "HTTP/1.%*d %d %n", &code, &reason_pos);
    char* reason = matched >= 1 ? str_dup_n(first + reason_pos, strlen(first + reason_pos)) : NULL;
    if (reason) trim_spaces(&reason);
    free(first);

    HttpBuf body_buf;
    buf_init(&body_buf);
    int no_body = (code >= 100 && code < 200) || code == 204 || code == 304;
    int body_r = (matched >= 1 && reason)
        ? collect_body(fd, &raw, body_start, headers, hcount, no_body, &body_buf, timeout, 1)
        : -1;
    close(fd);

    if (matched < 1 || !reason || body_r != 0) {
        free(reason);
        headers_free(headers, hcount);
        buf_free(&raw);
        buf_free(&body_buf);
        return lib_error(L, "respuesta HTTP inválida");
    }

    luna_newtable(L);
    int resp = luna_gettop(L);
    luna_pushboolean(L, code >= 200 && code < 400);
    luna_setfield(L, resp, "ok");
    luna_pushinteger(L, code);
    luna_setfield(L, resp, "status");
    luna_pushlstring(L, reason, strlen(reason));
    luna_setfield(L, resp, "reason");
    int push_bad = push_headers(L, headers, hcount) != 0;
    if (!push_bad) luna_setfield(L, resp, "headers");
    if (!push_bad) {
        luna_pushlstring(L, body_buf.data ? body_buf.data : "", body_buf.len);
        luna_setfield(L, resp, "body");
    }

    free(reason);
    headers_free(headers, hcount);
    buf_free(&raw);
    buf_free(&body_buf);
    if (push_bad) { luna_settop(L, base); return -1; }
    return 1;
}

typedef struct {
    int fd;
} HttpConnection;

static void server_finalizer(LunaState* L, void* ud) {
    (void)L;
    HttpServer* server = (HttpServer*)ud;
    if (server && server->fd >= 0) {
        close(server->fd);
        server->fd = -1;
    }
}

static void connection_finalizer(LunaState* L, void* ud) {
    (void)L;
    HttpConnection* conn = (HttpConnection*)ud;
    if (conn && conn->fd >= 0) {
        close(conn->fd);
        conn->fd = -1;
    }
}

static HttpServer* check_server(LunaState* L) {
    LunaValue v = lib_arg(L, 0);
    if (v.type != LUNA_TUSERDATA) {
        lib_error(L, "se esperaba un servidor http");
        return NULL;
    }
    HttpServer* server = (HttpServer*)luna_userdata_data(luna_val_userdata(v));
    if (!server || server->fd < 0) {
        lib_error(L, "servidor http cerrado");
        return NULL;
    }
    return server;
}

static HttpConnection* check_connection(LunaState* L) {
    LunaValue v = lib_arg(L, 0);
    if (v.type != LUNA_TUSERDATA) {
        lib_error(L, "se esperaba una conexión http");
        return NULL;
    }
    return (HttpConnection*)luna_userdata_data(luna_val_userdata(v));
}

static int h_listen(LunaState* L) {
    int failed = 0;
    size_t host_len = 0;
    const char* host = arg_string(L, 0, &host_len, 0, &failed);
    LunaValue portv = lib_arg(L, 1);
    if (failed || portv.type != LUNA_TINTEGER) return lib_error(L, "listen: host y puerto inválidos");
    int port = (int)luna_val_int(portv);
    int backlog = (int)arg_number(L, 2, 64);
    if (port <= 0 || port > 65535) return lib_error(L, "puerto fuera de rango");
    if (!host) { host = "127.0.0.1"; host_len = 9; }

    char* host_copy = str_dup_n(host, host_len);
    char portbuf[16];
    snprintf(portbuf, sizeof(portbuf), "%d", port);
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    struct addrinfo* list = NULL;
    int fd = -1;
    if (host_copy && getaddrinfo(host_copy, portbuf, &hints, &list) == 0) {
        for (struct addrinfo* ai = list; ai; ai = ai->ai_next) {
            fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
            if (fd < 0) continue;
            int yes = 1;
            setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
            if (bind(fd, ai->ai_addr, ai->ai_addrlen) == 0 &&
                listen(fd, backlog > 0 ? backlog : 64) == 0) break;
            close(fd);
            fd = -1;
        }
        freeaddrinfo(list);
    }
    free(host_copy);
    if (fd < 0) return lib_error(L, "no se pudo escuchar en %s:%d", host, port);

    HttpServer* server = (HttpServer*)luna_newuserdata_finalized(L, sizeof(HttpServer), server_finalizer);
    if (!server) { close(fd); return -1; }
    server->fd = fd;
    return 1;
}

static int h_close(LunaState* L) {
    HttpServer* server = check_server(L);
    if (!server) return -1;
    close(server->fd);
    server->fd = -1;
    luna_pushboolean(L, 1);
    return 1;
}

static int h_accept(LunaState* L) {
    int base = luna_gettop(L);
    HttpServer* server = check_server(L);
    if (!server) return -1;
    double timeout = arg_number(L, 1, 1.0);

    struct pollfd pfd;
    pfd.fd = server->fd;
    pfd.events = POLLIN;
    int ms = timeout < 0 ? -1 : (int)(timeout * 1000);
    int ready = poll(&pfd, 1, ms);
    if (ready == 0) { luna_pushnil(L); return 1; }
    if (ready < 0) return lib_error(L, "poll falló: %s", strerror(errno));

    int client = accept(server->fd, NULL, NULL);
    if (client < 0) return lib_error(L, "accept falló: %s", strerror(errno));
    set_timeout(client, timeout > 0 ? timeout : HTTP_DEFAULT_TIMEOUT);

    HttpBuf raw;
    buf_init(&raw);
    char* first = NULL;
    HttpHeader* headers = NULL;
    size_t hcount = 0, body_start = 0;
    int r = read_head(client, &raw, timeout, &first, &headers, &hcount, &body_start);
    if (r != 0) {
        close(client);
        buf_free(&raw);
        return lib_error(L, "petición HTTP inválida");
    }

    char method[64] = "";
    char path[2048] = "";
    sscanf(first, "%63s %2047s", method, path);
    free(first);
    HttpBuf req_body;
    buf_init(&req_body);
    r = collect_body(client, &raw, body_start, headers, hcount, 0, &req_body,
                     timeout > 0 ? timeout : HTTP_DEFAULT_TIMEOUT, 0);
    if (r != 0) {
        close(client);
        headers_free(headers, hcount);
        buf_free(&raw);
        buf_free(&req_body);
        return lib_error(L, "no se pudo leer el cuerpo de la petición");
    }

    HttpConnection* conn = (HttpConnection*)luna_newuserdata_finalized(L, sizeof(HttpConnection), connection_finalizer);
    if (!conn) {
        close(client);
        headers_free(headers, hcount);
        buf_free(&raw);
        buf_free(&req_body);
        return -1;
    }
    conn->fd = client;

    luna_newtable(L);
    int incoming = luna_gettop(L);
    luna_pushvalue(L, base + 1);
    luna_setfield(L, incoming, "connection");

    luna_newtable(L);
    int request = luna_gettop(L);
    luna_pushlstring(L, method, strlen(method));
    luna_setfield(L, request, "method");
    luna_pushlstring(L, path, strlen(path));
    luna_setfield(L, request, "path");
    luna_pushlstring(L, req_body.data ? req_body.data : "", req_body.len);
    luna_setfield(L, request, "body");
    int bad = push_headers(L, headers, hcount) != 0;
    if (!bad) luna_setfield(L, request, "headers");
    if (!bad) luna_setfield(L, incoming, "request");

    headers_free(headers, hcount);
    buf_free(&raw);
    buf_free(&req_body);
    if (bad) { luna_settop(L, base); return -1; }
    return 1;
}

static void reply_free(HttpReply* reply) {
    free(reply->reason);
    free(reply->body);
    reply->reason = NULL;
    reply->body = NULL;
    reply->headers = NULL;
}

static int reply_has_header(HttpReply* reply, const char* name) {
    if (!reply->headers) return 0;
    for (uint32_t i = 0; i < reply->headers->size; i++) {
        LunaTableNode* node = &reply->headers->nodes[i];
        if (node->key.type != LUNA_TSTRING) continue;
        LunaString* key = luna_val_string(node->key);
        if (key->len == strlen(name) && strncasecmp(key->data, name, key->len) == 0) return 1;
    }
    return 0;
}

static int build_reply_bytes(HttpReply* reply, HttpBuf* out) {
    if (buf_appendf(out, "HTTP/1.1 %d %s\r\n", reply->status,
                    reply->reason ? reply->reason : "Status") != 0) return -1;
    if (reply->headers && append_luna_headers(out, reply->headers) != 0) return -1;
    if (!reply_has_header(reply, "Content-Length") &&
        buf_appendf(out, "Content-Length: %zu\r\n", reply->body_len) != 0) return -1;
    if (!reply_has_header(reply, "Connection") &&
        buf_append_str(out, "Connection: close\r\n") != 0) return -1;
    if (buf_append_str(out, "\r\n") != 0) return -1;
    return buf_append(out, reply->body ? reply->body : "", reply->body_len);
}

static int table_field(LunaState* L, LunaTable* table, const char* name, LunaValue* out) {
    LunaString* key = luna_string_newz(L, name);
    if (!key) return 0;
    return luna_table_get(table, luna_string_val(key), out);
}

static int reply_from_table(LunaState* L, LunaValue value, HttpReply* reply) {
    memset(reply, 0, sizeof(*reply));
    if (value.type != LUNA_TTABLE) {
        lib_error(L, "respuesta http inválida");
        return -1;
    }
    LunaTable* table = luna_val_table(value);
    LunaValue field;

    reply->status = 200;
    if (table_field(L, table, "status", &field) && field.type == LUNA_TINTEGER) {
        reply->status = (int)luna_val_int(field);
    }

    const char* fallback = reason_phrase(reply->status);
    reply->reason = str_dup_n(fallback, strlen(fallback));
    if (table_field(L, table, "reason", &field)) {
        char* custom = value_text(field);
        if (custom && custom[0] != '\0') {
            free(reply->reason);
            reply->reason = custom;
        } else {
            free(custom);
        }
    }
    if (!reply->reason) return lib_error(L, "reason inválido en la respuesta http");

    reply->body = NULL;
    if (table_field(L, table, "body", &field)) {
        reply->body = value_text(field);
    }
    if (!reply->body) { free(reply->reason); return lib_error(L, "body inválido en la respuesta http"); }
    reply->body_len = strlen(reply->body);

    reply->headers = NULL;
    if (table_field(L, table, "headers", &field) && field.type == LUNA_TTABLE) {
        reply->headers = luna_val_table(field);
    }
    return 0;
}

static int h_respond(LunaState* L) {
    HttpConnection* conn = check_connection(L);
    if (!conn) return -1;
    if (conn->fd < 0) return lib_error(L, "conexión http cerrada");

    HttpReply reply;
    if (reply_from_table(L, lib_arg(L, 1), &reply) != 0) return -1;

    HttpBuf out;
    buf_init(&out);
    int bad = build_reply_bytes(&reply, &out) != 0;
    if (!bad) bad = send_all(conn->fd, out.data, out.len) != 0;
    close(conn->fd);
    conn->fd = -1;
    buf_free(&out);
    reply_free(&reply);
    if (bad) return lib_error(L, "no se pudo enviar la respuesta http");
    luna_pushboolean(L, 1);
    return 1;
}

static int h_close_connection(LunaState* L) {
    HttpConnection* conn = check_connection(L);
    if (!conn) return -1;
    if (conn->fd >= 0) {
        close(conn->fd);
        conn->fd = -1;
    }
    luna_pushboolean(L, 1);
    return 1;
}

#endif

#ifdef _WIN32
static int h_stub(LunaState* L) {
    return lib_error(L, "http no está disponible en Windows todavía");
}
#endif

static void register_http(LunaState* L, LunaTable* exports) {
#ifndef _WIN32
    lib_export_function(L, exports, "request", h_request);
    lib_export_function(L, exports, "listen", h_listen);
    lib_export_function(L, exports, "close", h_close);
    lib_export_function(L, exports, "accept", h_accept);
    lib_export_function(L, exports, "respond", h_respond);
    lib_export_function(L, exports, "close_connection", h_close_connection);
#else
    lib_export_function(L, exports, "request", h_stub);
    lib_export_function(L, exports, "listen", h_stub);
    lib_export_function(L, exports, "close", h_stub);
    lib_export_function(L, exports, "accept", h_stub);
    lib_export_function(L, exports, "respond", h_stub);
    lib_export_function(L, exports, "close_connection", h_stub);
#endif
}

static const LunaModule g_http_module = { "http_native", register_http, NULL };

void luna_open_http_lib(LunaState* L) {
    luna_register_module(L, &g_http_module);
}
