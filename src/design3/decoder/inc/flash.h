#ifndef __FLASH_H__
#define __FLASH_H__

#include <optional>
#include <string_view>

#include "buffer.h"
#include "types.h"

namespace ectf {

// Utility class used to store data to and retrieve data from persistent flash
// storage.
class FlashStorage {
public:
	// Retrieves the byte string stored in a given page of flash.
	static std::optional<SecureString> ReadPage(PageNumber page_num);
	// Stores the given byte string into the specified flash page, overwriting
	// any existing content. The length of the string may not exceed
	// MAX_INPUT_PAYLOAD_SIZE (see message_bus.h).
	static void WritePage(PageNumber page_num, std::string_view data);
};

}

#endif // __FLASH_H__
