# This file can be used to set build configuration
# variables.  These variables are defined in a file called 
# "Makefile" that is located next to this one.

# For instructions on how to use this system, see
# https://analog-devices-msdk.github.io/msdk/USERGUIDE/#build-system

#MXC_OPTIMIZE_CFLAGS = -Og
# ^ For example, you can uncomment this line to 
# optimize the project for debugging

# **********************************************************

# Add your config here!

# NOTE: In order to build, global secrets must be generated as global.secrets in this directory

GLOBAL_SECRETS := ../global.secrets

GLOBAL_SECRETS_CHANNELS := $(shell python3 -c 'import json; print("{" + ", ".join(map(str, json.load(open("$(GLOBAL_SECRETS)"))["channels"])) + "}")')
GLOBAL_SECRETS_SUBUPDATE_SALT := $(shell python3 -c 'import json; print(json.load(open("$(GLOBAL_SECRETS)"))["subupdate_salt"])')
GLOBAL_SECRETS_HMAC_AUTH_KEY := $(shell python3 -c 'import json; print(json.load(open("$(GLOBAL_SECRETS)"))["hmac_auth_key"])')
GLOBAL_SECRETS_EMERGENCY_KEY := $(shell python3 -c 'import json; print(json.load(open("$(GLOBAL_SECRETS)"))["emergency_key"])')

# Logging the secrets for debugging purposes (bad practice, but attackers wont have access)
$(info GLOBAL_SECRETS_CHANNELS=$(GLOBAL_SECRETS_CHANNELS))
$(info GLOBAL_SECRETS_SUBUPDATE_SALT=$(GLOBAL_SECRETS_SUBUPDATE_SALT))
$(info GLOBAL_SECRETS_HMAC_AUTH_KEY=$(GLOBAL_SECRETS_HMAC_AUTH_KEY))
$(info GLOBAL_SECRETS_EMERGENCY_KEY=$(GLOBAL_SECRETS_EMERGENCY_KEY))

$(info DECODER_ID=$(DECODER_ID))

# To enable WolfSSL features that we use
PROJ_CFLAGS += -DHAVE_PKCS7
PROJ_CFLAGS += -DHAVE_AES_KEYWRAP

PROJ_CFLAGS += -g

inc/global.secrets.h: $(GLOBAL_SECRETS)
	@echo "Generating header global.secrets.h"
	@echo "/* This file is auto-generated. Do not modify. */" > $@
	@echo "#ifndef GTONE_SECRET_H" >> $@
	@echo "#define GTONE_SECRET_H" >> $@
	@echo "#define SECRET_CHANNELS $(GLOBAL_SECRETS_CHANNELS)" >> $@
	@python3 -c 'import base64, sys; data = base64.b64decode(sys.argv[1]); print("#define SECRET_SUBUPDATE_SALT { " + ", ".join("0x{:02x}".format(b) for b in data) + " }")' $(GLOBAL_SECRETS_SUBUPDATE_SALT) >> $@
	@python3 -c 'import base64, sys; data = base64.b64decode(sys.argv[1]); print("#define SECRET_HMAC_AUTH_KEY { " + ", ".join("0x{:02x}".format(b) for b in data) + " }")' $(GLOBAL_SECRETS_HMAC_AUTH_KEY) >> $@
	@python3 -c 'import base64, sys; data = base64.b64decode(sys.argv[1]); print("#define SECRET_EMERGENCY_KEY { " + ", ".join("0x{:02x}".format(b) for b in data) + " }")' $(GLOBAL_SECRETS_EMERGENCY_KEY) >> $@
	@echo "#endif // GTONE_SECRET_H" >> $@

# This example is only compatible with the FTHR board,
# so we override the BOARD value to hard-set it.
override BOARD=FTHR_RevA
MFLOAT_ABI=soft

IPATH+=../deployment
IPATH+=inc/
VPATH+=src/

# ****************** eCTF Bootloader *******************
# DO NOT REMOVE
LINKERFILE=firmware.ld
STARTUPFILE=startup_firmware.S
ENTRY=firmware_startup

# ****************** eCTF Crypto Example *******************
# Uncomment the commented lines below and comment the disable
# lines to enable the eCTF Crypto Example.
# WolfSSL must be included in this directory as wolfssl/
# WolfSSL can be downloaded from: https://www.wolfssl.com/download/

# Disable Crypto Example
#CRYPTO_EXAMPLE=0

# Enable Crypto Example
CRYPTO_EXAMPLE=1
