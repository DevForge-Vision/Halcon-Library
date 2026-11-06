#include "Contour.h"
#include <vector>
int main()
{
	using namespace glim::halcon;
	using namespace glim::halcon::core;
	try
	{
		// A completed isolated component followed by a component stopped mid-trace.
		std::vector<Run> runs = {Run{0, 0, 0}, Run{3, 3, 5}, Run{4, 3, 5}, Run{5, 3, 5}};
		RegionData region;
		region.Assign(runs);
		ContourSet contours;
		if (!TraceOuterContours(region, contours) || contours.m_contours.size() != 2) return 1;
		if (TraceOuterContours(region, contours, 2) || !contours.IsEmpty()) return 2;
		if (!TraceOuterContours(region, contours) || contours.m_contours.size() != 2) return 3;
		return 0;
	}
	catch (...) { return 4; }
}
