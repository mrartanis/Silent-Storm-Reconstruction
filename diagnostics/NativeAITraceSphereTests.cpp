#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/aiTrace.h"

#include <cstdio>

int main()
{
	std::vector<NAI::SInterval> intersections;
	NAI::CTracer tracer(intersections);
	tracer.InitProjection(CVec3(1, 2, 3), CVec3(0, 1, 0));
	if (!tracer.TestSphere(CVec3(1, 5, 3), 0.25f)) return 1;
	if (tracer.TestSphere(CVec3(2, 5, 3), 0.5f)) return 2;
	tracer.InitProjection(CVec3(0, 0, 0), CVec3(1, 0, 0));
	if (!tracer.TestSphere(CVec3(5, 0.4f, 0), 0.5f)) return 3;
	std::puts("native AI trace spheres: 1,0,1");
	return 0;
}
