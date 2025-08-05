/**
 * This file defines a common cryptographic interface for AES, SHA256, and HMAC.
 */

#ifndef CRYPTO_UTILS_H
#define CRYPTO_UTILS_H

#include <stddef.h>
#include <stdint.h>

#define AES128 16
#define AES256 32

/**
 * @brief Decrypt with AES-128-CBC.
 * 
 * @param ciphertext Pointer to the ciphertext data to be decrypted.
 * @param len Length of the ciphertext data.
 * @param key Pointer to the decryption key.
 * @param key_size Size of the decryption key (AES128 or AES256).
 * @param iv Pointer to the initialization vector.
 * @param plaintext Pointer to the buffer where the decrypted data will be stored.
 * @param pt_len Pointer to an integer where the length of the plaintext (excluding padding) will be stored.
 * 
 * @return 0 on success
 */
int decrypt_cbc_sym(uint8_t *ciphertext, size_t len, uint8_t *key, int key_size, uint8_t *iv, uint8_t *plaintext, int *pt_len);

/**
 * @brief Hash data with SHA256.
 * 
 * @param in Pointer to the input data to be hashed.
 * @param len Length of the input data.
 * @param digest Pointer to the buffer where the resulting digest will be stored.
 */
void sha256_hash(uint8_t *in, size_t len, uint8_t *digest);

/**
 * @brief Generate HMAC-SHA-256 digest.
 * 
 * @param in Pointer to the input data.
 * @param len Length of the input data.
 * @param key Pointer to the HMAC key.
 * @param key_size Size of the HMAC key.
 * @param digest Pointer to the buffer where the resulting HMAC digest will be stored.
 */
void hmac_digest(uint8_t *in, size_t len, uint8_t *key, size_t key_size, uint8_t *digest);

/**
 * @brief Verifies HMAC signature.
 * 
 * @param data Pointer to the data to be verified.
 * @param len Length of the data.
 * @param hmac Pointer to the HMAC signature to be verified.
 * @param key Pointer to the HMAC key.
 * @param key_size Size of the HMAC key.
 * 
 * @return 0 on success, -1 on failure
 */
int hmac_verify(uint8_t *data, size_t len, uint8_t *hmac, uint8_t *key, size_t key_size);

#endif