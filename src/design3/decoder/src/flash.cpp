#include "flash.h"

#include <cstdint>
#include <cstring>
#include <optional>
#include <string_view>

// from MSDK
#include "flc.h"
#include "icc.h"

#include "buffer.h"
#include "channel.h"
#include "debug.h"
#include "message_bus.h"

namespace {

uint32_t GetPageAddress(ectf::PageNumber page_num) {
	ectf::Debug::Assert(page_num < ectf::MAX_CHANNELS);
	return MXC_FLASH_MEM_BASE + MXC_FLASH_MEM_SIZE
			- (page_num + 1) * MXC_FLASH_PAGE_SIZE;
}

}  // namespace

namespace ectf {

std::optional<SecureString> FlashStorage::ReadPage(PageNumber page_num) {
	const uint32_t page_address = GetPageAddress(page_num);
	uint16_t length;
	MXC_FLC_Read(page_address, &length, sizeof(length));
	// Erased pages are full of FF so length == FFFF means the page is empty,
	// while 0 length means the page was invalidated.
	if (length == 0xFFFF || length == 0) return std::nullopt;
	Debug::Assert(length <= MAX_INPUT_PAYLOAD_SIZE);
	SecureString ret(length);
	MXC_FLC_Read(page_address + sizeof(length), ret.data(), length);
	return ret;
}

void FlashStorage::WritePage(PageNumber page_num, std::string_view data) {
	const int page_address = GetPageAddress(page_num);
	const uint16_t length = data.size();
	Debug::Assert(length <= MAX_INPUT_PAYLOAD_SIZE,
			"Flash write size too large");
	SecureString buffer(length + sizeof(length));
	std::memcpy(buffer.data(), &length, sizeof(length));
	std::memcpy(buffer.data() + sizeof(length), data.data(), length);
	MXC_ICC_Disable(MXC_ICC0);
	int retcode = MXC_FLC_PageErase(page_address);
	Debug::Assert(retcode == E_NO_ERROR, "Failed to erase flash");
	retcode = MXC_FLC_Write(page_address, buffer.size(),
			(uint32_t*) buffer.data());
	MXC_ICC_Enable(MXC_ICC0);
	Debug::Assert(retcode == E_NO_ERROR, "Failed to write to flash");
}

}  // namespace ectf
