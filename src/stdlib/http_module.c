#include "http_module.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Forward-declare from interpreter.c — avoids TokenType clash with Windows SDK */
void runtime_error(int line, const char* fmt, ...);

/* ========== Platform Abstraction (same as net.c) ========== */

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  typedef SOCKET socket_t;
  #define SOCKET_INVALID INVALID_SOCKET
  #define CLOSE_SOCKET closesocket
  #define SOCK_ERR WSAGetLastError()
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <netdb.h>
  #include <unistd.h>
  #include <errno.h>
  typedef int socket_t;
  #define SOCKET_INVALID -1
  #define CLOSE_SOCKET close
  #define SOCK_ERR errno
#endif

static int http_wsa_initialized = 0;

static void ensure_http_wsa(void) {
#ifdef _WIN32
    if (!http_wsa_initialized) {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
            runtime_error(0, "http: WSAStartup failed (error %d)", WSAGetLastError());
        http_wsa_initialized = 1;
    }
#else
    http_wsa_initialized = 1;
#endif
}

/* ========== Helpers ========== */

static void module_add_fn(Value* mod, const char* name, BuiltinFn fn, int min, int max) {
    Value* key = value_new_string(name);
    Value* fn_val = value_new_builtin(name, fn, min, max);
    dict_set(mod, key, fn_val);
}

/* ---- URL Parser ---- */

typedef struct {
    char host[256];
    int  port;
    char path[2048];
} ParsedURL;

/* Parses "http://host[:port]/path" into components.
   Returns 0 on success, -1 on failure. */
static int parse_url(const char* url, ParsedURL* out) {
    memset(out, 0, sizeof(ParsedURL));
    out->port = 80;
    strcpy(out->path, "/");

    /* Skip scheme */
    if (strncmp(url, "http://", 7) == 0)
        url += 7;
    else if (strncmp(url, "https://", 8) == 0)
        return -1;  /* HTTPS not supported */

    /* Extract host[:port] */
    const char* slash = strchr(url, '/');
    const char* colon = strchr(url, ':');

    size_t host_len;
    if (colon && (!slash || colon < slash)) {
        /* host:port */
        host_len = colon - url;
        if (host_len >= sizeof(out->host)) return -1;
        memcpy(out->host, url, host_len);
        out->host[host_len] = '\0';
        out->port = atoi(colon + 1);
        if (out->port <= 0 || out->port > 65535) return -1;
    } else {
        host_len = slash ? (size_t)(slash - url) : strlen(url);
        if (host_len >= sizeof(out->host)) return -1;
        memcpy(out->host, url, host_len);
        out->host[host_len] = '\0';
    }

    /* Extract path */
    if (slash) {
        size_t path_len = strlen(slash);
        if (path_len >= sizeof(out->path)) path_len = sizeof(out->path) - 1;
        memcpy(out->path, slash, path_len);
        out->path[path_len] = '\0';
    }

    return (out->host[0] != '\0') ? 0 : -1;
}

/* ---- TCP Helpers ---- */

static socket_t tcp_connect(const char* host, int port) {
    ensure_http_wsa();

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);

    if (getaddrinfo(host, port_str, &hints, &res) != 0)
        return SOCKET_INVALID;

    socket_t sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock == SOCKET_INVALID) { freeaddrinfo(res); return SOCKET_INVALID; }

    if (connect(sock, res->ai_addr, (int)res->ai_addrlen) != 0) {
        CLOSE_SOCKET(sock);
        freeaddrinfo(res);
        return SOCKET_INVALID;
    }

    freeaddrinfo(res);
    return sock;
}

static int tcp_send_all(socket_t sock, const char* data, int len) {
    int total = 0;
    while (total < len) {
        int sent = send(sock, data + total, len - total, 0);
        if (sent <= 0) return -1;
        total += sent;
    }
    return total;
}

/* Read until connection closes. Returns malloc'd buffer, sets *out_len. */
static char* tcp_recv_all(socket_t sock, size_t* out_len) {
    size_t cap = 8192, len = 0;
    char* buf = (char*)malloc(cap);

    for (;;) {
        if (len + 4096 > cap) {
            cap *= 2;
            buf = (char*)realloc(buf, cap);
        }
        int n = recv(sock, buf + len, (int)(cap - len - 1), 0);
        if (n <= 0) break;
        len += n;
    }

    buf[len] = '\0';
    *out_len = len;
    return buf;
}

/* ---- HTTP Response Parser ---- */

/* Parse raw HTTP response into a Melody dict: {status, headers, body} */
static Value* parse_http_response(const char* raw, size_t raw_len) {
    Value* result = value_new_dict();

    /* Find end of status line */
    const char* line_end = strstr(raw, "\r\n");
    if (!line_end)
        line_end = strstr(raw, "\n");
    if (!line_end) {
        dict_set(result, value_new_string("status"), value_new_int(0));
        dict_set(result, value_new_string("body"), value_new_string(raw));
        dict_set(result, value_new_string("headers"), value_new_dict());
        return result;
    }

    /* Parse status code: "HTTP/1.1 200 OK" */
    int status = 0;
    const char* sp = strchr(raw, ' ');
    if (sp && sp < line_end)
        status = atoi(sp + 1);
    dict_set(result, value_new_string("status"), value_new_int(status));

    /* Find header/body boundary */
    const char* hdr_end = strstr(raw, "\r\n\r\n");
    int hdr_sep_len = 4;
    if (!hdr_end) {
        hdr_end = strstr(raw, "\n\n");
        hdr_sep_len = 2;
    }

    /* Parse headers into a dict */
    Value* headers = value_new_dict();
    const char* hdr_start = line_end + (line_end[1] == '\n' ? 1 : 2);

    if (hdr_end) {
        const char* cur = hdr_start;
        while (cur < hdr_end) {
            const char* next = strstr(cur, "\r\n");
            if (!next || next > hdr_end) next = strstr(cur, "\n");
            if (!next || next > hdr_end) break;

            const char* colon = strchr(cur, ':');
            if (colon && colon < next) {
                /* key */
                size_t klen = colon - cur;
                char* key = melody_strndup(cur, klen);
                /* value (skip ": ") */
                const char* vstart = colon + 1;
                while (vstart < next && *vstart == ' ') vstart++;
                size_t vlen = next - vstart;
                char* val = melody_strndup(vstart, vlen);

                dict_set(headers, value_new_string_take(key), value_new_string_take(val));
            }

            /* Advance past \r\n or \n */
            cur = next + (next[0] == '\r' ? 2 : 1);
        }
    }
    dict_set(result, value_new_string("headers"), headers);

    /* Body = everything after header separator */
    if (hdr_end) {
        const char* body = hdr_end + hdr_sep_len;
        size_t body_len = raw_len - (body - raw);
        char* body_str = melody_strndup(body, body_len);
        dict_set(result, value_new_string("body"), value_new_string_take(body_str));
    } else {
        dict_set(result, value_new_string("body"), value_new_string(""));
    }

    return result;
}

/* ========== Core HTTP Request ========== */

static Value* do_http_request(const char* method, const char* url,
                              const char* body_data, int body_len,
                              const char* content_type) {
    ParsedURL pu;
    if (parse_url(url, &pu) != 0)
        runtime_error(0, "http: invalid URL '%s' (only http:// supported)", url);

    socket_t sock = tcp_connect(pu.host, pu.port);
    if (sock == SOCKET_INVALID)
        runtime_error(0, "http: cannot connect to %s:%d", pu.host, pu.port);

    /* Build request */
    size_t req_cap = 4096 + body_len;
    char* req = (char*)malloc(req_cap);
    int req_len;

    if (body_data && body_len > 0) {
        req_len = snprintf(req, req_cap,
            "%s %s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n",
            method, pu.path, pu.host,
            content_type ? content_type : "text/plain",
            body_len);
        memcpy(req + req_len, body_data, body_len);
        req_len += body_len;
    } else {
        req_len = snprintf(req, req_cap,
            "%s %s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "Connection: close\r\n"
            "\r\n",
            method, pu.path, pu.host);
    }

    /* Send */
    if (tcp_send_all(sock, req, req_len) < 0) {
        free(req);
        CLOSE_SOCKET(sock);
        runtime_error(0, "http: send failed");
    }
    free(req);

    /* Receive full response */
    size_t resp_len;
    char* resp = tcp_recv_all(sock, &resp_len);
    CLOSE_SOCKET(sock);

    /* Parse and return dict */
    Value* result = parse_http_response(resp, resp_len);
    free(resp);
    return result;
}

/* ========== Melody Functions ========== */

/* http.get(url) → {status, headers, body} */
static Value* builtin_http_get(Value** args, int argc) {
    if (args[0]->type != VAL_STRING)
        runtime_error(0, "http.get(): url must be a string");

    return do_http_request("GET", args[0]->as.string, NULL, 0, NULL);
}

/* http.post(url, body, content_type?) → {status, headers, body} */
static Value* builtin_http_post(Value** args, int argc) {
    if (args[0]->type != VAL_STRING)
        runtime_error(0, "http.post(): url must be a string");
    if (args[1]->type != VAL_STRING)
        runtime_error(0, "http.post(): body must be a string");

    const char* ct = "application/x-www-form-urlencoded";
    if (argc >= 3 && args[2]->type == VAL_STRING)
        ct = args[2]->as.string;

    return do_http_request("POST", args[0]->as.string,
                           args[1]->as.string, (int)strlen(args[1]->as.string), ct);
}

/* http.put(url, body, content_type?) → {status, headers, body} */
static Value* builtin_http_put(Value** args, int argc) {
    if (args[0]->type != VAL_STRING)
        runtime_error(0, "http.put(): url must be a string");
    if (args[1]->type != VAL_STRING)
        runtime_error(0, "http.put(): body must be a string");

    const char* ct = "application/json";
    if (argc >= 3 && args[2]->type == VAL_STRING)
        ct = args[2]->as.string;

    return do_http_request("PUT", args[0]->as.string,
                           args[1]->as.string, (int)strlen(args[1]->as.string), ct);
}

/* http.delete(url) → {status, headers, body} */
static Value* builtin_http_delete(Value** args, int argc) {
    if (args[0]->type != VAL_STRING)
        runtime_error(0, "http.delete(): url must be a string");

    return do_http_request("DELETE", args[0]->as.string, NULL, 0, NULL);
}

/* ========== Module Creation ========== */

Value* create_http_module(void) {
    static Value* cached = NULL;
    if (cached) return cached;

    Value* mod = value_new_dict();
    module_add_fn(mod, "get",    builtin_http_get,    1, 1);
    module_add_fn(mod, "post",   builtin_http_post,   2, 3);
    module_add_fn(mod, "put",    builtin_http_put,    2, 3);
    module_add_fn(mod, "delete", builtin_http_delete,  1, 1);

    cached = mod;
    return mod;
}
