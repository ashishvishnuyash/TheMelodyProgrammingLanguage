#include "crypto_module.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

void runtime_error(int line, const char* fmt, ...);

static void module_add_fn(Value* mod, const char* name, BuiltinFn fn, int min, int max) {
    Value* key = value_new_string(name);
    Value* fn_val = value_new_builtin(name, fn, min, max);
    dict_set(mod, key, fn_val);
}

static char* bytes_to_hex(const uint8_t* data, int len) {
    char* hex = (char*)malloc(len * 2 + 1);
    for (int i = 0; i < len; i++) sprintf(hex + i * 2, "%02x", data[i]);
    hex[len * 2] = '\0';
    return hex;
}

/* ========== MD5 (RFC 1321) ========== */

static const uint32_t md5_T[64] = {
    0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501,
    0x698098d8,0x8b44f7af,0xffff5bb1,0x895cd7be,0x6b901122,0xfd987193,0xa679438e,0x49b40821,
    0xf61e2562,0xc040b340,0x265e5a51,0xe9b6c7aa,0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8,
    0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,0xa9e3e905,0xfcefa3f8,0x676f02d9,0x8d2a4c8a,
    0xfffa3942,0x8771f681,0x6d9d6122,0xfde5380c,0xa4beea44,0x4bdecfa9,0xf6bb4b60,0xbebfbc70,
    0x289b7ec6,0xeaa127fa,0xd4ef3085,0x04881d05,0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,
    0xf4292244,0x432aff97,0xab9423a7,0xfc93a039,0x655b59c3,0x8f0ccc92,0xffeff47d,0x85845dd1,
    0x6fa87e4f,0xfe2ce6e0,0xa3014314,0x4e0811a1,0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391,
};
static const int md5_s[64] = {
    7,12,17,22,7,12,17,22,7,12,17,22,7,12,17,22,
    5,9,14,20,5,9,14,20,5,9,14,20,5,9,14,20,
    4,11,16,23,4,11,16,23,4,11,16,23,4,11,16,23,
    6,10,15,21,6,10,15,21,6,10,15,21,6,10,15,21,
};
static uint32_t md5_left_rotate(uint32_t x, int c) { return (x << c) | (x >> (32 - c)); }

static void md5_hash(const uint8_t* msg, size_t len, uint8_t out[16]) {
    uint32_t a0=0x67452301, b0=0xefcdab89, c0=0x98badcfe, d0=0x10325476;
    size_t new_len = len + 1;
    while (new_len % 64 != 56) new_len++;
    uint8_t* buf = (uint8_t*)calloc(new_len + 8, 1);
    memcpy(buf, msg, len);
    buf[len] = 0x80;
    uint64_t bit_len = (uint64_t)len * 8;
    memcpy(buf + new_len, &bit_len, 8);
    for (size_t offset = 0; offset < new_len + 8; offset += 64) {
        uint32_t* M = (uint32_t*)(buf + offset);
        uint32_t A=a0, B=b0, C=c0, D=d0;
        for (int i = 0; i < 64; i++) {
            uint32_t F; int g;
            if (i < 16)      { F = (B & C) | (~B & D); g = i; }
            else if (i < 32) { F = (D & B) | (~D & C); g = (5*i+1) % 16; }
            else if (i < 48) { F = B ^ C ^ D;          g = (3*i+5) % 16; }
            else              { F = C ^ (B | ~D);       g = (7*i) % 16; }
            F = F + A + md5_T[i] + M[g];
            A = D; D = C; C = B; B = B + md5_left_rotate(F, md5_s[i]);
        }
        a0 += A; b0 += B; c0 += C; d0 += D;
    }
    free(buf);
    memcpy(out, &a0, 4); memcpy(out+4, &b0, 4); memcpy(out+8, &c0, 4); memcpy(out+12, &d0, 4);
}

/* ========== SHA-256 ========== */

static const uint32_t sha256_k[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2,
};
#define SHA256_RR(x,n) (((x)>>(n))|((x)<<(32-(n))))
#define SHA256_CH(x,y,z) (((x)&(y))^(~(x)&(z)))
#define SHA256_MAJ(x,y,z) (((x)&(y))^((x)&(z))^((y)&(z)))
#define SHA256_EP0(x) (SHA256_RR(x,2)^SHA256_RR(x,13)^SHA256_RR(x,22))
#define SHA256_EP1(x) (SHA256_RR(x,6)^SHA256_RR(x,11)^SHA256_RR(x,25))
#define SHA256_SIG0(x) (SHA256_RR(x,7)^SHA256_RR(x,18)^((x)>>3))
#define SHA256_SIG1(x) (SHA256_RR(x,17)^SHA256_RR(x,19)^((x)>>10))

static void sha256_hash(const uint8_t* msg, size_t len, uint8_t out[32]) {
    uint32_t h[8] = {0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    size_t new_len = len + 1;
    while (new_len % 64 != 56) new_len++;
    uint8_t* buf = (uint8_t*)calloc(new_len + 8, 1);
    memcpy(buf, msg, len); buf[len] = 0x80;
    uint64_t bit_len = (uint64_t)len * 8;
    for (int i = 0; i < 8; i++) buf[new_len + i] = (uint8_t)(bit_len >> (56 - i * 8));
    for (size_t offset = 0; offset < new_len + 8; offset += 64) {
        uint32_t w[64];
        for (int i = 0; i < 16; i++)
            w[i] = ((uint32_t)buf[offset+i*4]<<24)|((uint32_t)buf[offset+i*4+1]<<16)|
                   ((uint32_t)buf[offset+i*4+2]<<8)|buf[offset+i*4+3];
        for (int i = 16; i < 64; i++)
            w[i] = SHA256_SIG1(w[i-2]) + w[i-7] + SHA256_SIG0(w[i-15]) + w[i-16];
        uint32_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],hh=h[7];
        for (int i = 0; i < 64; i++) {
            uint32_t t1 = hh + SHA256_EP1(e) + SHA256_CH(e,f,g) + sha256_k[i] + w[i];
            uint32_t t2 = SHA256_EP0(a) + SHA256_MAJ(a,b,c);
            hh=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
        }
        h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=hh;
    }
    free(buf);
    for (int i = 0; i < 8; i++) {
        out[i*4]=(uint8_t)(h[i]>>24);out[i*4+1]=(uint8_t)(h[i]>>16);
        out[i*4+2]=(uint8_t)(h[i]>>8);out[i*4+3]=(uint8_t)h[i];
    }
}

/* ========== SHA-512 ========== */

static const uint64_t sha512_k[80] = {
    0x428a2f98d728ae22ULL,0x7137449123ef65cdULL,0xb5c0fbcfec4d3b2fULL,0xe9b5dba58189dbbcULL,
    0x3956c25bf348b538ULL,0x59f111f1b605d019ULL,0x923f82a4af194f9bULL,0xab1c5ed5da6d8118ULL,
    0xd807aa98a3030242ULL,0x12835b0145706fbeULL,0x243185be4ee4b28cULL,0x550c7dc3d5ffb4e2ULL,
    0x72be5d74f27b896fULL,0x80deb1fe3b1696b1ULL,0x9bdc06a725c71235ULL,0xc19bf174cf692694ULL,
    0xe49b69c19ef14ad2ULL,0xefbe4786384f25e3ULL,0x0fc19dc68b8cd5b5ULL,0x240ca1cc77ac9c65ULL,
    0x2de92c6f592b0275ULL,0x4a7484aa6ea6e483ULL,0x5cb0a9dcbd41fbd4ULL,0x76f988da831153b5ULL,
    0x983e5152ee66dfabULL,0xa831c66d2db43210ULL,0xb00327c898fb213fULL,0xbf597fc7beef0ee4ULL,
    0xc6e00bf33da88fc2ULL,0xd5a79147930aa725ULL,0x06ca6351e003826fULL,0x142929670a0e6e70ULL,
    0x27b70a8546d22ffcULL,0x2e1b21385c26c926ULL,0x4d2c6dfc5ac42aedULL,0x53380d139d95b3dfULL,
    0x650a73548baf63deULL,0x766a0abb3c77b2a8ULL,0x81c2c92e47edaee6ULL,0x92722c851482353bULL,
    0xa2bfe8a14cf10364ULL,0xa81a664bbc423001ULL,0xc24b8b70d0f89791ULL,0xc76c51a30654be30ULL,
    0xd192e819d6ef5218ULL,0xd69906245565a910ULL,0xf40e35855771202aULL,0x106aa07032bbd1b8ULL,
    0x19a4c116b8d2d0c8ULL,0x1e376c085141ab53ULL,0x2748774cdf8eeb99ULL,0x34b0bcb5e19b48a8ULL,
    0x391c0cb3c5c95a63ULL,0x4ed8aa4ae3418acbULL,0x5b9cca4f7763e373ULL,0x682e6ff3d6b2b8a3ULL,
    0x748f82ee5defb2fcULL,0x78a5636f43172f60ULL,0x84c87814a1f0ab72ULL,0x8cc702081a6439ecULL,
    0x90befffa23631e28ULL,0xa4506cebde82bde9ULL,0xbef9a3f7b2c67915ULL,0xc67178f2e372532bULL,
    0xca273eceea26619cULL,0xd186b8c721c0c207ULL,0xeada7dd6cde0eb1eULL,0xf57d4f7fee6ed178ULL,
    0x06f067aa72176fbaULL,0x0a637dc5a2c898a6ULL,0x113f9804bef90daeULL,0x1b710b35131c471bULL,
    0x28db77f523047d84ULL,0x32caab7b40c72493ULL,0x3c9ebe0a15c9bebcULL,0x431d67c49c100d4cULL,
    0x4cc5d4becb3e42b6ULL,0x597f299cfc657e2aULL,0x5fcb6fab3ad6faecULL,0x6c44198c4a475817ULL,
};
#define SHA512_RR(x,n) (((x)>>(n))|((x)<<(64-(n))))
#define SHA512_CH(x,y,z) (((x)&(y))^(~(x)&(z)))
#define SHA512_MAJ(x,y,z) (((x)&(y))^((x)&(z))^((y)&(z)))
#define SHA512_EP0(x) (SHA512_RR(x,28)^SHA512_RR(x,34)^SHA512_RR(x,39))
#define SHA512_EP1(x) (SHA512_RR(x,14)^SHA512_RR(x,18)^SHA512_RR(x,41))
#define SHA512_SIG0(x) (SHA512_RR(x,1)^SHA512_RR(x,8)^((x)>>7))
#define SHA512_SIG1(x) (SHA512_RR(x,19)^SHA512_RR(x,61)^((x)>>6))

static void sha512_hash(const uint8_t* msg, size_t len, uint8_t out[64]) {
    uint64_t h[8] = {
        0x6a09e667f3bcc908ULL,0xbb67ae8584caa73bULL,0x3c6ef372fe94f82bULL,0xa54ff53a5f1d36f1ULL,
        0x510e527fade682d1ULL,0x9b05688c2b3e6c1fULL,0x1f83d9abfb41bd6bULL,0x5be0cd19137e2179ULL,
    };
    size_t new_len = len + 1;
    while (new_len % 128 != 112) new_len++;
    uint8_t* buf = (uint8_t*)calloc(new_len + 16, 1);
    memcpy(buf, msg, len); buf[len] = 0x80;
    uint64_t bit_len = (uint64_t)len * 8;
    for (int i = 0; i < 8; i++) buf[new_len + 8 + i] = (uint8_t)(bit_len >> (56 - i * 8));
    for (size_t offset = 0; offset < new_len + 16; offset += 128) {
        uint64_t w[80];
        for (int i = 0; i < 16; i++) {
            w[i] = 0;
            for (int j = 0; j < 8; j++) w[i] |= (uint64_t)buf[offset+i*8+j] << (56-j*8);
        }
        for (int i = 16; i < 80; i++)
            w[i] = SHA512_SIG1(w[i-2]) + w[i-7] + SHA512_SIG0(w[i-15]) + w[i-16];
        uint64_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],hh=h[7];
        for (int i = 0; i < 80; i++) {
            uint64_t t1 = hh + SHA512_EP1(e) + SHA512_CH(e,f,g) + sha512_k[i] + w[i];
            uint64_t t2 = SHA512_EP0(a) + SHA512_MAJ(a,b,c);
            hh=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;
        }
        h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=hh;
    }
    free(buf);
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++) out[i*8+j] = (uint8_t)(h[i] >> (56-j*8));
}

/* ========== HMAC-SHA256 ========== */

static void hmac_sha256(const uint8_t* key, size_t key_len, const uint8_t* msg, size_t msg_len, uint8_t out[32]) {
    uint8_t k_pad[64];
    memset(k_pad, 0, 64);
    if (key_len > 64) sha256_hash(key, key_len, k_pad);
    else memcpy(k_pad, key, key_len);
    uint8_t* inner = (uint8_t*)malloc(64 + msg_len);
    for (int i = 0; i < 64; i++) inner[i] = k_pad[i] ^ 0x36;
    memcpy(inner + 64, msg, msg_len);
    uint8_t inner_hash[32];
    sha256_hash(inner, 64 + msg_len, inner_hash);
    free(inner);
    uint8_t outer[96];
    for (int i = 0; i < 64; i++) outer[i] = k_pad[i] ^ 0x5c;
    memcpy(outer + 64, inner_hash, 32);
    sha256_hash(outer, 96, out);
}

/* ========== Base64 ========== */

static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static char* base64_encode(const uint8_t* data, size_t len) {
    size_t out_len = 4 * ((len + 2) / 3);
    char* out = (char*)malloc(out_len + 1);
    int j = 0;
    for (size_t i = 0; i < len; i += 3) {
        uint32_t n = ((uint32_t)data[i]) << 16;
        if (i+1 < len) n |= ((uint32_t)data[i+1]) << 8;
        if (i+2 < len) n |= data[i+2];
        out[j++] = b64_table[(n>>18)&0x3F];
        out[j++] = b64_table[(n>>12)&0x3F];
        out[j++] = (i+1 < len) ? b64_table[(n>>6)&0x3F] : '=';
        out[j++] = (i+2 < len) ? b64_table[n&0x3F] : '=';
    }
    out[j] = '\0';
    return out;
}

static int b64_decode_char(char c) {
    if (c>='A'&&c<='Z') return c-'A';
    if (c>='a'&&c<='z') return c-'a'+26;
    if (c>='0'&&c<='9') return c-'0'+52;
    if (c=='+') return 62; if (c=='/') return 63;
    return -1;
}

static uint8_t* base64_decode(const char* str, size_t* out_len) {
    size_t len = strlen(str);
    if (len % 4 != 0) { *out_len = 0; return (uint8_t*)calloc(1,1); }
    *out_len = len/4*3;
    if (len>0 && str[len-1]=='=') (*out_len)--;
    if (len>1 && str[len-2]=='=') (*out_len)--;
    uint8_t* out = (uint8_t*)malloc(*out_len + 1);
    int j = 0;
    for (size_t i = 0; i < len; i += 4) {
        int a=b64_decode_char(str[i]),b=b64_decode_char(str[i+1]);
        int c=(str[i+2]!='=')?b64_decode_char(str[i+2]):0;
        int d=(str[i+3]!='=')?b64_decode_char(str[i+3]):0;
        uint32_t n = (a<<18)|(b<<12)|(c<<6)|d;
        if (j<(int)*out_len) out[j++]=(uint8_t)(n>>16);
        if (j<(int)*out_len) out[j++]=(uint8_t)(n>>8);
        if (j<(int)*out_len) out[j++]=(uint8_t)n;
    }
    out[*out_len] = '\0';
    return out;
}

/* ========== Melody bindings ========== */

static Value* builtin_crypto_md5(Value** args, int argc) {
    if (args[0]->type != VAL_STRING) runtime_error(0, "crypto.md5(): argument must be a string");
    uint8_t hash[16];
    md5_hash((const uint8_t*)args[0]->as.string, strlen(args[0]->as.string), hash);
    return value_new_string_take(bytes_to_hex(hash, 16));
}

static Value* builtin_crypto_sha256(Value** args, int argc) {
    if (args[0]->type != VAL_STRING) runtime_error(0, "crypto.sha256(): argument must be a string");
    uint8_t hash[32];
    sha256_hash((const uint8_t*)args[0]->as.string, strlen(args[0]->as.string), hash);
    return value_new_string_take(bytes_to_hex(hash, 32));
}

static Value* builtin_crypto_sha512(Value** args, int argc) {
    if (args[0]->type != VAL_STRING) runtime_error(0, "crypto.sha512(): argument must be a string");
    uint8_t hash[64];
    sha512_hash((const uint8_t*)args[0]->as.string, strlen(args[0]->as.string), hash);
    return value_new_string_take(bytes_to_hex(hash, 64));
}

static Value* builtin_crypto_hmac_sha256(Value** args, int argc) {
    if (args[0]->type != VAL_STRING || args[1]->type != VAL_STRING)
        runtime_error(0, "crypto.hmac_sha256(): both arguments must be strings");
    uint8_t hash[32];
    hmac_sha256((const uint8_t*)args[0]->as.string, strlen(args[0]->as.string),
                (const uint8_t*)args[1]->as.string, strlen(args[1]->as.string), hash);
    return value_new_string_take(bytes_to_hex(hash, 32));
}

static Value* builtin_crypto_base64_encode(Value** args, int argc) {
    if (args[0]->type != VAL_STRING) runtime_error(0, "crypto.base64_encode(): argument must be a string");
    return value_new_string_take(base64_encode((const uint8_t*)args[0]->as.string, strlen(args[0]->as.string)));
}

static Value* builtin_crypto_base64_decode(Value** args, int argc) {
    if (args[0]->type != VAL_STRING) runtime_error(0, "crypto.base64_decode(): argument must be a string");
    size_t out_len;
    uint8_t* decoded = base64_decode(args[0]->as.string, &out_len);
    return value_new_string_take((char*)decoded);
}

Value* create_crypto_module(void) {
    static Value* cached = NULL;
    if (cached) return cached;
    Value* mod = value_new_dict();
    module_add_fn(mod, "md5",           builtin_crypto_md5,           1, 1);
    module_add_fn(mod, "sha256",        builtin_crypto_sha256,        1, 1);
    module_add_fn(mod, "sha512",        builtin_crypto_sha512,        1, 1);
    module_add_fn(mod, "hmac_sha256",   builtin_crypto_hmac_sha256,   2, 2);
    module_add_fn(mod, "base64_encode", builtin_crypto_base64_encode, 1, 1);
    module_add_fn(mod, "base64_decode", builtin_crypto_base64_decode, 1, 1);
    cached = mod;
    return mod;
}
