#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/GAnimPath.h"

#include <cmath>
#include <cstdio>
#include <cstring>

static bool Near(float a, float b)
{
	return std::fabs(a - b) < 0.0005f;
}

int main()
{
	using PathPoint = NAnimation::CPathInterpolator::SPathPoint;
	using PathPointCodec = S2FileIO::StructureFieldCodec<PathPoint>;
	if (!PathPointCodec::kPortable || PathPointCodec::kWireSize != 20) return 3;
	PathPoint wirePoint{1.0f, CVec3(2.0f, 3.0f, 4.0f), -0.0f};
	const std::uint8_t expectedPoint[20] = {
		0, 0, 0x80, 0x3f, 0, 0, 0, 0x40, 0, 0, 0x40, 0x40,
		0, 0, 0x80, 0x40, 0, 0, 0, 0x80};
	std::uint8_t pointBytes[20] = {};
	PathPoint decodedPoint{};
	if (!PathPointCodec::Encode(wirePoint, pointBytes, sizeof(pointBytes)) ||
		std::memcmp(pointBytes, expectedPoint, sizeof(pointBytes)) != 0 ||
		!PathPointCodec::Decode(pointBytes, sizeof(pointBytes), &decodedPoint) ||
		decodedPoint.fT != 1.0f || decodedPoint.ptPos.x != 2.0f ||
		decodedPoint.ptPos.y != 3.0f || decodedPoint.ptPos.z != 4.0f ||
		!std::signbit(decodedPoint.fAngle) ||
		PathPointCodec::Decode(pointBytes, 19, &decodedPoint) ||
		PathPointCodec::Encode(wirePoint, pointBytes, 19)) return 3;
	std::vector<CVec3> straight{CVec3(0, 0, 0), CVec3(2, 0, 0)};
	CObj<NAnimation::CPathInterpolator> path =
		new NAnimation::CPathInterpolator(straight);
	CVec3 before = path->GetPosition(-1);
	CVec3 middle = path->GetPosition(1);
	CVec3 after = path->GetPosition(3);
	if (!Near(path->GetDistance(), 2) || !Near(before.x, 0) ||
		!Near(middle.x, 1) || !Near(after.x, 2) ||
		!Near(middle.y, 0) || !Near(path->GetDirection(1), 0))
		return 1;

	std::vector<CVec3> bend{CVec3(0, 0, 0), CVec3(1, 0, 0), CVec3(1, 1, 0)};
	CObj<NAnimation::CPathInterpolator> curved =
		new NAnimation::CPathInterpolator(bend);
	CVec3 start = curved->GetPosition(0);
	CVec3 end = curved->GetPosition(curved->GetDistance());
	if (!Near(start.x, 0) || !Near(start.y, 0) ||
		!Near(end.x, 1) || !Near(end.y, 1) ||
		curved->GetDistance() <= 0 || !std::isfinite(curved->GetDistance()))
		return 2;
	std::printf("native animation path: straight=%0.6f bend=%0.6f points=%d\n",
		path->GetDistance(), curved->GetDistance(), curved->GetNumPoints());
	return 0;
}
