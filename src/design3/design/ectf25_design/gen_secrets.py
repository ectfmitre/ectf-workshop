"""
This source file is part of an example system for MITRE's 2025 Embedded System
CTF (eCTF). 
The gen_secrets module generates symmetric keys and public/private keys for
each channel and a random seed for generating device-specific subscription keys.
The secrets are encoded using the pickle format.
"""

import argparse
import pickle
from Crypto.PublicKey import ECC
from Crypto.Random import get_random_bytes
from loguru import logger
from pathlib import Path


def gen_secrets(channels: list[int]) -> bytes:
	"""Generate the contents secrets file

	This will be passed to the Encoder, ectf25_design.gen_subscription, and the build
	process of the decoder

	:param channels: List of channel numbers that will be valid in this deployment.
		Channel 0 is the emergency broadcast, which will always be valid and will
		NOT be included in this list

	:returns: Contents of the secrets file
	"""
	# List of secrets:
	# - symmetric and priv/pub keys for all channels, including 0
	# - random subscription seed

	secrets = {}
	secrets['channels'] = channels
	secrets['channel_keys'] = {}
	for channel_id in [0] + channels:
		key_data = {}
		key_data['symmetric'] = get_random_bytes(32)
		private_key = ECC.generate(curve='ed25519')
		key_data['private'] = private_key.export_key(format='DER')
		key_data['public'] = private_key.public_key().export_key(format='raw')
		secrets['channel_keys'][channel_id] = key_data
	secrets['sub_seed'] = get_random_bytes(32)
	return pickle.dumps(secrets)


def parse_args():
	"""Define and parse the command line arguments

	NOTE: Your design must not change this function
	"""
	parser = argparse.ArgumentParser()
	parser.add_argument(
		"--force",
		"-f",
		action="store_true",
		help="Force creation of secrets file, overwriting existing file",
	)
	parser.add_argument(
		"secrets_file",
		type=Path,
		help="Path to the secrets file to be created",
	)
	parser.add_argument(
		"channels",
		nargs="+",
		type=int,
		help="Supported channels. Channel 0 (broadcast) is always valid and will not"
		" be provided in this list",
	)
	return parser.parse_args()


def main():
	"""Main function of gen_secrets

	You will likely not have to change this function
	"""
	# Parse the command line arguments
	args = parse_args()

	secrets = gen_secrets(args.channels)

	# Print the generated secrets for your own debugging
	# Attackers will NOT have access to the output of this, but feel free to remove
	#
	# NOTE: Printing sensitive data is generally not good security practice
	logger.debug(f"Generated secrets: {secrets}")

	# Open the file, erroring if the file exists unless the --force arg is provided
	with open(args.secrets_file, "wb" if args.force else "xb") as f:
		# Dump the secrets to the file
		f.write(secrets)

	# For your own debugging. Feel free to remove
	logger.success(f"Wrote secrets to {str(args.secrets_file.absolute())}")


if __name__ == "__main__":
	main()
