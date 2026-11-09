#pragma once
#include "GlimImage.h"
#include "Region.h"
namespace glim { namespace halcon { namespace feature {
ImageStatus ComputeRegionFeatures20(const std::vector<Run>& runs, const RegionFeatures& shape,
	const RegionMoments& moments, const RegionTopology& topology, int originX, int originY,
	int boxWidth, int boxHeight, RegionFeatures20& result);
}}}
