import argparse
import struct
import re
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import padding

import tempfile
import os


def parse_secrets_file(secrets_path):
    """Parses the `global.secrets` file (C header format) and extracts encryption keys.

    Args:
        secrets_path (str): Path to the `global.secrets` file.

    Returns:
        dict: Dictionary with `channel_keys` (dict of channel IDs to AES keys) and `secret_key` (bytes).

    Raises:
        ValueError: If the `secret_key` or `channel_keys` cannot be extracted.
    """
    secrets_dict = {"channel_keys": {}}

    with open(secrets_path, "r") as f:
        content = f.read()

    # Extract all channel keys
    channel_keys_match = re.findall(r"\{\s*((?:0x[0-9A-Fa-f]+,\s*){15}0x[0-9A-Fa-f]+)\s*\}", content)

    if not channel_keys_match:
        raise ValueError("Error: Unable to extract channel keys from secrets file.")

    for index, key_string in enumerate(channel_keys_match):
        # Remove extra spaces and split by ","
        key_bytes = bytes(int(b, 16) for b in key_string.replace(" ", "").split(","))
        secrets_dict["channel_keys"][index] = key_bytes  # Channels start from 1

    # Extract the master secret key
    secret_key_match = re.search(
        r"static const uint8_t secret_key\[16\] = \{\s*((?:0x[0-9A-Fa-f]+,\s*){15}0x[0-9A-Fa-f]+)\s*\}", 
        content
    )

    if not secret_key_match:
        raise ValueError("Error: Unable to extract secret_key from secrets file.")

    key_string = secret_key_match.group(1)
    secrets_dict["secret_key"] = bytes(int(b, 16) for b in key_string.replace(" ", "").split(","))

    return secrets_dict


class Encoder:
    """Handles encryption-based encoding of data frames for secure transmission.

    This class encrypts frames using a two-layer encryption process:
    1. First encryption layer with a channel-specific key.
    2. Second encryption layer with a master secret key.

    Attributes:
        channel_keys (dict[int, bytes]): A dictionary mapping channel IDs to AES keys.
        secret_key (bytes): The master AES encryption key used for final encryption.
    """

    def __init__(self, secrets_content: bytes):  # Changed to accept secrets content
        """Initializes the Encoder with encryption keys.

        Args:
            secrets_content (bytes): The content of the secrets file as bytes.

        Raises:
            ValueError: If the secrets file is improperly formatted or missing required keys.
        """

        # Create a temporary file
        with tempfile.NamedTemporaryFile(mode='w+b', delete=False, suffix=".secrets") as tmp_file: # Open in binary write mode
            tmp_file.write(secrets_content)
            tmp_file_path = tmp_file.name

        try:
            secrets = parse_secrets_file(tmp_file_path)
            self.channel_keys = secrets["channel_keys"]
            self.secret_key = secrets["secret_key"]
        except Exception as e:
            print(f"Error parsing secrets: {e}")
            raise  # Re-raise the exception to signal failure
        finally:
            # Clean up the temporary file
            os.remove(tmp_file_path)

    def _encrypt(self, data: bytes, key: bytes) -> bytes:
        """Encrypts data using AES-ECB mode.

        Args:
            data (bytes): The data to be encrypted.
            key (bytes): The AES key used for encryption.

        Returns:
            bytes: The encrypted data.
        """
        cipher = Cipher(algorithms.AES(key), modes.ECB(), backend=default_backend())
        encryptor = cipher.encryptor()
        encrypted_data = encryptor.update(data) + encryptor.finalize()
        return encrypted_data

    def encode(self, channel: int, frame: bytes, timestamp: int) -> bytes:
        """Encodes and encrypts a frame for transmission.

        This method performs the following steps:
        1. Validates the frame size.
        2. Computes the effective channel ID.
        3. Retrieves the appropriate channel key or uses the master secret key.
        4. Pads the frame to a multiple of 16 bytes.
        5. Encrypts the frame using the channel key.
        6. Constructs a packet header with channel, timestamp, and frame size.
        7. Encrypts the entire packet using the master secret key.

        Args:
            channel (int): The channel number for which the frame is encoded.
            frame (bytes): The raw frame data. Must not exceed 64 bytes.
            timestamp (int): A 64-bit timestamp.

        Returns:
            bytes: The fully encrypted packet.

        Raises:
            ValueError: If the frame exceeds the allowed size or if no encryption key is found.
        """
        if len(frame) > 64:
            raise ValueError("Frame size must not exceed 64 bytes.")

        frame_size = len(frame)
  
        effective_channel = channel % 10007
        # if channel is emergency channel use secret_key
        if channel == 0:
            channel_key = self.secret_key
        # otherwise use respective key (channel % 10007)
        else:
            channel_key = self.channel_keys[effective_channel]
            

        # Pad the frame to a multiple of 16 bytes
        if frame_size % 16 != 0:
            frame_padding = 16 - (frame_size % 16)
            frame += b"\x80" + b"\x00" * (frame_padding - 1)

        encrypted_frame = self._encrypt(frame, channel_key)

        # Construct packet header
        header = struct.pack("<IQI", channel, timestamp, frame_size)

        full_packet = header + encrypted_frame

        # Pad the full packet to a multiple of 16 bytes
        if len(full_packet) % 16 != 0:
            padding_length = 16 - (len(full_packet) % 16)
            full_packet += b"\x80" + b"\x00" * (padding_length - 1)

        encrypted_packet = self._encrypt(full_packet, self.secret_key)

        return encrypted_packet


def main():
    """Main function to encode a frame using command-line arguments.

    This function:
    1. Parses command-line arguments for the secrets file, channel, frame, and timestamp.
    2. Initializes the Encoder with the provided secrets.
    3. Encodes the frame using the Encoder class.
    4. Prints the encoded packet in byte format.

    Command-line Arguments:
        secrets_file (str): Path to the `global.secrets` file.
        channel (int): Channel ID for encoding.
        frame (str): Raw frame data (string format).
        timestamp (int): 64-bit timestamp.
    """
    parser = argparse.ArgumentParser(prog="ectf25_design.encoder")
    parser.add_argument(
        "secrets_file", type=str, help="Path to the global.secrets file"
    )
    parser.add_argument("channel", type=int, help="Channel to encode for")
    parser.add_argument("frame", help="Contents of the frame")
    parser.add_argument("timestamp", type=int, help="64b timestamp to use")
    args = parser.parse_args()

    # Adapt the following line:
    with open(args.secrets_file, 'rb') as f: #Open in binary read mode
        secrets_content = f.read()
    encoder = Encoder(secrets_content) #This line needs to be adapted if you use this main function separately
    print(repr(encoder.encode(args.channel, args.frame.encode(), args.timestamp)))


if __name__ == "__main__":
    main()
