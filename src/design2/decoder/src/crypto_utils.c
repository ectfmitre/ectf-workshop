/**
 * 
 * This file defines a common cryptographic interface for AES, SHA256, and HMAC.
 */

#include "crypto_utils.h"

#include <wolfssl/wolfcrypt/aes.h>
#include <wolfssl/wolfcrypt/pkcs7.h>
#include <wolfssl/wolfcrypt/blake2.h>
#include <wolfssl/wolfcrypt/hmac.h>

#define HMAC_LEN    SHA256_DIGEST_SIZE

/**
 * Verify and unpad bytes. 
 * 
 * Padding consists of bytes of value of the pad size.
 * Padding value must be between [1, 16].
 * 
 * e.g. MESSAGE\x03\x03\x03
 */
static int pkcs7_unpad(uint8_t *in, size_t len, int *pt_len) {
    int pad_val = in[len - 1];

    if (pad_val < 1 || pad_val > AES_BLOCK_SIZE) 
        return -1;

    for (int i = 0; i < pad_val; i++) {
        if (in[len - 1 - i] != pad_val) {
            return -1;
        }
    }

    *pt_len = len - pad_val;
    return 0;
}

int decrypt_cbc_sym(uint8_t *ciphertext, size_t len, uint8_t *key, int key_size, uint8_t *iv, uint8_t *plaintext, int *pt_len) {
    Aes aes;
    int result;

    if (len <= 0)
        return -1;

    if (key_size != AES128 && key_size != AES256)
        return -1;

    // Init Aes ctx
    wc_AesInit(&aes, NULL, INVALID_DEVID);

    // Set Aes key
    result = wc_AesSetKey(&aes, key, key_size, iv, AES_DECRYPTION);
    if (result != 0)
        return -1;

    result = wc_AesCbcDecrypt(&aes, plaintext, ciphertext, len);
    if (result != 0) 
        return -1;

    // Remove padding
    result = pkcs7_unpad(plaintext, len, pt_len);
    if (result != 0) 
        return -1;

    return 0;
}

void sha256_hash(uint8_t *in, size_t len, uint8_t *digest) {
    wc_Sha256 sha;
    wc_InitSha256(&sha);
    wc_Sha256Update(&sha, in, len);
    wc_Sha256Final(&sha, digest);
}

void hmac_digest(uint8_t *in, size_t len, uint8_t *key, size_t key_size, uint8_t *digest) {
    Hmac hmac;
    wc_HmacSetKey(&hmac, SHA256, key, key_size);
    wc_HmacUpdate(&hmac, in, len);
    wc_HmacFinal(&hmac, digest);
}

int hmac_verify(uint8_t *data, size_t len, uint8_t *hmac, uint8_t *key, size_t key_size) {
    uint8_t our_hmac[HMAC_LEN];

    hmac_digest(data, len, key, key_size, our_hmac);

    if (memcmp(hmac, our_hmac, HMAC_LEN) != 0)
        return -1;

    return 0;
}