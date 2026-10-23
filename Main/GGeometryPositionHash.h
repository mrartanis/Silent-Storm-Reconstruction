#ifndef S2_GGEOMETRY_POSITION_HASH_H
#define S2_GGEOMETRY_POSITION_HASH_H

#include "../Misc/Geom.h"

namespace NGScene
{
struct SGeometryPosHash
{
    int operator()(const CVec3& position) const
    {
        // Retail's XOR fed a prime-sized table. Mix it for masked buckets,
        // preserving full-hash collisions and equal signed-zero positions.
        unsigned int hash = static_cast<unsigned int>(SVec3Hash()(position)) & 0x7fffffffU;
        hash ^= hash >> 16;
        hash *= 0x7feb352dU;
        hash ^= hash >> 15;
        hash *= 0x846ca68bU;
        hash ^= hash >> 16;
        return static_cast<int>(hash);
    }
};
}
#endif
