#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#include "../Misc/Geom.h"
#endif
#include "../Main/PolyUtils.h"

#include <cmath>
#include <cstdio>

static double Area( const TPolygonsList &polygons )
{
	double total = 0;
	for ( const auto &polygon : polygons )
	{
		double area = 0;
		for ( std::size_t i = 0; i < polygon.size(); ++i )
		{
			const CVec2 &a = polygon[i];
			const CVec2 &b = polygon[(i + 1) % polygon.size()];
			area += double(a.x) * b.y - double(a.y) * b.x;
		}
		total += std::fabs(area) * 0.5;
	}
	return total;
}

int main()
{
	const std::vector<CVec2> first = {
		CVec2(0, 0), CVec2(4, 0), CVec2(4, 4), CVec2(0, 4) };
	const std::vector<CVec2> second = {
		CVec2(2, 2), CVec2(6, 2), CVec2(6, 6), CVec2(2, 6) };
	if ( IsPolygonInverse(first) == IsPolygonInverse(
		std::vector<CVec2>(first.rbegin(), first.rend())) )
		return 1;
	TPolygonsList source, clipping, intersection, remainder;
	source.push_back(first);
	clipping.push_back(second);
	ClipPolygon(source, clipping, &intersection, &remainder);
	const double overlap = Area(intersection);
	const double outside = Area(remainder);
	std::printf("intersection=%zu remainder=%zu areas=%.6f,%.6f\n",
		intersection.size(), remainder.size(), overlap, outside);
	return std::fabs(overlap - 4.0) < 0.0001 &&
		std::fabs(outside - 12.0) < 0.0001 ? 0 : 2;
}
