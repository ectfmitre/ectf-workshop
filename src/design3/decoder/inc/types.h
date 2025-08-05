#ifndef __TYPES_H__
#define __TYPES_H__

#include <cstdint>

namespace ectf {

// Data type for storing a 32-bit channel ID
using ChannelID = uint32_t;
// Data type for storing a 32-bit device ID (a.k.a. decoder ID)
using DeviceID = uint32_t;
// Data type for storing a 64-bit timestamp
using Timestamp = uint64_t;
// Data type for storing a flash page number (valid range is 1 to 8)
using PageNumber = uint8_t;

}

#endif // __TYPES_H__
