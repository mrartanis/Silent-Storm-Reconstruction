#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/aiInterval.h"

#include <cstdio>

int main()
{
	NAI::SSourceInfo source;
	std::vector<float> enters{4.0f, 1.0f};
	std::vector<float> exits{5.0f, 2.0f};
	std::vector<NAI::SSimpleInterval> result;
	NAI::FillIntersectionResults(&result, &enters, &exits, source, 17, false);
	if (result.size() != 2 || result[0].fEnter != 1.0f ||
		result[0].fExit != 2.0f || result[1].fEnter != 4.0f ||
		result[1].fExit != 5.0f || result[0].nUserID != 17) return 1;

	enters = {3.0f};
	exits = {1.0f, 5.0f};
	result.clear();
	NAI::FillIntersectionResults(&result, &enters, &exits, source, 23, true);
	if (result.size() != 2 || result[0].fEnter > -1.0e9f ||
		result[0].fExit != 1.0f || result[1].fEnter != 3.0f ||
		result[1].fExit != 5.0f || result[1].nUserID != 23) return 2;

	std::puts("native AI intervals: sorted pairs and terrain entry");
	return 0;
}
