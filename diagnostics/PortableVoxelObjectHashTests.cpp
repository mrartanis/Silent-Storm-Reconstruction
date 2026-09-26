#include "../Main/VoxelObjectHash.h"

#include <cstdint>
#include <cstdio>

int main()
{
	constexpr std::uintptr_t low = UINT32_C(0x81234567);
	constexpr int id = -17;
	constexpr std::size_t expectedLow =
		static_cast<std::size_t>(low) ^
		static_cast<std::size_t>(static_cast<std::uint32_t>(id));
	if ( NAI::HashVoxelObjectKey(low, id) != expectedLow ) return 1;
	if ( NAI::HashVoxelObjectKey(low, id) == NAI::HashVoxelObjectKey(low, id + 1) ) return 2;
	if ( sizeof(std::uintptr_t) == 8 )
	{
		const std::uintptr_t high = low | (UINT64_C(1) << 40);
		if ( NAI::HashVoxelObjectKey(low, id) == NAI::HashVoxelObjectKey(high, id) ) return 3;
	}
	std::printf("voxel_hash_bits=%zu low=%zx\n", sizeof(std::uintptr_t) * 8,
		NAI::HashVoxelObjectKey(low, id));
	return 0;
}
