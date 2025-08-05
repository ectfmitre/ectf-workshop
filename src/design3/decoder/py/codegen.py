import pickle
import sys
from Crypto.Cipher import ChaCha20_Poly1305
from Crypto.Hash import BLAKE2b
from Crypto.Hash import SHA3_256
from Crypto.PublicKey import ECC
from Crypto.Random import get_random_bytes
from Crypto.Random import random

def GenerateDeterministicSymmetricKeyRaw(sub_seed: bytes, device_id: int) -> bytes:
	blake2b = BLAKE2b.new(digest_bits=256)
	blake2b.update(device_id.to_bytes(4) + sub_seed)
	return blake2b.digest()

def GenerateDeterministicECCKey(sub_seed: bytes, device_id: int):
	sha3_256 = SHA3_256.new()
	sha3_256.update(device_id.to_bytes(4) + sub_seed)
	ecc_seed = sha3_256.digest()
	return ECC.construct(curve='ed25519', seed=ecc_seed)

def RandomSalt(min_length: int, max_length: int) -> bytes:
	n = random.randint(min_length, max_length)
	return n.to_bytes(1) + get_random_bytes(n)

def Escape(byte_str: bytes) -> str:
	return ''.join('\\%03o' % c for c in byte_str)

def MakeFunction(func_name: str, data: bytes) -> str:
	return 'std::string_view %s() { return {"%s", %d}; }\n' % (func_name, Escape(data), len(data))

def main():
	device_id = int(sys.argv[1], 0)
	with open('/global.secrets', 'rb') as f:
		secret_data = pickle.loads(f.read())
	channel0_keys = secret_data['channel_keys'][0]
	subscription_seed = secret_data['sub_seed']
	subscription_priv_key = GenerateDeterministicECCKey(subscription_seed, device_id)
	subscription_pub_key_raw = subscription_priv_key.public_key().export_key(format='raw')
	subscription_symmetric_key_raw = GenerateDeterministicSymmetricKeyRaw(subscription_seed, device_id)
	flash_key_raw = get_random_bytes(32)
	flash_iv = get_random_bytes(12)
	flash_key = ChaCha20_Poly1305.new(key=flash_key_raw, nonce=flash_iv)
	
	payload = b''
	payload += device_id.to_bytes(4, 'little')
	payload += channel0_keys['symmetric']
	payload += channel0_keys['public']
	payload += subscription_symmetric_key_raw
	payload += subscription_pub_key_raw
	ciphertext, tag = flash_key.encrypt_and_digest(payload)
	cipher_len = len(ciphertext)
	secret_string = RandomSalt(50, 80) + cipher_len.to_bytes(2, 'little') + ciphertext + tag + RandomSalt(50, 80)
	
	with open('/root/gencode/secret_data.cpp', 'w') as f:
		f.write('#include <string_view>\n')
		f.write('namespace ectf {\n')
		f.write(MakeFunction('GetFlashKey', flash_key_raw))
		f.write(MakeFunction('GetFlashIV', flash_iv))
		f.write(MakeFunction('GetFlashSecretData', secret_string))
		f.write('}\n')

if __name__ == '__main__':
	main()
