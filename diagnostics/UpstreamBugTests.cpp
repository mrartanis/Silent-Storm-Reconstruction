#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/wMain.h"
#include "../Main/wUnitGroup.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>

static void Require(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}

static void TemporaryGroups()
{
    CObj<NWorld::CWorld> world = new NWorld::CWorld;
    CObj<NWorld::CUnitGroup> temporary = world->CreateUnitGroup(42);
    Require(temporary->GetID() == 42, "temporary group lost its requested ID");
    Require(!world->GetUnitGroup(42), "temporary group leaked into the world registry");
    for (int i = 0; i < 100; ++i) {
        CObj<NWorld::CUnitGroup> query = world->CreateUnitGroup(-1);
        Require(!world->GetUnitGroup(query->GetID()), "query claimed a reinforcement ID");
    }
}

int main(int argc, char** argv)
{
    try {
        const char* mode = argc > 1 ? argv[1] : "temporary-groups";
        if (!std::strcmp(mode, "temporary-groups")) TemporaryGroups();
        else throw std::runtime_error("unknown upstream regression mode");
        std::printf("PASS: %s\n", mode);
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL: %s\n", error.what());
        return 1;
    }
}
