/**
 * @file    decoder.c
 * @author  Samuel Meyers
 * @brief   eCTF Decoder Example Design Implementation
 * @date    2025
 *
 * This source file is part of an example system for MITRE's 2025 Embedded System CTF (eCTF).
 * This code is being provided only for educational purposes for the 2025 MITRE eCTF competition,
 * and may not meet MITRE standards for quality. Use this code at your own risk!
 *
 * @copyright Copyright (c) 2025 The MITRE Corporation
 */

/*********************** INCLUDES *************************/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "mxc_device.h"
#include "status_led.h"
#include "board.h"
#include "mxc_delay.h"
#include "simple_flash.h"
#include "host_messaging.h"

#include "simple_uart.h"

#include "crypto_utils.h"
#include "global.secrets.h"

#ifndef DECODER_ID
#error "DECODER_ID is not defined"
#endif

#define DEVICE_ID ((decoder_id_t) DECODER_ID)

#define EMERGENCY_RECEIVED (0xff)

#define UART_BUF_MAX 256

/**
 * Bit masking macros to modify and check the 8-bit channel status.
 */
#define CHANNEL_RECEIVED(status, c) ((status & (1U << c)) != 0)
#define SET_CHANNEL_RECEIVED(status, c) (status |= (1U << c))

/**
 * Primitive types
 */
#define timestamp_t uint64_t
#define channel_id_t uint32_t
#define decoder_id_t uint32_t
#define pkt_len_t uint16_t

// HMAC-SHA256 signature
typedef struct {
    uint8_t bytes[32];
} hmac_sig_t;

// 128-bit IV
typedef struct {
    uint8_t bytes[16];
} iv_t;

// 128-bit channel key
typedef struct {
    uint8_t bytes[16];
} channel_key_t;

/**
 * Constants
 */
#define MAX_CHANNEL_COUNT 8
#define EMERGENCY_CHANNEL 0
#define FRAME_SIZE 64
#define DEFAULT_CHANNEL_TIMESTAMP 0xFFFFFFFFFFFFFFFF
// This is a canary value so we can confirm whether this decoder has booted before
#define FLASH_FIRST_BOOT 0xDEADBEEF
#define HASH_SIZE 32

/**
 * State Macros
 */

// Calculate the flash address where we will store channel info as the 2nd to last page available
#define FLASH_STATUS_ADDR ((MXC_FLASH_MEM_BASE + MXC_FLASH_MEM_SIZE) - (2 * MXC_FLASH_PAGE_SIZE))


/**********************************************************
 *********** COMMUNICATION PACKET DEFINITIONS *************
 **********************************************************/

#pragma pack(push, 1) // Tells the compiler not to pad the struct members
// for more information on what struct padding does, see:
// https://www.gnu.org/software/c-intro-and-ref/manual/html_node/Structure-Layout.html

// Decrypted frame body contents
typedef struct {
    timestamp_t timestamp;
    uint8_t data[FRAME_SIZE];
    char padding[8]; // Expected padding bytes
} frame_packet_payload_t;

typedef struct {
    channel_id_t channel;
    hmac_sig_t hmac_signature;
    iv_t iv;
    uint8_t encrypted_data[sizeof(frame_packet_payload_t)];
} frame_packet_t;

// Decrypted subscription body contents
typedef struct {
    decoder_id_t device_id;
    timestamp_t start;
    timestamp_t end;
    channel_id_t channel;
    channel_key_t channel_key;
    char padding[8];
} subscription_update_payload_t;

typedef struct {
    hmac_sig_t hmac_signature;
    iv_t iv;

    // device_id, start, end, channel, channel key
    uint8_t encrypted_data[sizeof(subscription_update_payload_t)];
} subscription_update_packet_t;

typedef struct {
    channel_id_t channel;
    timestamp_t start;
    timestamp_t end;
} channel_info_t;

typedef struct {
    uint32_t n_channels;
    channel_info_t channel_info[MAX_CHANNEL_COUNT];
} list_response_t;

#pragma pack(pop) // Tells the compiler to resume padding struct members

/**
 * Type definitions
 */

typedef struct {
    uint32_t channels[MAX_CHANNEL_COUNT];
    uint8_t subupdate_salt[16];
    uint8_t hmac_auth_key[32];
    uint8_t emergency_key[16];
} secrets_t;

typedef struct {
    bool active;
    channel_id_t id;
    timestamp_t start_timestamp;
    timestamp_t end_timestamp;
    channel_key_t key;
} channel_status_t;

typedef struct {
    uint32_t first_boot; // if set to FLASH_FIRST_BOOT, device has booted before.
    channel_status_t subscribed_channels[MAX_CHANNEL_COUNT];
} flash_entry_t;

/**
 * Globals
 */

// This is used to track decoder subscriptions
flash_entry_t decoder_status;
// This tracks the last timestamp for emergency channel
static timestamp_t last_emergency_timestamp = 0;
// This holds the last timestamp for each channel. 
static timestamp_t last_frame_timestamps[MAX_CHANNEL_COUNT] = {0};

// Bitfield to track if a frame has been received for each channel
// has_received_frame[0] is normal channels, has_received_frame[1] is emergency channel
static char has_received_frame[2] = {0};

// The global secrets
static const secrets_t secrets = {
    .channels = SECRET_CHANNELS,
    .subupdate_salt = SECRET_SUBUPDATE_SALT,
    .hmac_auth_key = SECRET_HMAC_AUTH_KEY,
    .emergency_key = SECRET_EMERGENCY_KEY,
};

/**
 * Utility functions
 */

/** @brief Checks whether the decoder is subscribed to a given channel
 *
 *  @param channel The channel number to be checked.
 *  @return 1 if the the decoder is subscribed to the channel.  0 if not.
*/
int is_subscribed(channel_id_t channel) {
    // Check if this is an emergency broadcast message
    if (channel == EMERGENCY_CHANNEL) {
        return 1;
    }
    // Check if the decoder has has a subscription
    for (int i = 0; i < MAX_CHANNEL_COUNT; i++) {
        if (decoder_status.subscribed_channels[i].id == channel && decoder_status.subscribed_channels[i].active) {
            return 1;
        }
    }

    return 0;
}

/**
 * @brief Check to see if the channel is a valid channel
 * 
 * @param channel ID of channel
 * 
 * @return 1 if valid. 0 if invalid
 */
int is_valid_channel(channel_id_t channel) {
    for (int i = 0; i < sizeof(secrets.channels); i++) {
        if (secrets.channels[i] == channel) {
            return 1;
        }
    }

    return 0;
}

/**
 * Find subscription in list of subscription entries.
 * 
 * @param channel ID of the channel
 * 
 * @return pointer to subscription, or NULL if not found
 */
channel_status_t *find_subscription(channel_id_t channel) {
    for (int i = 0; i < MAX_CHANNEL_COUNT; i++) {
        if (decoder_status.subscribed_channels[i].id == channel && decoder_status.subscribed_channels[i].active) {
            return &decoder_status.subscribed_channels[i];
        }
    }

    return NULL;
}


/**********************************************************
 ********************* CORE FUNCTIONS *********************
 **********************************************************/

/** @brief Lists out the actively subscribed channels over UART.
 *
 *  @return 0 if successful.
*/
int list_channels() {
    list_response_t resp;
    pkt_len_t len;

    resp.n_channels = 0;

    for (uint32_t i = 0; i < MAX_CHANNEL_COUNT; i++) {
        if (decoder_status.subscribed_channels[i].active) {
            resp.channel_info[resp.n_channels].channel =  decoder_status.subscribed_channels[i].id;
            resp.channel_info[resp.n_channels].start = decoder_status.subscribed_channels[i].start_timestamp;
            resp.channel_info[resp.n_channels].end = decoder_status.subscribed_channels[i].end_timestamp;
            resp.n_channels++;
        }
    }

    len = sizeof(resp.n_channels) + (sizeof(channel_info_t) * resp.n_channels);

    // Success message
    write_packet(LIST_MSG, &resp, len);
    return 0;
}


/** @brief Updates the channel subscription for a subset of channels.
 *
 *  @param pkt_len The length of the incoming packet
 *  @param update A pointer to an array of channel_update structs,
 *      which contains the channel number, start, and end timestamps
 *      for each channel being updated.
 *
 *  @note Take care to note that this system is little endian.
 *
 *  @return 0 upon success.  -1 if error.
*/
int update_subscription(pkt_len_t pkt_len, subscription_update_packet_t *update) {
    int i;

    int hmac_status = hmac_verify(update->encrypted_data, sizeof(update->encrypted_data), update->hmac_signature.bytes, secrets.hmac_auth_key, sizeof(secrets.hmac_auth_key));
    if (hmac_status != 0) {
        STATUS_LED_RED();
        print_error("Failed to update subscription - HMAC verification failed\n");
        return -1;
    }

    char prehash[sizeof(decoder_id_t) + sizeof(secrets.subupdate_salt)];
    char subupdate_key[HASH_SIZE];
    subscription_update_payload_t payload;
    
    // IMPORTANT - Zero out stack variables to prevent stack-based attacks!!!
#define ZERO_PRIVATES() do { \
    memset(prehash, 0, sizeof(prehash)); \
    memset(subupdate_key, 0, sizeof(subupdate_key)); \
    memset(&payload, 0, sizeof(subscription_update_payload_t)); \
} while (0)
    
    ((decoder_id_t *)prehash)[0] = DEVICE_ID;
    memcpy(prehash + sizeof(decoder_id_t), secrets.subupdate_salt, sizeof(secrets.subupdate_salt));

    // Hash the prehash to get the key
    sha256_hash(prehash, sizeof(prehash), subupdate_key);

    // Decrypt the sub update
    int payload_size;
    int result = decrypt_cbc_sym(update->encrypted_data, sizeof(subscription_update_payload_t), subupdate_key, AES256, update->iv.bytes, (uint8_t *)&payload, &payload_size);

    if (result != 0) {
        ZERO_PRIVATES();
        STATUS_LED_RED();
        print_error("Failed to update subscription - decryption failed\n");
        return -1;
    }

    if (payload.channel == EMERGENCY_CHANNEL) {
        ZERO_PRIVATES();
        STATUS_LED_RED();
        print_error("Failed to update subscription - cannot subscribe to emergency channel\n");
        return -1;
    }

    if (!is_valid_channel(payload.channel)) {
        ZERO_PRIVATES();
        STATUS_LED_RED();
        print_error("Failed to update subscription - invalid channel\n");
        return -1;
    }

    // Find the first empty slot in the subscription array
    for (i = 0; i < MAX_CHANNEL_COUNT; i++) {
        if (decoder_status.subscribed_channels[i].id == payload.channel || !decoder_status.subscribed_channels[i].active) {
            decoder_status.subscribed_channels[i].id = payload.channel;
            decoder_status.subscribed_channels[i].start_timestamp = payload.start;
            decoder_status.subscribed_channels[i].end_timestamp = payload.end;
            decoder_status.subscribed_channels[i].active = true;
            memcpy(decoder_status.subscribed_channels[i].key.bytes, payload.channel_key.bytes, sizeof(payload.channel_key.bytes));

            break;
        }
    }

    // If we do not have any room for more subscriptions
    if (i == MAX_CHANNEL_COUNT) {
        ZERO_PRIVATES();
        STATUS_LED_RED();
        print_error("Failed to update subscription - max subscriptions installed\n");
        return -1;
    }

    ZERO_PRIVATES();
#undef ZERO_PRIVATES

    flash_simple_erase_page(FLASH_STATUS_ADDR);
    flash_simple_write(FLASH_STATUS_ADDR, &decoder_status, sizeof(flash_entry_t));
    // Success message with an empty body
    write_packet(SUBSCRIBE_MSG, NULL, 0);
    print_debug("Subscription successfully decoded!\n");
    return 0;
}

/**
 * Verify subscription time window
 */
int check_subscription(channel_id_t channel, timestamp_t *time) {
    channel_status_t *channel_status = find_subscription(channel);

    if (channel_status == NULL) {
        return 0;
    }

    return *time >= channel_status->start_timestamp && *time <= channel_status->end_timestamp;
}

/** @brief Processes a packet containing frame data.
 *
 *  @param pkt_len A pointer to the incoming packet.
 *  @param new_frame A pointer to the incoming packet.
 *
 *  @return 0 if successful.  -1 if data is from unsubscribed channel.
*/
int decode(pkt_len_t pkt_len, frame_packet_t *new_frame) {
    char output_buf[128] = {0};
    uint16_t payload_size;
    channel_id_t channel;
    frame_packet_payload_t payload;

#define ZERO_PRIVATES() do { \
    payload_size = 0; \
    memset(&payload, 0, sizeof(frame_packet_payload_t)); \
} while (0)

    // Frame size is the size of the packet minus the size of non-frame elements
    payload_size = pkt_len - (sizeof(new_frame->channel) + sizeof(new_frame->hmac_signature) + sizeof(new_frame->iv));
    channel = new_frame->channel;

    // Check that we are subscribed to the channel...
    print_debug("Checking subscription\n");
    if (is_subscribed(channel)) {
        print_debug("Subscription Valid\n");
        
        int result;
        int hmac_status = hmac_verify(new_frame->encrypted_data, sizeof(new_frame->encrypted_data), new_frame->hmac_signature.bytes, secrets.hmac_auth_key, sizeof(secrets.hmac_auth_key));
        
        if (hmac_status != 0) {
            ZERO_PRIVATES();
            STATUS_LED_RED();
            print_error("Failed to decode - HMAC verification failed\n");
            return -1;
        }

        channel_key_t key;
        if (channel == EMERGENCY_CHANNEL) {
            memcpy(&key.bytes, secrets.emergency_key, sizeof(secrets.emergency_key));
        } else {
            channel_status_t *channel_status = find_subscription(channel);
            if (channel_status == NULL) {
                ZERO_PRIVATES();
                STATUS_LED_RED();
                print_error("Failed to decode - channel not found\n");
                return -1;
            }
            key = channel_status->key;
        }

        // pt_len is the length of the decrypted payload
        int pt_len;

        result = decrypt_cbc_sym(
            new_frame->encrypted_data,
            payload_size, 
            key.bytes,
            AES128,
            new_frame->iv.bytes, 
            (uint8_t *)&payload, 
            &pt_len
        );

        if (result != 0) {
            ZERO_PRIVATES();
            STATUS_LED_RED();
            print_error("Failed to decode - decryption failed\n");
            return -1;
        }

        if (!check_subscription(channel, &payload.timestamp) && channel != EMERGENCY_CHANNEL) {
            ZERO_PRIVATES();
            STATUS_LED_RED();
            print_error("Failed to decode - subscription expired\n");
            return -1;
        }

        // Check if emergency channel is monotonically increasing
        if (channel == EMERGENCY_CHANNEL) {
            if (payload.timestamp <= last_emergency_timestamp && has_received_frame[1] == EMERGENCY_RECEIVED) {
                ZERO_PRIVATES();
                print_error("Failed to decode - emergency frame timestamp not strictly increasing\n");
                return -1;
            }   
            // Update the emergency channel's last timestamp
            last_emergency_timestamp = payload.timestamp;
            has_received_frame[1] = EMERGENCY_RECEIVED;
        } else { // Check if standard channels are monotonically increasing

            // Find index in the subscription array
            int idx;
            for (idx = 0; idx < MAX_CHANNEL_COUNT + 1; idx++) {
                // Reject if subscription not found
                if (idx == MAX_CHANNEL_COUNT) {
                    ZERO_PRIVATES();
                    STATUS_LED_RED();
                    print_error("Failed to decode - verify timestamp: subscription not found for channel\n");
                    return -1;
                }

                if (decoder_status.subscribed_channels[idx].active && decoder_status.subscribed_channels[idx].id == channel) {
                    break;
                }
            }

            // enforce strictly monotonically increasing timestamps
            int channel_received = CHANNEL_RECEIVED(has_received_frame[0], idx);
            if (payload.timestamp <= last_frame_timestamps[idx] && channel_received) {
                ZERO_PRIVATES();
                STATUS_LED_RED();
                print_error("Failed to decode - timestamp not strictly increasing\n");
                return -1;
            }
            last_frame_timestamps[idx] = payload.timestamp;
            SET_CHANNEL_RECEIVED(has_received_frame[0], idx);
        }

        // Return decoded frame
        write_packet(DECODE_MSG, payload.data, pt_len - sizeof(timestamp_t));
        return 0;
    } else {
        ZERO_PRIVATES();
        STATUS_LED_RED();
        sprintf(
            output_buf,
            "Receiving unsubscribed channel data.  %u\n", channel);
        print_error(output_buf);
        return -1;
    }
}

/** @brief Initializes peripherals for system boot.
*/
void init() {
    int ret;

    // Initialize the flash peripheral to enable access to persistent memory
    flash_simple_init();

    // Read starting flash values into our flash status struct
    flash_simple_read(FLASH_STATUS_ADDR, &decoder_status, sizeof(flash_entry_t));
    if (decoder_status.first_boot != FLASH_FIRST_BOOT) {
        /* If this is the first boot of this decoder, mark all channels as unsubscribed.
        *  This data will be persistent across reboots of the decoder. Whenever the decoder
        *  processes a subscription update, this data will be updated.
        */
        print_debug("First boot.  Setting flash...\n");

        decoder_status.first_boot = FLASH_FIRST_BOOT;

        channel_status_t subscription[MAX_CHANNEL_COUNT];

        for (int i = 0; i < MAX_CHANNEL_COUNT; i++){
            subscription[i].start_timestamp = DEFAULT_CHANNEL_TIMESTAMP;
            subscription[i].end_timestamp = DEFAULT_CHANNEL_TIMESTAMP;
            subscription[i].active = false;
        }

        // Write the starting channel subscriptions into flash.
        memcpy(decoder_status.subscribed_channels, subscription, MAX_CHANNEL_COUNT*sizeof(channel_status_t));

        flash_simple_erase_page(FLASH_STATUS_ADDR);
        flash_simple_write(FLASH_STATUS_ADDR, &decoder_status, sizeof(flash_entry_t));
    }
    
    // Initialize the uart peripheral to enable serial I/O
    ret = uart_init();
    if (ret < 0) {
        STATUS_LED_ERROR();
        // if uart fails to initialize, do not continue to execute
        while (1);
    }
}

/**********************************************************
 *********************** MAIN LOOP ************************
 **********************************************************/

int main(void) {
    char output_buf[128] = {0};
    uint8_t uart_buf[UART_BUF_MAX];
    msg_type_t cmd;
    int result;
    uint16_t pkt_len;
    
    // initialize the device
    init();

    print_debug("Decoder Booted!\n");

    // process commands forever
    while (1) {
        print_debug("Ready\n");

        STATUS_LED_GREEN();

        result = read_packet(&cmd, uart_buf, &pkt_len);

        if (result < 0) {
            STATUS_LED_ERROR();
            print_error("Failed to receive cmd from host\n");
            continue;
        }

        // Handle the requested command
        switch (cmd) {

        // Handle list command
        case LIST_MSG:
            STATUS_LED_CYAN();

            list_channels();
            break;

        // Handle decode command
        case DECODE_MSG:
            STATUS_LED_PURPLE();
            decode(pkt_len, (frame_packet_t *)uart_buf);
            break;

        // Handle subscribe command
        case SUBSCRIBE_MSG:
            STATUS_LED_YELLOW();
            update_subscription(pkt_len, (subscription_update_packet_t *)uart_buf);
            break;

        // Handle bad command
        default:
            STATUS_LED_ERROR();
            sprintf(output_buf, "Invalid Command: %c\n", cmd);
            print_error(output_buf);
            break;
        }
    }
}
