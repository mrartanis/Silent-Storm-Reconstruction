#ifndef __VOXELOBJECTHASH_H_
#define __VOXELOBJECTHASH_H_

#include <cstddef>
#include <cstdint>

namespace NAI
{
// Preserve the retail low-word XOR on x86 while retaining all address bits
// for 64-bit blast-object maps. This is an in-memory lookup hash, not a
// serialized value or a stable object ID.
inline constexpr std::size_t HashVoxelObjectKey( std::uintptr_t address, int userID )
{
	return static_cast<std::size_t>(address) ^
		static_cast<std::size_t>(static_cast<std::uint32_t>(userID));
}
}

#endif // __VOXELOBJECTHASH_H_
