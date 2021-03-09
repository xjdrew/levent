/*
** $Id: lcrypto.c,v 1.2 2006/08/25 03:24:17 nezroy Exp $
** See Copyright Notice in license.html
*/

#include <openssl/bio.h>
#include <openssl/dsa.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <stddef.h>
#include <string.h>

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifdef __GNUC__
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif

#include <lauxlib.h>
#include <lua.h>

#include "lcrypto.h"
#include "lcrypto_compat_110.h"

static int crypto_error(lua_State* L)
{
    char buf[120];
    unsigned long e = ERR_get_error();
    ERR_load_crypto_strings();
    lua_pushnil(L);
    lua_pushstring(L, ERR_error_string(e, buf));
    return 2;
}

/*************** DIGEST API ***************/
typedef struct digest_context {
    EVP_MD_CTX* ctx;
} digest_context;

static digest_context* digest_pnew(lua_State* L)
{
    digest_context* c = (digest_context*)lua_newuserdata(L, sizeof(digest_context));
    c->ctx = EVP_MD_CTX_new();

    luaL_getmetatable(L, LUACRYPTO_DIGESTNAME);
    lua_setmetatable(L, -2);
    return c;
}

static int digest_fnew(lua_State* L)
{
    const char* s = luaL_checkstring(L, 1);
    const EVP_MD* digest = EVP_get_digestbyname(s);
    digest_context* c;

    if (digest == NULL)
        return luaL_argerror(L, 1, "invalid digest/cipher type");

    c = digest_pnew(L);
    if (EVP_DigestInit_ex(c->ctx, digest, NULL) != 1)
        return crypto_error(L);

    return 1;
}

static int digest_clone(lua_State* L)
{
    digest_context* c = (digest_context*)luaL_checkudata(L, 1, LUACRYPTO_DIGESTNAME);
    digest_context* d = digest_pnew(L);
    if (!EVP_MD_CTX_copy_ex(d->ctx, c->ctx)) {
        return crypto_error(L);
    }
    return 1;
}

static int digest_reset(lua_State* L)
{
    digest_context* c = (digest_context*)luaL_checkudata(L, 1, LUACRYPTO_DIGESTNAME);
    const EVP_MD* t = EVP_MD_CTX_md(c->ctx);
    if (!EVP_MD_CTX_reset(c->ctx)) {
        return crypto_error(L);
    }
    if (!EVP_DigestInit_ex(c->ctx, t, NULL)) {
        return crypto_error(L);
    }
    return 0;
}

static int digest_update(lua_State* L)
{
    digest_context* c = (digest_context*)luaL_checkudata(L, 1, LUACRYPTO_DIGESTNAME);
    size_t sl;
    const char* s = luaL_checklstring(L, 2, &sl);

    if (!EVP_DigestUpdate(c->ctx, s, sl)) {
        return crypto_error(L);
    }

    lua_settop(L, 1);
    return 1;
}

static int digest_final(lua_State* L)
{
    digest_context* c = (digest_context*)luaL_checkudata(L, 1, LUACRYPTO_DIGESTNAME);
    EVP_MD_CTX* d = NULL;
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int written = 0;
    unsigned int i;
    char* hex;
    size_t sl;
    const char* s;

    if (lua_isstring(L, 2)) {
        s = luaL_checklstring(L, 2, &sl);
        if (!EVP_DigestUpdate(c->ctx, s, sl)) {
            return crypto_error(L);
        }
    }

    d = EVP_MD_CTX_create();
    if (!EVP_MD_CTX_copy_ex(d, c->ctx)) {
        return crypto_error(L);
    }
    if (!EVP_DigestFinal_ex(d, digest, &written)) {
        return crypto_error(L);
    }
    EVP_MD_CTX_destroy(d);

    if (lua_toboolean(L, 3))
        lua_pushlstring(L, (char*)digest, written);
    else {
        hex = (char*)calloc(sizeof(char), written * 2 + 1);
        for (i = 0; i < written; i++)
            sprintf(hex + 2 * i, "%02x", digest[i]);
        lua_pushlstring(L, hex, written * 2);
        free(hex);
    }

    return 1;
}

static int digest_tostring(lua_State* L)
{
    digest_context* c = (digest_context*)luaL_checkudata(L, 1, LUACRYPTO_DIGESTNAME);
    char s[64];
    sprintf(s, "%s %p", LUACRYPTO_DIGESTNAME, (void*)c);
    lua_pushstring(L, s);
    return 1;
}

static int digest_gc(lua_State* L)
{
    digest_context* c = (digest_context*)luaL_checkudata(L, 1, LUACRYPTO_DIGESTNAME);
    EVP_MD_CTX_free(c->ctx);
    return 0;
}

static int digest_fdigest(lua_State* L)
{
    const char* type_name = luaL_checkstring(L, 2);
    const EVP_MD* type = EVP_get_digestbyname(type_name);
    size_t sl;
    const char* s = luaL_checklstring(L, 3, &sl);
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int written = 0;
    EVP_MD_CTX* c;

    if (type == NULL) {
        luaL_argerror(L, 1, "invalid digest type");
        return 0;
    }

    c = EVP_MD_CTX_create();
    if (!EVP_DigestInit_ex(c, type, NULL)) {
        EVP_MD_CTX_destroy(c);
        return crypto_error(L);
    }
    if (!EVP_DigestUpdate(c, s, sl)) {
        EVP_MD_CTX_destroy(c);
        return crypto_error(L);
    }
    if (!EVP_DigestFinal_ex(c, digest, &written)) {
        EVP_MD_CTX_destroy(c);
        return crypto_error(L);
    }
    EVP_MD_CTX_destroy(c);

    if (lua_toboolean(L, 4))
        lua_pushlstring(L, (char*)digest, written);
    else {
        char* hex = (char*)calloc(sizeof(char), written * 2 + 1);
        unsigned int i;
        for (i = 0; i < written; i++)
            sprintf(hex + 2 * i, "%02x", digest[i]);
        lua_pushlstring(L, hex, written * 2);
        free(hex);
    }

    return 1;
}

/*************** ENCRYPT API ***************/
typedef struct cipher_context {
    EVP_CIPHER_CTX* ctx;
} cipher_context;

static int parse_enc_params(lua_State* L, EVP_CIPHER** cipher, char** key, size_t* key_len,
    char** iv, size_t* iv_len, int* pad, int* size_to_return,
    int cipher_pos, int key_pos, int iv_pos, int pad_pos)
{
    const char* s = luaL_checkstring(L, cipher_pos);
    *cipher = (EVP_CIPHER*)EVP_get_cipherbyname(s);
    *size_to_return = 0;

    if (*cipher == NULL) {
        return luaL_argerror(L, cipher_pos, "invalid encrypt cipher");
    }

    *key_len = 0;
    *key = (char*)luaL_checklstring(L, key_pos, key_len);
    if (*key_len > EVP_MAX_KEY_LENGTH) {
        return luaL_argerror(L, key_pos, "invalid encrypt/decrypt key");
    }

    *iv_len = 0;
    *iv = (char*)lua_tolstring(L, iv_pos, iv_len); /* can be NULL */
    if (*iv && (*iv_len > (size_t)EVP_CIPHER_iv_length(*cipher))) {
        return luaL_argerror(L, iv_pos, "invalid iv length");
    }

    *pad = (lua_gettop(L) < pad_pos || lua_toboolean(L, pad_pos));

    return 1;
}

static int parse_new_enc_params(lua_State* L, EVP_CIPHER** cipher, char** key, size_t* key_len,
    char** iv, size_t* iv_len, int* pad, int* size_to_return)
{
    return parse_enc_params(L, cipher, key, key_len, iv, iv_len, pad, size_to_return,
        1, 2, 3, 4);
}

static int parse_f_enc_params(lua_State* L, EVP_CIPHER** cipher, char** key, size_t* key_len,
    char** iv, size_t* iv_len, int* pad, int* size_to_return)
{
    return parse_enc_params(L, cipher, key, key_len, iv, iv_len, pad, size_to_return,
        2, 4, 5, 6);
}

#define TRY_CTX(fun)                       \
    if (!fun) {                            \
        *size_to_return = crypto_error(L); \
        return 0;                          \
    }

static int init_encryptor_decryptor(int (*init_fun)(EVP_CIPHER_CTX*, const EVP_CIPHER*, ENGINE*, const unsigned char*, const unsigned char*),
    lua_State* L, EVP_CIPHER_CTX* c, const EVP_CIPHER* cipher, const char* key, size_t key_len,
    const char* iv, size_t iv_len, int pad, int* size_to_return)
{
    unsigned char the_key[EVP_MAX_KEY_LENGTH] = { 0 };
    unsigned char the_iv[EVP_MAX_IV_LENGTH] = { 0 };

    TRY_CTX(init_fun(c, cipher, NULL, NULL, NULL))

    if (!pad)
        TRY_CTX(EVP_CIPHER_CTX_set_padding(c, 0))

    if (iv)
        memcpy(the_iv, iv, iv_len);

    memcpy(the_key, key, key_len);
    TRY_CTX(init_fun(c, NULL, NULL, the_key, the_iv))

    return 1;
}

static cipher_context* encrypt_pnew(lua_State* L)
{
    cipher_context* c = (cipher_context*)lua_newuserdata(L, sizeof(cipher_context));
    c->ctx = EVP_CIPHER_CTX_new();

    luaL_getmetatable(L, LUACRYPTO_ENCRYPTNAME);
    lua_setmetatable(L, -2);

    return c;
}

static int encrypt_fnew(lua_State* L)
{
    const char *key = 0, *iv = 0;
    const EVP_CIPHER* cipher = 0;

    size_t key_len = 0, iv_len = 0;
    int pad = 1, size_to_return = 0;

    cipher_context* c;

    if (!parse_new_enc_params(L, (EVP_CIPHER**)&cipher, (char**)&key, &key_len,
            (char**)&iv, &iv_len, &pad, &size_to_return)) {
        return size_to_return;
    }

    c = encrypt_pnew(L);
    if (!init_encryptor_decryptor(EVP_EncryptInit_ex, L, c->ctx, cipher, key, key_len, iv, iv_len, pad, &size_to_return)) {
        return size_to_return;
    }
    return 1;
}

static int encrypt_update(lua_State* L)
{
    cipher_context* c = (cipher_context*)luaL_checkudata(L, 1, LUACRYPTO_ENCRYPTNAME);
    size_t input_len = 0;
    const unsigned char* input = (unsigned char*)luaL_checklstring(L, 2, &input_len);
    int output_len = 0;
    unsigned char* buffer = NULL;

    buffer = (unsigned char*)malloc(input_len + (size_t)EVP_CIPHER_CTX_block_size(c->ctx));
    if (!EVP_EncryptUpdate(c->ctx, buffer, &output_len, input, (int)input_len)) {
        free(buffer);
        return crypto_error(L);
    }
    lua_pushlstring(L, (char*)buffer, (size_t)output_len);
    free(buffer);

    return 1;
}

static int encrypt_final(lua_State* L)
{
    cipher_context* c = (cipher_context*)luaL_checkudata(L, 1, LUACRYPTO_ENCRYPTNAME);
    int output_len = 0;
    unsigned char buffer[EVP_MAX_BLOCK_LENGTH];

    if (!EVP_EncryptFinal_ex(c->ctx, buffer, &output_len)) {
        return crypto_error(L);
    }
    lua_pushlstring(L, (char*)buffer, (size_t)output_len);
    return 1;
}

static int encrypt_tostring(lua_State* L)
{
    cipher_context* c = (cipher_context*)luaL_checkudata(L, 1, LUACRYPTO_ENCRYPTNAME);
    char s[64];
    sprintf(s, "%s %p", LUACRYPTO_ENCRYPTNAME, (void*)c);
    lua_pushstring(L, s);
    return 1;
}

static int encrypt_gc(lua_State* L)
{
    cipher_context* c = (cipher_context*)luaL_checkudata(L, 1, LUACRYPTO_ENCRYPTNAME);
    EVP_CIPHER_CTX_free(c->ctx);
    return 0;
}

static int encrypt_fencrypt(lua_State* L)
{
    /* parameter 1 is the 'crypto.encrypt' table */
    const EVP_CIPHER* type;

    size_t input_len = 0;
    const unsigned char* input = (unsigned char*)luaL_checklstring(L, 3, &input_len);

    size_t key_len = 0, iv_len = 0;
    const char *key = NULL, *iv = NULL;
    int pad = 0, size_to_return = 0;

    int output_len = 0;
    int len = 0;

    unsigned char* buffer;

    EVP_CIPHER_CTX* c = EVP_CIPHER_CTX_new();

    if (!parse_f_enc_params(L, (EVP_CIPHER**)&type, (char**)&key, &key_len, (char**)&iv, &iv_len, &pad, &size_to_return)) {
        EVP_CIPHER_CTX_free(c);
        return size_to_return;
    }

    if (!init_encryptor_decryptor(EVP_EncryptInit_ex, L, c, type, key, key_len, iv, iv_len, pad, &size_to_return)) {
        EVP_CIPHER_CTX_free(c);
        return size_to_return;
    }

    buffer = (unsigned char*)malloc(input_len + (size_t)EVP_CIPHER_CTX_block_size(c));

    if (!EVP_EncryptUpdate(c, buffer, &len, input, (int)input_len)) {
        EVP_CIPHER_CTX_free(c);
        free(buffer);
        return crypto_error(L);
    }
    output_len += len;
    if (!EVP_EncryptFinal_ex(c, &buffer[output_len], &len)) {
        EVP_CIPHER_CTX_free(c);
        free(buffer);
        return crypto_error(L);
    }
    output_len += len;

    lua_pushlstring(L, (char*)buffer, (size_t)output_len);
    free(buffer);
    EVP_CIPHER_CTX_free(c);
    return 1;
}

/*************** DECRYPT API ***************/

static cipher_context* decrypt_pnew(lua_State* L)
{
    cipher_context* c = (cipher_context*)lua_newuserdata(L, sizeof(cipher_context));
    c->ctx = EVP_CIPHER_CTX_new();

    luaL_getmetatable(L, LUACRYPTO_DECRYPTNAME);
    lua_setmetatable(L, -2);

    return c;
}

static int decrypt_fnew(lua_State* L)
{
    const char *key = 0, *iv = 0;
    const EVP_CIPHER* cipher = 0;

    size_t key_len = 0, iv_len = 0;
    int pad = 1, size_to_return = 0;
    cipher_context* c;

    if (!parse_new_enc_params(L, (EVP_CIPHER**)&cipher, (char**)&key, &key_len,
            (char**)&iv, &iv_len, &pad, &size_to_return)) {
        return size_to_return;
    }

    c = decrypt_pnew(L);
    if (!init_encryptor_decryptor(EVP_DecryptInit_ex, L, c->ctx, cipher, key, key_len, iv, iv_len, pad, &size_to_return)) {
        return size_to_return;
    }
    return 1;
}

static int decrypt_update(lua_State* L)
{
    cipher_context* c = (cipher_context*)luaL_checkudata(L, 1, LUACRYPTO_DECRYPTNAME);
    size_t input_len = 0;
    const unsigned char* input = (unsigned char*)luaL_checklstring(L, 2, &input_len);
    int output_len = 0;
    unsigned char* buffer = NULL;

    buffer = (unsigned char*)malloc(input_len + (size_t)EVP_CIPHER_CTX_block_size(c->ctx));
    if (!EVP_DecryptUpdate(c->ctx, buffer, &output_len, input, (int)input_len)) {
        return crypto_error(L);
    }
    lua_pushlstring(L, (char*)buffer, (size_t)output_len);
    free(buffer);

    return 1;
}

static int decrypt_final(lua_State* L)
{
    cipher_context* c = (cipher_context*)luaL_checkudata(L, 1, LUACRYPTO_DECRYPTNAME);
    int output_len = 0;
    unsigned char buffer[EVP_MAX_BLOCK_LENGTH];

    if (!EVP_DecryptFinal_ex(c->ctx, buffer, &output_len)) {
        return crypto_error(L);
    }
    lua_pushlstring(L, (char*)buffer, (size_t)output_len);
    return 1;
}

static int decrypt_tostring(lua_State* L)
{
    cipher_context* c = (cipher_context*)luaL_checkudata(L, 1, LUACRYPTO_DECRYPTNAME);
    char s[64];
    sprintf(s, "%s %p", LUACRYPTO_DECRYPTNAME, (void*)c);
    lua_pushstring(L, s);
    return 1;
}

static int decrypt_gc(lua_State* L)
{
    cipher_context* c = (cipher_context*)luaL_checkudata(L, 1, LUACRYPTO_DECRYPTNAME);
    EVP_CIPHER_CTX_free(c->ctx);
    return 1;
}

static int decrypt_fdecrypt(lua_State* L)
{
    /* parameter 1 is the 'crypto.decrypt' table */
    size_t input_len = 0;
    const unsigned char* input = (unsigned char*)luaL_checklstring(L, 3, &input_len);

    const EVP_CIPHER* type;
    size_t key_len = 0, iv_len = 0;
    const char *key = NULL, *iv = NULL;
    int pad = 0, size_to_return = 0;
    unsigned char* buffer;
    int output_len = 0;
    int len = 0;

    EVP_CIPHER_CTX* c = EVP_CIPHER_CTX_new();

    if (!parse_f_enc_params(L, (EVP_CIPHER**)&type, (char**)&key, &key_len, (char**)&iv, &iv_len, &pad, &size_to_return)) {
        EVP_CIPHER_CTX_free(c);
        return size_to_return;
    }

    if (!init_encryptor_decryptor(EVP_DecryptInit_ex, L, c, type, key, key_len, iv, iv_len, pad, &size_to_return)) {
        EVP_CIPHER_CTX_free(c);
        return size_to_return;
    }

    buffer = (unsigned char*)malloc(input_len + (size_t)EVP_CIPHER_CTX_block_size(c));
    if (!EVP_DecryptUpdate(c, buffer, &len, input, (int)input_len)) {
        EVP_CIPHER_CTX_free(c);
        free(buffer);
        return crypto_error(L);
    }
    output_len += len;
    if (!EVP_DecryptFinal_ex(c, &buffer[len], &len)) {
        EVP_CIPHER_CTX_free(c);
        free(buffer);
        return crypto_error(L);
    }
    output_len += len;

    lua_pushlstring(L, (char*)buffer, (size_t)output_len);
    free(buffer);
    EVP_CIPHER_CTX_free(c);
    return 1;
}

/*************** HMAC API ***************/
typedef struct hmac_context {
    HMAC_CTX* ctx;
} hmac_context;

static hmac_context* hmac_pnew(lua_State* L)
{
    hmac_context* c = (hmac_context*)lua_newuserdata(L, sizeof(hmac_context));
    c->ctx = HMAC_CTX_new();

    luaL_getmetatable(L, LUACRYPTO_HMACNAME);
    lua_setmetatable(L, -2);

    return c;
}

static int hmac_fnew(lua_State* L)
{
    hmac_context* c = hmac_pnew(L);

    size_t k_len;
    const char* s = luaL_checkstring(L, 1);
    const char* k = luaL_checklstring(L, 2, &k_len);
    const EVP_MD* type = EVP_get_digestbyname(s);

    if (type == NULL)
        return luaL_argerror(L, 1, "invalid digest type");

    HMAC_Init_ex(c->ctx, k, k_len, type, NULL);

    return 1;
}

static int hmac_clone(lua_State* L)
{
    hmac_context* c = (hmac_context*)luaL_checkudata(L, 1, LUACRYPTO_HMACNAME);
    hmac_context* d = hmac_pnew(L);
    if (!HMAC_CTX_copy(d->ctx, c->ctx)) {
        return crypto_error(L);
    }
    return 1;
}

static int hmac_reset(lua_State* L)
{
    hmac_context* c = (hmac_context*)luaL_checkudata(L, 1, LUACRYPTO_HMACNAME);
    HMAC_Init_ex(c->ctx, NULL, 0, NULL, NULL);
    return 0;
}

static int hmac_update(lua_State* L)
{
    size_t s_len;
    hmac_context* c = (hmac_context*)luaL_checkudata(L, 1, LUACRYPTO_HMACNAME);
    const char* s = luaL_checklstring(L, 2, &s_len);

    if (HMAC_Update(c->ctx, (const unsigned char*)s, s_len) != 1) {
        return crypto_error(L);
    }

    lua_settop(L, 1);
    return 1;
}

static int hmac_final(lua_State* L)
{
    hmac_context* c = (hmac_context*)luaL_checkudata(L, 1, LUACRYPTO_HMACNAME);
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int written = 0;
    unsigned int i;
    char* hex;

    if (lua_isstring(L, 2)) {
        size_t s_len;
        const char* s = luaL_checklstring(L, 2, &s_len);
        HMAC_Update(c->ctx, (const unsigned char*)s, s_len);
    }

    if (HMAC_Final(c->ctx, digest, &written) != 1) {
        return crypto_error(L);
    }

    if (lua_toboolean(L, 3))
        lua_pushlstring(L, (char*)digest, written);
    else {
        hex = (char*)calloc(sizeof(char), written * 2 + 1);
        for (i = 0; i < written; i++)
            sprintf(hex + 2 * i, "%02x", digest[i]);
        lua_pushlstring(L, hex, written * 2);
        free(hex);
    }

    return 1;
}

static int hmac_tostring(lua_State* L)
{
    hmac_context* c = (hmac_context*)luaL_checkudata(L, 1, LUACRYPTO_HMACNAME);
    char s[64];
    sprintf(s, "%s %p", LUACRYPTO_HMACNAME, (void*)c);
    lua_pushstring(L, s);
    return 1;
}

static int hmac_gc(lua_State* L)
{
    hmac_context* c = (hmac_context*)luaL_checkudata(L, 1, LUACRYPTO_HMACNAME);
    HMAC_CTX_free(c->ctx);
    return 1;
}

static int hmac_fdigest(lua_State* L)
{
    const char* t = luaL_checkstring(L, 1);
    const EVP_MD* type = EVP_get_digestbyname(t);
    size_t s_len, k_len;
    const char *s, *k;
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int written = 0;
    unsigned int i;
    char* hex;

    HMAC_CTX* c = HMAC_CTX_new();

    if (type == NULL) {
        HMAC_CTX_free(c);
        luaL_argerror(L, 1, "invalid digest type");
        return 0;
    }

    s = luaL_checklstring(L, 2, &s_len);
    k = luaL_checklstring(L, 3, &k_len);

    HMAC_Init_ex(c, k, (int)k_len, type, NULL);
    HMAC_Update(c, (unsigned char*)s, s_len);
    HMAC_Final(c, digest, &written);
    HMAC_CTX_free(c);

    if (lua_toboolean(L, 4))
        lua_pushlstring(L, (char*)digest, written);
    else {
        hex = (char*)calloc(sizeof(char), written * 2 + 1);
        for (i = 0; i < written; i++)
            sprintf(hex + 2 * i, "%02x", digest[i]);
        lua_pushlstring(L, hex, written * 2);
        free(hex);
    }

    return 1;
}

/*************** SIGN API ***************/
typedef struct sign_context {
    EVP_MD_CTX* ctx;
} sign_context;

static sign_context* sign_pnew(lua_State* L)
{
    sign_context* c = (sign_context*)lua_newuserdata(L, sizeof(sign_context));
    c->ctx = EVP_MD_CTX_new();

    luaL_getmetatable(L, LUACRYPTO_SIGNNAME);
    lua_setmetatable(L, -2);
    return c;
}

static int sign_fnew(lua_State* L)
{
    const char* s = luaL_checkstring(L, 1);
    const EVP_MD* md = EVP_get_digestbyname(s);
    sign_context* c;

    if (md == NULL)
        return luaL_argerror(L, 1, "invalid digest type");

    c = sign_pnew(L);
    if (EVP_SignInit_ex(c->ctx, md, NULL) != 1)
        return crypto_error(L);

    return 1;
}

static int sign_update(lua_State* L)
{
    sign_context* c = (sign_context*)luaL_checkudata(L, 1, LUACRYPTO_SIGNNAME);
    size_t input_len = 0;
    const unsigned char* input = (unsigned char*)luaL_checklstring(L, 2, &input_len);

    EVP_SignUpdate(c->ctx, input, input_len);
    return 0;
}

static int sign_final(lua_State* L)
{
    sign_context* c = (sign_context*)luaL_checkudata(L, 1, LUACRYPTO_SIGNNAME);
    unsigned int output_len = 0;
    unsigned char* buffer;
    EVP_PKEY** pkey = (EVP_PKEY**)luaL_checkudata(L, 2, LUACRYPTO_PKEYNAME);

    buffer = (unsigned char*)malloc((size_t)EVP_PKEY_size(*pkey));
    if (!EVP_SignFinal(c->ctx, buffer, &output_len, *pkey)) {
        free(buffer);
        return crypto_error(L);
    }
    lua_pushlstring(L, (char*)buffer, output_len);
    free(buffer);

    return 1;
}

static int sign_tostring(lua_State* L)
{
    sign_context* c = (sign_context*)luaL_checkudata(L, 1, LUACRYPTO_SIGNNAME);
    char s[64];
    sprintf(s, "%s %p", LUACRYPTO_SIGNNAME, (void*)c);
    lua_pushstring(L, s);
    return 1;
}

static int sign_gc(lua_State* L)
{
    sign_context* c = (sign_context*)luaL_checkudata(L, 1, LUACRYPTO_SIGNNAME);
    EVP_MD_CTX_free(c->ctx);
    return 1;
}

static int sign_fsign(lua_State* L)
{
    /* parameter 1 is the 'crypto.sign' table */
    const char* type_name = luaL_checkstring(L, 2);
    const EVP_MD* type = EVP_get_digestbyname(type_name);
    size_t input_len;
    unsigned int output_len;
    const unsigned char* input;
    unsigned char* buffer;
    EVP_PKEY** pkey;
    EVP_MD_CTX* c;

    if (type == NULL) {
        luaL_argerror(L, 2, "invalid digest type");
        return 0;
    }

    input = (unsigned char*)luaL_checklstring(L, 3, &input_len);
    pkey = (EVP_PKEY**)luaL_checkudata(L, 4, LUACRYPTO_PKEYNAME);

    c = EVP_MD_CTX_create();
    EVP_MD_CTX_init(c);
    EVP_SignInit_ex(c, type, NULL);
    buffer = (unsigned char*)malloc((size_t)EVP_PKEY_size(*pkey));
    EVP_SignUpdate(c, input, input_len);
    if (!EVP_SignFinal(c, buffer, &output_len, *pkey)) {
        EVP_MD_CTX_free(c);
        free(buffer);
        return crypto_error(L);
    }
    EVP_MD_CTX_free(c);

    lua_pushlstring(L, (char*)buffer, output_len);
    free(buffer);
    return 1;
}

/*************** VERIFY API ***************/
typedef struct verify_context {
    EVP_MD_CTX* ctx;
} verify_context;

static verify_context* verify_pnew(lua_State* L)
{
    verify_context* c = (verify_context*)lua_newuserdata(L, sizeof(verify_context));
    c->ctx = EVP_MD_CTX_new();

    luaL_getmetatable(L, LUACRYPTO_VERIFYNAME);
    lua_setmetatable(L, -2);
    return c;
}

static int verify_fnew(lua_State* L)
{
    const char* s = luaL_checkstring(L, 1);
    const EVP_MD* md = EVP_get_digestbyname(s);
    verify_context* c;

    if (md == NULL)
        return luaL_argerror(L, 1, "invalid digest type");

    c = verify_pnew(L);

    if (EVP_VerifyInit_ex(c->ctx, md, NULL) != 1)
        return crypto_error(L);

    return 1;
}

static int verify_update(lua_State* L)
{
    verify_context* c = (verify_context*)luaL_checkudata(L, 1, LUACRYPTO_VERIFYNAME);
    size_t input_len = 0;
    const unsigned char* input = (unsigned char*)luaL_checklstring(L, 2, &input_len);

    EVP_VerifyUpdate(c->ctx, input, input_len);
    return 0;
}

static int verify_final(lua_State* L)
{
    verify_context* c = (verify_context*)luaL_checkudata(L, 1, LUACRYPTO_VERIFYNAME);
    size_t sig_len = 0;
    const unsigned char* sig = (unsigned char*)luaL_checklstring(L, 2, &sig_len);
    EVP_PKEY** pkey = (EVP_PKEY**)luaL_checkudata(L, 3, LUACRYPTO_PKEYNAME);
    int ret;

    ret = EVP_VerifyFinal(c->ctx, sig, sig_len, *pkey);
    if (ret == -1)
        return crypto_error(L);

    lua_pushboolean(L, ret);
    return 1;
}

static int verify_tostring(lua_State* L)
{
    verify_context* c = (verify_context*)luaL_checkudata(L, 1, LUACRYPTO_VERIFYNAME);
    char s[64];
    sprintf(s, "%s %p", LUACRYPTO_VERIFYNAME, (void*)c);
    lua_pushstring(L, s);
    return 1;
}

static int verify_gc(lua_State* L)
{
    verify_context* c = (verify_context*)luaL_checkudata(L, 1, LUACRYPTO_VERIFYNAME);
    EVP_MD_CTX_free(c->ctx);
    return 1;
}

static int verify_fverify(lua_State* L)
{
    /* parameter 1 is the 'crypto.verify' table */
    const char* type_name = luaL_checkstring(L, 2);
    const EVP_MD* type = EVP_get_digestbyname(type_name);

    size_t input_len = 0;
    const unsigned char* input;
    size_t sig_len = 0;
    const unsigned char* sig;
    int ret;
    EVP_PKEY** pkey;
    EVP_MD_CTX* c;

    if (type == NULL) {
        luaL_argerror(L, 1, "invalid digest type");
        return 0;
    }

    input = (unsigned char*)luaL_checklstring(L, 3, &input_len);
    sig = (unsigned char*)luaL_checklstring(L, 4, &sig_len);
    pkey = (EVP_PKEY**)luaL_checkudata(L, 5, LUACRYPTO_PKEYNAME);

    c = EVP_MD_CTX_new();
    EVP_VerifyInit_ex(c, type, NULL);
    EVP_VerifyUpdate(c, input, input_len);
    ret = EVP_VerifyFinal(c, sig, sig_len, *pkey);
    EVP_MD_CTX_free(c);
    if (ret == -1)
        return crypto_error(L);

    lua_pushboolean(L, ret);
    return 1;
}
/*************** RAND API ***************/

static int rand_do_bytes(lua_State* L, int (*bytes)(unsigned char*, int))
{
    size_t count = (size_t)luaL_checkinteger(L, 1);
    unsigned char tmp[256], *buf = tmp;
    if (count > sizeof tmp)
        buf = (unsigned char*)malloc(count);
    if (!buf)
        return luaL_error(L, "out of memory");
    else if (!bytes(buf, (int)count))
        return crypto_error(L);
    lua_pushlstring(L, (char*)buf, count);
    if (buf != tmp)
        free(buf);
    return 1;
}

static int rand_bytes(lua_State* L)
{
    return rand_do_bytes(L, RAND_bytes);
}

static int rand_add(lua_State* L)
{
    size_t num;
    const void* buf = luaL_checklstring(L, 1, &num);
    double entropy = luaL_optnumber(L, 2, num);
    RAND_add(buf, (int)num, entropy);
    return 0;
}

static int rand_status(lua_State* L)
{
    lua_pushboolean(L, RAND_status());
    return 1;
}

enum {
    WRITE_FILE_COUNT = 1024
};
static int rand_load(lua_State* L)
{
    const char* name = luaL_optstring(L, 1, 0);
    char tmp[256];
    int n;
    if (!name && !(name = RAND_file_name(tmp, sizeof tmp)))
        return crypto_error(L);
    n = RAND_load_file(name, WRITE_FILE_COUNT);
    if (n == 0)
        return crypto_error(L);
    lua_pushnumber(L, n);
    return 1;
}

static int rand_write(lua_State* L)
{
    const char* name = luaL_optstring(L, 1, 0);
    char tmp[256];
    int n;
    if (!name && !(name = RAND_file_name(tmp, sizeof tmp)))
        return crypto_error(L);
    n = RAND_write_file(name);
    if (n == 0)
        return crypto_error(L);
    lua_pushnumber(L, n);
    return 1;
}

static int rand_cleanup(lua_State* L UNUSED)
{
    RAND_cleanup();
    return 0;
}

/*************** PKEY API ***************/
typedef struct pkey_context {
    EVP_PKEY* pkey;
} pkey_context;

static pkey_context* pkey_new(lua_State* L)
{
    pkey_context* c = (pkey_context*)lua_newuserdata(L, sizeof(pkey_context));
    c->pkey = NULL;

    luaL_getmetatable(L, LUACRYPTO_PKEYNAME);
    lua_setmetatable(L, -2);

    return c;
}

static int pkey_generate(lua_State* L)
{
    const char* options[] = { "rsa", "dsa", NULL };
    int idx = luaL_checkoption(L, 1, NULL, options);
    int key_len = luaL_checkinteger(L, 2);
    pkey_context* c = pkey_new(L);
    if (idx == 0) {
        RSA* rsa = RSA_new();
        BIGNUM* bne = BN_new();
        BN_set_word(bne, RSA_F4);
        if (RSA_generate_key_ex(rsa, key_len, bne, NULL) != 1) {
            RSA_free(rsa);
            BN_free(bne);
            return crypto_error(L);
        }
        BN_free(bne);

        c->pkey = EVP_PKEY_new();
        EVP_PKEY_assign_RSA(c->pkey, rsa);
        return 1;
    } else {
        DSA* dsa = DSA_new();
        if (DSA_generate_parameters_ex(dsa, key_len, NULL, 0, NULL, NULL, NULL) != 1) {
            DSA_free(dsa);
            return crypto_error(L);
        }

        c->pkey = EVP_PKEY_new();
        EVP_PKEY_assign_DSA(c->pkey, dsa);
        return 1;
    }
}

static int pkey_to_pem(lua_State* L)
{
    pkey_context* c = (pkey_context*)luaL_checkudata(L, 1, LUACRYPTO_PKEYNAME);
    int private = lua_isboolean(L, 2) && lua_toboolean(L, 2);
    int ret;

    BUF_MEM* buf;
    BIO* mem = BIO_new(BIO_s_mem());
    int type = EVP_PKEY_base_id(c->pkey);

    if (private && type == EVP_PKEY_DSA)
        ret = PEM_write_bio_DSAPrivateKey(mem, EVP_PKEY_get0_DSA(c->pkey), NULL, NULL, 0, NULL, NULL);
    else if (private && type == EVP_PKEY_RSA)
        ret = PEM_write_bio_RSAPrivateKey(mem, EVP_PKEY_get0_RSA(c->pkey), NULL, NULL, 0, NULL, NULL);
    else if (private)
        ret = PEM_write_bio_PrivateKey(mem, c->pkey, NULL, NULL, 0, NULL, NULL);
    else
        ret = PEM_write_bio_PUBKEY(mem, c->pkey);

    if (ret == 0) {
        ret = crypto_error(L);
        goto error;
    }

    BIO_get_mem_ptr(mem, &buf);
    lua_pushlstring(L, buf->data, buf->length);
    ret = 1;

error:
    BIO_free(mem);
    return ret;
}

static int pkey_read(lua_State* L)
{
    const char* filename = luaL_checkstring(L, 1);
    int readPrivate = lua_isboolean(L, 2) && lua_toboolean(L, 2);
    FILE* fp = fopen(filename, "r");
    pkey_context* c = pkey_new(L);

    if (!fp)
        luaL_error(L, "File not found: %s", filename);

    if (readPrivate)
        c->pkey = PEM_read_PrivateKey(fp, NULL, NULL, NULL);
    else
        c->pkey = PEM_read_PUBKEY(fp, NULL, NULL, NULL);

    fclose(fp);

    if (!c->pkey)
        return crypto_error(L);

    return 1;
}

static int pkey_from_pem(lua_State* L)
{
    const char* key = luaL_checkstring(L, 1);
    int private = lua_isboolean(L, 2) && lua_toboolean(L, 2);
    pkey_context* c = pkey_new(L);
    BIO* mem = BIO_new(BIO_s_mem());
    int ret;

    ret = BIO_puts(mem, key);
    if (ret != (int)strlen(key)) {
        goto error;
    }

    if (private)
        c->pkey = PEM_read_bio_PrivateKey(mem, NULL, NULL, NULL);
    else
        c->pkey = PEM_read_bio_PUBKEY(mem, NULL, NULL, NULL);

    if (!c->pkey)
        goto error;

    return 1;

error:
    BIO_free(mem);
    return crypto_error(L);
}

static int pkey_write(lua_State* L)
{
    pkey_context* c = (pkey_context*)luaL_checkudata(L, 1, LUACRYPTO_PKEYNAME);
    const char* pubfn = lua_tostring(L, 2);
    const char* privfn = lua_tostring(L, 3);
    if (pubfn) {
        FILE* fp = fopen(pubfn, "w");
        if (!fp)
            luaL_error(L, "Unable to write to file: %s", pubfn);
        if (!PEM_write_PUBKEY(fp, c->pkey))
            return crypto_error(L);
        fclose(fp);
    }
    if (privfn) {
        FILE* fp = fopen(privfn, "w");
        if (!fp)
            luaL_error(L, "Unable to write to file: %s", privfn);
        if (!PEM_write_PrivateKey(fp, c->pkey, NULL, NULL, 0, NULL, NULL))
            return crypto_error(L);
        fclose(fp);
    }
    return 0;
}

static int pkey_gc(lua_State* L)
{
    pkey_context* c = (pkey_context*)luaL_checkudata(L, 1, LUACRYPTO_PKEYNAME);
    if (c->pkey) {
        EVP_PKEY_free(c->pkey);
    }
    return 0;
}

static int pkey_tostring(lua_State* L)
{
    pkey_context* c = (pkey_context*)luaL_checkudata(L, 1, LUACRYPTO_PKEYNAME);
    char buf[60];
    sprintf(buf, "%s %s %d %p", LUACRYPTO_PKEYNAME, EVP_PKEY_base_id(c->pkey) == EVP_PKEY_DSA ? "DSA" : "RSA", EVP_PKEY_bits(c->pkey), c);
    lua_pushstring(L, buf);
    return 1;
}

/*************** SEAL API ***************/

typedef struct seal_context {
    EVP_CIPHER_CTX* ctx;
    int eklen;
    unsigned char iv[EVP_MAX_IV_LENGTH];
    unsigned char* ek;
} seal_context;

static seal_context* seal_pnew(lua_State* L)
{
    seal_context* c = (seal_context*)lua_newuserdata(L, sizeof(seal_context));
    luaL_getmetatable(L, LUACRYPTO_SEALNAME);
    lua_setmetatable(L, -2);

    memset(c, 0, sizeof(seal_context));
    c->ctx = EVP_CIPHER_CTX_new();

    return c;
}

static int seal_gc(lua_State* L)
{
    seal_context* c = (seal_context*)luaL_checkudata(L, 1, LUACRYPTO_SEALNAME);
    EVP_CIPHER_CTX_free(c->ctx);
    if (c->ek != NULL) {
        free(c->ek);
    }
    return 0;
}

static int seal_tostring(lua_State* L)
{
    seal_context* c = (seal_context*)luaL_checkudata(L, 1, LUACRYPTO_SEALNAME);
    char s[64];
    sprintf(s, "%s %p %s", LUACRYPTO_SEALNAME, (void*)c, EVP_CIPHER_name(EVP_CIPHER_CTX_cipher(c->ctx)));
    lua_pushstring(L, s);
    return 1;
}

static int seal_fnew(lua_State* L)
{
    const char* cipher_type = luaL_checkstring(L, 1);
    const EVP_CIPHER* cipher = EVP_get_cipherbyname(cipher_type);
    int npubk = 1;
    pkey_context* pkey_ctx;
    seal_context* seal_ctx;

    if (cipher == NULL)
        return luaL_argerror(L, 1, "invalid encrypt cipher");

    pkey_ctx = (pkey_context*)luaL_checkudata(L, 2, LUACRYPTO_PKEYNAME);

    seal_ctx = seal_pnew(L);
    EVP_CIPHER_CTX_init(seal_ctx->ctx);

    seal_ctx->ek = (unsigned char*)malloc((size_t)EVP_PKEY_size(pkey_ctx->pkey) * (size_t)npubk);

    if (!EVP_SealInit(seal_ctx->ctx, cipher, &seal_ctx->ek, &seal_ctx->eklen,
            seal_ctx->iv, &pkey_ctx->pkey, npubk)) {
        free(seal_ctx->ek);
        seal_ctx->ek = NULL;
        return crypto_error(L);
    }

    return 1;
}

static int seal_update(lua_State* L)
{
    seal_context* c = (seal_context*)luaL_checkudata(L, 1, LUACRYPTO_SEALNAME);
    size_t input_len = 0;
    const unsigned char* input = (unsigned char*)luaL_checklstring(L, 2, &input_len);
    int output_len = 0;

    unsigned char* temp = (unsigned char*)malloc(input_len + (size_t)EVP_CIPHER_CTX_block_size(c->ctx));
    EVP_SealUpdate(c->ctx, temp, &output_len, input, (int)input_len);
    lua_pushlstring(L, (char*)temp, (size_t)output_len);
    free(temp);

    return 1;
}

static int seal_final(lua_State* L)
{
    seal_context* c = (seal_context*)luaL_checkudata(L, 1, LUACRYPTO_SEALNAME);
    int output_len = 0;
    unsigned char buffer[EVP_MAX_BLOCK_LENGTH];

    EVP_SealFinal(c->ctx, buffer, &output_len);
    lua_pushlstring(L, (char*)buffer, (size_t)output_len);

    lua_pushlstring(L, (const char*)c->ek, (size_t)c->eklen);
    lua_pushlstring(L, (const char*)c->iv, (size_t)EVP_CIPHER_iv_length(EVP_CIPHER_CTX_cipher(c->ctx)));

    free(c->ek);
    c->ek = NULL;

    return 3;
}

static int seal_fseal(lua_State* L)
{
    /* parameter 1 is the 'crypto.seal' table */
    const char* cipher_type = luaL_checkstring(L, 2);
    const EVP_CIPHER* cipher = EVP_get_cipherbyname(cipher_type);
    pkey_context* pkey_ctx = (pkey_context*)luaL_checkudata(L, 4, LUACRYPTO_PKEYNAME);
    int npubk = 1;
    int eklen;
    unsigned char iv[EVP_MAX_IV_LENGTH];
    const unsigned char* message;
    int message_length;
    unsigned char* ek;
    int block_size;
    EVP_CIPHER_CTX* ctx;
    luaL_Buffer buffer;
    int output_length;
    char* temp;
    int sz;

    if (cipher == NULL) {
        luaL_argerror(L, 1, "invalid encrypt cipher");
        return 0;
    }

    message = (const unsigned char*)luaL_checkstring(L, 3);
    message_length = (int)lua_rawlen(L, 3);

    ek = (unsigned char*)malloc((size_t)EVP_PKEY_size(pkey_ctx->pkey) * (size_t)npubk);

    ctx = EVP_CIPHER_CTX_new();
    if (!EVP_SealInit(ctx, cipher, &ek, &eklen, iv, &pkey_ctx->pkey, npubk)) {
        free(ek);
        EVP_CIPHER_CTX_free(ctx);
        return crypto_error(L);
    }

    luaL_buffinit(L, &buffer);

    block_size = EVP_CIPHER_block_size(cipher);

    while (message_length > 0) {
        temp = luaL_prepbuffer(&buffer);
        sz = MIN(LUAL_BUFFERSIZE - block_size - 1, message_length);

        if (!EVP_SealUpdate(ctx, (unsigned char*)temp, &output_length, message, sz)) {
            free(ek);
            EVP_CIPHER_CTX_free(ctx);
            return crypto_error(L);
        }
        message += sz;
        message_length -= sz;
        luaL_addsize(&buffer, output_length);
    }

    temp = luaL_prepbuffer(&buffer);
    if (!EVP_SealFinal(ctx, (unsigned char*)temp, &output_length)) {
        free(ek);
        EVP_CIPHER_CTX_free(ctx);
        return crypto_error(L);
    }

    luaL_addsize(&buffer, output_length);

    luaL_pushresult(&buffer);
    lua_pushlstring(L, (const char*)ek, (size_t)eklen);
    lua_pushlstring(L, (const char*)iv, (size_t)EVP_CIPHER_iv_length(cipher));

    EVP_CIPHER_CTX_free(ctx);
    free(ek);

    return 3;
}

/*************** OPEN API ***************/

typedef struct open_context {
    EVP_CIPHER_CTX* ctx;
    EVP_CIPHER* cipher;
    int pkey_ref;
} open_context;

static open_context* open_pnew(lua_State* L)
{
    open_context* c = (open_context*)lua_newuserdata(L, sizeof(open_context));
    luaL_getmetatable(L, LUACRYPTO_OPENNAME);
    lua_setmetatable(L, -2);

    memset(c, 0, sizeof(open_context));
    c->ctx = EVP_CIPHER_CTX_new();
    c->pkey_ref = LUA_NOREF;

    return c;
}

static int open_gc(lua_State* L)
{
    open_context* c = luaL_checkudata(L, 1, LUACRYPTO_OPENNAME);
    EVP_CIPHER_CTX_free(c->ctx);

    if (c->pkey_ref != LUA_NOREF) {
        luaL_unref(L, LUA_REGISTRYINDEX, c->pkey_ref);
    }
    return 0;
}

static int open_tostring(lua_State* L)
{
    open_context* c = (open_context*)luaL_checkudata(L, 1, LUACRYPTO_OPENNAME);
    pkey_context* pkey_ctx = (pkey_context*)luaL_checkudata(L, -1, LUACRYPTO_PKEYNAME);
    char s[64];

    lua_rawgeti(L, LUA_REGISTRYINDEX, c->pkey_ref);
    sprintf(s, "%s %p %s %s %d %p", LUACRYPTO_OPENNAME, (void*)c, EVP_CIPHER_name(c->cipher),
        EVP_PKEY_base_id(pkey_ctx->pkey) == EVP_PKEY_DSA ? "DSA" : "RSA", EVP_PKEY_bits(pkey_ctx->pkey), pkey_ctx->pkey);

    lua_pop(L, 1);
    lua_pushstring(L, s);
    return 1;
}

static int open_fnew(lua_State* L)
{
    const char* type_name = luaL_checkstring(L, 1);
    const EVP_CIPHER* cipher = EVP_get_cipherbyname(type_name);
    size_t encrypted_key_len, iv_len;
    const unsigned char *encrypted_key, *iv;
    pkey_context* pkey_ctx = (pkey_context*)luaL_checkudata(L, 2, LUACRYPTO_PKEYNAME);
    open_context* open_ctx;

    if (cipher == NULL)
        return luaL_argerror(L, 1, "invalid decrypt cipher");

    /* checks for the encrypted key */
    encrypted_key = (const unsigned char*)luaL_checklstring(L, 3, &encrypted_key_len);
    /* checks for the initialization vector */
    iv = (const unsigned char*)luaL_checklstring(L, 4, &iv_len);

    if ((size_t)EVP_CIPHER_iv_length(cipher) != iv_len)
        return luaL_argerror(L, 4, "invalid iv length");

    open_ctx = open_pnew(L);
    open_ctx->cipher = (EVP_CIPHER*)cipher;

    if (!EVP_OpenInit(open_ctx->ctx, open_ctx->cipher, encrypted_key,
            (int)encrypted_key_len, iv, pkey_ctx->pkey))
        return crypto_error(L);

    lua_pushvalue(L, 2);
    open_ctx->pkey_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    return 1;
}

static int open_update(lua_State* L)
{
    open_context* c = (open_context*)luaL_checkudata(L, 1, LUACRYPTO_OPENNAME);
    size_t input_len = 0;
    const unsigned char* input = (unsigned char*)luaL_checklstring(L, 2, &input_len);

    luaL_Buffer buffer;
    luaL_buffinit(L, &buffer);

    while (input_len > 0) {
        int output_length;
        unsigned char* temp = (unsigned char*)luaL_prepbuffer(&buffer);
        size_t sz = MIN(LUAL_BUFFERSIZE - 1, input_len);
        if (!EVP_OpenUpdate(c->ctx, temp, &output_length, input, (int)sz)) {
            return crypto_error(L);
        }

        input += sz;
        input_len -= sz;
        luaL_addsize(&buffer, output_length);
    }

    luaL_pushresult(&buffer);
    return 1;
}

static int open_final(lua_State* L)
{
    open_context* c = (open_context*)luaL_checkudata(L, 1, LUACRYPTO_OPENNAME);
    int output_len = 0;
    unsigned char buffer[EVP_MAX_BLOCK_LENGTH];

    EVP_OpenFinal(c->ctx, buffer, &output_len);
    lua_pushlstring(L, (char*)buffer, (size_t)output_len);

    return 1;
}

static int open_fopen(lua_State* L)
{
    /* parameter 1 is the 'crypto.open' table */
    const char* type_name = luaL_checkstring(L, 2);
    const EVP_CIPHER* cipher = EVP_get_cipherbyname(type_name);
    pkey_context* pkey_ctx = (pkey_context*)luaL_checkudata(L, 4, LUACRYPTO_PKEYNAME);
    size_t data_len, encrypted_key_len, iv_len;
    const unsigned char *data, *encrypted_key, *iv;
    EVP_CIPHER_CTX* ctx;
    int output_length;
    unsigned char* temp;
    size_t sz;
    luaL_Buffer buffer;

    if (cipher == NULL) {
        luaL_argerror(L, 1, "invalid decrypt cipher");
        return 0;
    }

    data = (const unsigned char*)luaL_checklstring(L, 3, &data_len);
    encrypted_key = (const unsigned char*)luaL_checklstring(L, 5, &encrypted_key_len);
    iv = (const unsigned char*)luaL_checklstring(L, 6, &iv_len);

    if ((size_t)EVP_CIPHER_iv_length(cipher) != iv_len) {
        luaL_argerror(L, 6, "invalid iv length");
        return 0;
    }

    ctx = EVP_CIPHER_CTX_new();

    if (!EVP_OpenInit(ctx, cipher, encrypted_key, encrypted_key_len, iv, pkey_ctx->pkey)) {
        EVP_CIPHER_CTX_free(ctx);
        return crypto_error(L);
    }

    luaL_buffinit(L, &buffer);

    while (data_len > 0) {
        temp = (unsigned char*)luaL_prepbuffer(&buffer);
        sz = MIN(LUAL_BUFFERSIZE - 1U, data_len);
        if (!EVP_OpenUpdate(ctx, temp, &output_length, data, (int)sz)) {
            EVP_CIPHER_CTX_free(ctx);
            return crypto_error(L);
        }

        data += sz;
        data_len -= sz;
        luaL_addsize(&buffer, output_length);
    }

    temp = (unsigned char*)luaL_prepbuffer(&buffer);
    if (!EVP_OpenFinal(ctx, temp, &output_length)) {
        EVP_CIPHER_CTX_free(ctx);
        return crypto_error(L);
    }
    luaL_addsize(&buffer, output_length);

    luaL_pushresult(&buffer);
    EVP_CIPHER_CTX_free(ctx);
    return 1;
}

/*************** CORE API ***************/

static void list_callback(const OBJ_NAME* obj, void* arg)
{
    lua_State* L = (lua_State*)arg;
    int idx = (int)lua_rawlen(L, -1);
    lua_pushstring(L, obj->name);
    lua_rawseti(L, -2, idx + 1);
}

static int luacrypto_list(lua_State* L)
{
    int options[] = { OBJ_NAME_TYPE_CIPHER_METH, OBJ_NAME_TYPE_MD_METH };
    const char* names[] = { "ciphers", "digests", NULL };
    int idx = luaL_checkoption(L, 1, NULL, names);
    lua_createtable(L, 0, 0);
    OBJ_NAME_do_all_sorted(options[idx], list_callback, L);
    return 1;
}

static int luacrypto_hex(lua_State* L)
{
    size_t i, len = 0;
    const unsigned char* input = (unsigned char*)luaL_checklstring(L, 1, &len);
    char* hex = (char*)calloc(sizeof(char), len * 2 + 1);
    for (i = 0; i < len; i++) {
        sprintf(hex + 2 * i, "%02x", input[i]);
    }
    lua_pushlstring(L, hex, len * 2);
    free(hex);
    return 1;
}

static int luacrypto_base64_encode(lua_State* L)
{
    BUF_MEM* buf;
    size_t len = 0;
    const unsigned char* input = (unsigned char*)luaL_checklstring(L, 1, &len);
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* mem = BIO_new(BIO_s_mem());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    mem = BIO_push(b64, mem);
    BIO_write(mem, input, len);
    BIO_flush(mem);
    BIO_get_mem_ptr(mem, &buf);
    lua_pushlstring(L, buf->data, buf->length);
    BIO_free_all(mem);
    return 1;
}

static int luacrypto_base64_decode(lua_State* L)
{
    size_t len, out = 0;
    const unsigned char* input = (unsigned char*)luaL_checklstring(L, 1, &len);
    char* buffer = (char*)malloc(len);

    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* mem = BIO_new_mem_buf((void*)input, len);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    mem = BIO_push(b64, mem);
    out = BIO_read(mem, buffer, len);
    lua_pushlstring(L, buffer, out);
    free(buffer);
    BIO_free_all(mem);
    return 1;
}
/*************** x509 API ***************/

static X509* x509__load_cert(BIO* cert)
{
    X509* x = PEM_read_bio_X509_AUX(cert, NULL, NULL, NULL);
    return x;
}

static X509* x509__x509_from_string(const char* pem)
{
    BIO* mem = BIO_new(BIO_s_mem());
    X509* cert;
    int ret;

    if (!mem)
        return NULL;

    ret = BIO_puts(mem, pem);
    if (ret != (int)strlen(pem))
        goto error;

    cert = x509__load_cert(mem);
    if (cert == NULL)
        goto error;

    return cert;

error:
    BIO_free(mem);
    return NULL;
}

struct x509_cert {
    X509* cert;
};

static struct x509_cert* x509_cert__get(lua_State* L)
{
    return luaL_checkudata(L, 1, LUACRYPTO_X509_CERT_NAME);
}

static int x509_cert_fnew(lua_State* L)
{
    struct x509_cert* x = lua_newuserdata(L, sizeof(struct x509_cert));

    x->cert = NULL;

    luaL_getmetatable(L, LUACRYPTO_X509_CERT_NAME);
    lua_setmetatable(L, -2);

    return 1;
}

static int x509_cert_fx509_cert(lua_State* L)
{
    return x509_cert_fnew(L);
}

static int x509_cert_gc(lua_State* L)
{
    struct x509_cert* c = x509_cert__get(L);
    X509_free(c->cert);
    return 0;
}

static int x509_cert_from_pem(lua_State* L)
{
    struct x509_cert* x = x509_cert__get(L);
    const char* pem = luaL_checkstring(L, 2);

    if (x->cert != NULL)
        X509_free(x->cert);

    x->cert = x509__x509_from_string(pem);
    if (!x->cert)
        return crypto_error(L);

    return 1;
}

static int x509_cert_pubkey(lua_State* L)
{
    struct x509_cert* x = x509_cert__get(L);
    EVP_PKEY* pkey = X509_get_pubkey(x->cert);

    if (!pkey)
        return crypto_error(L);

    pkey_context* pkey_ctx = pkey_new(L);
    pkey_ctx->pkey = pkey;

    return 1;
}

struct x509_ca {
    X509_STORE* store;
    STACK_OF(X509) * stack;
};

static int x509_ca_fnew(lua_State* L)
{
    struct x509_ca* x = lua_newuserdata(L, sizeof(struct x509_ca));

    x->store = X509_STORE_new();
    x->stack = sk_X509_new_null();

    luaL_getmetatable(L, LUACRYPTO_X509_CA_NAME);
    lua_setmetatable(L, -2);

    return 1;
}

static int x509_ca_fx509_ca(lua_State* L)
{
    return x509_ca_fnew(L);
}

static struct x509_ca* x509_ca__get(lua_State* L)
{
    return luaL_checkudata(L, 1, LUACRYPTO_X509_CA_NAME);
}

static int x509_ca_gc(lua_State* L)
{
    struct x509_ca* c = x509_ca__get(L);
    sk_X509_pop_free(c->stack, X509_free);
    X509_STORE_free(c->store);
    return 0;
}

/* verify a cert is signed by the ca */
static int x509_ca__verify(struct x509_ca* x, X509* cert)
{
    X509_STORE_CTX* csc;
    int ret = -1;

    csc = X509_STORE_CTX_new();
    if (csc == NULL)
        return ret;

    X509_STORE_set_flags(x->store, 0);
    if (!X509_STORE_CTX_init(csc, x->store, cert, 0))
        goto out;

    X509_STORE_CTX_trusted_stack(csc, x->stack);
    ret = X509_verify_cert(csc);

out:
    X509_STORE_CTX_free(csc);
    return ret;
}

/* verify a cert is signed by the ca */
static int x509_ca_verify_pem(lua_State* L)
{
    struct x509_ca* x = x509_ca__get(L);
    const char* pem = luaL_checkstring(L, 2);
    X509* cert;
    int ret;

    cert = x509__x509_from_string(pem);
    if (!cert)
        return crypto_error(L);

    ret = x509_ca__verify(x, cert);
    X509_free(cert);
    if (ret < 0)
        return crypto_error(L);

    lua_pushboolean(L, ret);

    return 1;
}

static int x509_ca_add_pem(lua_State* L)
{
    struct x509_ca* x = x509_ca__get(L);
    const char* pem = luaL_checkstring(L, 2);
    X509* cert;

    cert = x509__x509_from_string(pem);
    if (!cert)
        return crypto_error(L);

    sk_X509_push(x->stack, cert);

    lua_pushboolean(L, 1);

    return 1;
}

/*
** Create a metatable and keep stack balance when leave
*/
static void luacrypto_createmeta(lua_State* L, const char* name, const luaL_Reg* methods)
{
    if (!luaL_newmetatable(L, name))
        return;

    /* define metamethods */
    lua_pushvalue(L, -1); // push metatable
    lua_setfield(L, -2, "__index"); // metatable.__index = metatable
    lua_pushliteral(L, LUACRYPTO_PREFIX "you're not allowed to get this metatable");
    lua_setfield(L, -2, "__metatable");
    luaL_setfuncs(L, methods, 0); //add methods to new metatable
    lua_pop(L, 1); // pop new metatable
}

static void create_call_table(lua_State* L, const char* name, lua_CFunction creator, lua_CFunction starter)
{
    lua_createtable(L, 0, 1);
    lua_pushcfunction(L, creator);
    lua_setfield(L, -2, "new");
    /* create metatable for call */
    lua_createtable(L, 0, 1);
    lua_pushcfunction(L, starter);
    lua_setfield(L, -2, "__call");
    lua_setmetatable(L, -2);
    lua_setfield(L, -2, name);
}

#define EVP_METHODS(name)                  \
    struct luaL_Reg name##_methods[] = {   \
        { "__tostring", name##_tostring }, \
        { "__gc", name##_gc },             \
        { "final", name##_final },         \
        { "tostring", name##_tostring },   \
        { "update", name##_update },       \
        { NULL, NULL },                    \
    }

/*
** Create metatables for each class of object.
*/
static void create_metatables(lua_State* L)
{
    struct luaL_Reg digest_methods[] = {
        { "__tostring", digest_tostring },
        { "__gc", digest_gc },
        { "final", digest_final },
        { "tostring", digest_tostring },
        { "update", digest_update },
        { "reset", digest_reset },
        { "clone", digest_clone },
        { NULL, NULL }
    };
    EVP_METHODS(encrypt);
    EVP_METHODS(decrypt);
    EVP_METHODS(sign);
    EVP_METHODS(verify);
    EVP_METHODS(seal);
    EVP_METHODS(open);
    struct luaL_Reg hmac_functions[] = {
        { "digest", hmac_fdigest },
        { "new", hmac_fnew },
        { NULL, NULL }
    };
    struct luaL_Reg hmac_methods[] = {
        { "__tostring", hmac_tostring },
        { "__gc", hmac_gc },
        { "clone", hmac_clone },
        { "final", hmac_final },
        { "reset", hmac_reset },
        { "tostring", hmac_tostring },
        { "update", hmac_update },
        { NULL, NULL }
    };
    struct luaL_Reg rand_functions[] = {
        { "bytes", rand_bytes },
        { "add", rand_add },
        { "seed", rand_add },
        { "status", rand_status },
        { "load", rand_load },
        { "write", rand_write },
        { "cleanup", rand_cleanup },
        { NULL, NULL }
    };
    struct luaL_Reg pkey_functions[] = {
        { "generate", pkey_generate },
        { "read", pkey_read },
        { "from_pem", pkey_from_pem },
        { NULL, NULL }
    };
    struct luaL_Reg pkey_methods[] = {
        { "__tostring", pkey_tostring },
        { "__gc", pkey_gc },
        { "write", pkey_write },
        { "to_pem", pkey_to_pem },
        { NULL, NULL }
    };
    struct luaL_Reg x509_methods[] = {
        { "__gc", x509_cert_gc },
        { "from_pem", x509_cert_from_pem },
        { "pubkey", x509_cert_pubkey },
        { NULL, NULL }
    };
    struct luaL_Reg x509_ca_methods[] = {
        { "__gc", x509_ca_gc },
        { "add_pem", x509_ca_add_pem },
        { "verify_pem", x509_ca_verify_pem },
        { NULL, NULL }
    };

#define CALLTABLE(n) create_call_table(L, #n, n##_fnew, n##_f##n)
    CALLTABLE(digest);
    CALLTABLE(encrypt);
    CALLTABLE(decrypt);
    CALLTABLE(verify);
    CALLTABLE(sign);
    CALLTABLE(seal);
    CALLTABLE(open);
    CALLTABLE(x509_cert);
    CALLTABLE(x509_ca);

    luacrypto_createmeta(L, LUACRYPTO_DIGESTNAME, digest_methods);
    luacrypto_createmeta(L, LUACRYPTO_ENCRYPTNAME, encrypt_methods);
    luacrypto_createmeta(L, LUACRYPTO_DECRYPTNAME, decrypt_methods);
    luacrypto_createmeta(L, LUACRYPTO_HMACNAME, hmac_methods);
    luacrypto_createmeta(L, LUACRYPTO_SIGNNAME, sign_methods);
    luacrypto_createmeta(L, LUACRYPTO_VERIFYNAME, verify_methods);
    luacrypto_createmeta(L, LUACRYPTO_PKEYNAME, pkey_methods);
    luacrypto_createmeta(L, LUACRYPTO_SEALNAME, seal_methods);
    luacrypto_createmeta(L, LUACRYPTO_OPENNAME, open_methods);
    luacrypto_createmeta(L, LUACRYPTO_X509_CERT_NAME, x509_methods);
    luacrypto_createmeta(L, LUACRYPTO_X509_CA_NAME, x509_ca_methods);

#define NONCALLTABLE(n)            \
    luaL_newlib(L, n##_functions); \
    lua_setfield(L, -2, #n)
    NONCALLTABLE(rand);
    NONCALLTABLE(hmac);
    NONCALLTABLE(pkey);
}

/*
** Assumes the table is on top of the stack.
*/
static void luacrypto_set_info(lua_State* L)
{
    lua_pushliteral(L, "_COPYRIGHT");
    lua_pushliteral(L, "Copyright (C) 2005-2006 Keith Howe");
    lua_settable(L, -3);
    lua_pushliteral(L, "_DESCRIPTION");
    lua_pushliteral(L, "LuaCrypto is a Lua wrapper for OpenSSL");
    lua_settable(L, -3);
    lua_pushliteral(L, "_VERSION");
    lua_pushliteral(L, "LuaCrypto 0.4.0");
    lua_settable(L, -3);
}

/*
** Creates the metatables for the objects and registers the
** driver open method.
*/
int luaopen_crypto(lua_State* L)
{
    struct luaL_Reg core_functions[] = {
        { "list", luacrypto_list },
        { "hex", luacrypto_hex },
        { "base64", luacrypto_base64_encode },
        { "unbase64", luacrypto_base64_decode },
        { NULL, NULL }
    };

    luaL_newlib(L, core_functions);
    create_metatables(L);
    luacrypto_set_info(L);
    luaL_checkversion(L);
    return 1;
}

/*
 * OpenSSL_add_all_* is not thread safe, so init Openssl when dlopen
 */
void __attribute__((constructor)) luacrypto_init(void)
{
#ifndef OPENSSL_EXTERNAL_INITIALIZATION
    OpenSSL_add_all_digests();
    OpenSSL_add_all_ciphers();
#endif
}
