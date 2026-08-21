#include "net.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Forward-declare from interpreter.c — avoids pulling in lexer.h which
   clashes with Windows SDK's TokenType in winnt.h. */
void runtime_error(int line, const char* fmt, ...);

/* ========== Platform Abstraction ========== */

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

static int wsa_initialized = 0;

static void ensure_wsa_init(void) {
#ifdef _WIN32
    if (!wsa_initialized) {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
            runtime_error(0, "net: WSAStartup failed (error %d)", WSAGetLastError());
        wsa_initialized = 1;
    }
#else
    wsa_initialized = 1;
#endif
}

/* ========== Module Helpers ========== */

static void module_add_fn(Value* mod, const char* name, BuiltinFn fn, int min, int max) {
    Value* key = value_new_string(name);
    Value* fn_val = value_new_builtin(name, fn, min, max);
    dict_set(mod, key, fn_val);
}

/* ========== Socket Functions ========== */

/* net.connect(host, port) → socket id */
static Value* builtin_net_connect(Value** args, int argc) {
    if (args[0]->type != VAL_STRING)
        runtime_error(0, "net.connect(): host must be a string");
    if (args[1]->type != VAL_INT)
        runtime_error(0, "net.connect(): port must be an integer");

    ensure_wsa_init();

    const char* host = args[0]->as.string;
    int port = (int)args[1]->as.integer;

    /* Resolve hostname */
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);

    int status = getaddrinfo(host, port_str, &hints, &res);
    if (status != 0)
        runtime_error(0, "net.connect(): cannot resolve '%s' (error %d)", host, status);

    /* Create socket */
    socket_t sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock == SOCKET_INVALID) {
        freeaddrinfo(res);
        runtime_error(0, "net.connect(): socket creation failed (error %d)", SOCK_ERR);
    }

    /* Connect */
    if (connect(sock, res->ai_addr, (int)res->ai_addrlen) != 0) {
        int err = SOCK_ERR;
        CLOSE_SOCKET(sock);
        freeaddrinfo(res);
        runtime_error(0, "net.connect(): connection to '%s:%d' failed (error %d)", host, port, err);
    }

    freeaddrinfo(res);
    return value_new_int((long long)sock);
}

/* net.send(sock, data) → bytes sent */
static Value* builtin_net_send(Value** args, int argc) {
    if (args[0]->type != VAL_INT)
        runtime_error(0, "net.send(): first argument must be a socket id");
    if (args[1]->type != VAL_STRING)
        runtime_error(0, "net.send(): second argument must be a string");

    socket_t sock = (socket_t)args[0]->as.integer;
    const char* data = args[1]->as.string;
    int len = (int)strlen(data);

    int sent = send(sock, data, len, 0);
    if (sent < 0)
        runtime_error(0, "net.send(): failed (error %d)", SOCK_ERR);

    return value_new_int(sent);
}

/* net.recv(sock, max_bytes) → string */
static Value* builtin_net_recv(Value** args, int argc) {
    if (args[0]->type != VAL_INT)
        runtime_error(0, "net.recv(): first argument must be a socket id");
    if (args[1]->type != VAL_INT)
        runtime_error(0, "net.recv(): second argument must be max bytes");

    socket_t sock = (socket_t)args[0]->as.integer;
    int max_bytes = (int)args[1]->as.integer;

    if (max_bytes <= 0 || max_bytes > 1048576)
        runtime_error(0, "net.recv(): max_bytes must be 1–1048576");

    char* buf = (char*)malloc(max_bytes + 1);
    int received = recv(sock, buf, max_bytes, 0);

    if (received < 0) {
        free(buf);
        runtime_error(0, "net.recv(): failed (error %d)", SOCK_ERR);
    }

    buf[received] = '\0';
    return value_new_string_take(buf);
}

/* net.close(sock) → null */
static Value* builtin_net_close(Value** args, int argc) {
    if (args[0]->type != VAL_INT)
        runtime_error(0, "net.close(): argument must be a socket id");

    socket_t sock = (socket_t)args[0]->as.integer;
    CLOSE_SOCKET(sock);
    return value_new_null();
}

/* net.listen(host, port) → server socket id */
static Value* builtin_net_listen(Value** args, int argc) {
    if (args[0]->type != VAL_STRING)
        runtime_error(0, "net.listen(): host must be a string");
    if (args[1]->type != VAL_INT)
        runtime_error(0, "net.listen(): port must be an integer");

    ensure_wsa_init();

    const char* host = args[0]->as.string;
    int port = (int)args[1]->as.integer;

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);

    int status = getaddrinfo(host, port_str, &hints, &res);
    if (status != 0)
        runtime_error(0, "net.listen(): cannot resolve '%s' (error %d)", host, status);

    socket_t sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock == SOCKET_INVALID) {
        freeaddrinfo(res);
        runtime_error(0, "net.listen(): socket creation failed (error %d)", SOCK_ERR);
    }

    /* Allow address reuse */
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    if (bind(sock, res->ai_addr, (int)res->ai_addrlen) != 0) {
        int err = SOCK_ERR;
        CLOSE_SOCKET(sock);
        freeaddrinfo(res);
        runtime_error(0, "net.listen(): bind to '%s:%d' failed (error %d)", host, port, err);
    }

    freeaddrinfo(res);

    if (listen(sock, 5) != 0) {
        int err = SOCK_ERR;
        CLOSE_SOCKET(sock);
        runtime_error(0, "net.listen(): listen failed (error %d)", err);
    }

    return value_new_int((long long)sock);
}

/* net.accept(server_sock) → client socket id */
static Value* builtin_net_accept(Value** args, int argc) {
    if (args[0]->type != VAL_INT)
        runtime_error(0, "net.accept(): argument must be a socket id");

    socket_t server = (socket_t)args[0]->as.integer;
    struct sockaddr_in client_addr;
    int addr_len = sizeof(client_addr);

    socket_t client = accept(server, (struct sockaddr*)&client_addr,
#ifdef _WIN32
                             &addr_len
#else
                             (socklen_t*)&addr_len
#endif
                             );

    if (client == SOCKET_INVALID)
        runtime_error(0, "net.accept(): failed (error %d)", SOCK_ERR);

    return value_new_int((long long)client);
}

/* ========== Module Creation ========== */

Value* create_net_module(void) {
    static Value* cached = NULL;
    if (cached) return cached;

    Value* mod = value_new_dict();
    module_add_fn(mod, "connect", builtin_net_connect, 2, 2);
    module_add_fn(mod, "send",    builtin_net_send,    2, 2);
    module_add_fn(mod, "recv",    builtin_net_recv,    2, 2);
    module_add_fn(mod, "close",   builtin_net_close,   1, 1);
    module_add_fn(mod, "listen",  builtin_net_listen,  2, 2);
    module_add_fn(mod, "accept",  builtin_net_accept,  1, 1);

    cached = mod;
    return mod;
}
