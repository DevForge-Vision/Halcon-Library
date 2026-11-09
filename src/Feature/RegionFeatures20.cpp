#include "RegionFeatures20.h"
#include "Contour.h"
#include <algorithm>
#include <cmath>
#include <tuple>

// Provisional alternatives: docs/02_DEFINITIONS.md RF20-D1/R1/C1.
#ifndef GLIM_RF20_EXTERNAL_CONTOUR
#define GLIM_RF20_EXTERNAL_CONTOUR 0
#endif
#ifndef GLIM_RF20_RECT_PERIMETER
#define GLIM_RF20_RECT_PERIMETER 0
#endif
#ifndef GLIM_RF20_RADIUS_CELL
#define GLIM_RF20_RADIUS_CELL 0
#endif

namespace glim { namespace halcon { namespace feature {
namespace {
// Lower envelope of parabolas, exact squared Euclidean distance in one dimension.
void DistanceTransform(const std::vector<double>& input, int count,
	std::vector<double>& output, std::vector<int>& sites, std::vector<double>& cuts)
{
	int k = 0;
	sites[0] = 0; cuts[0] = -1e30; cuts[1] = 1e30;
	for (int q = 1; q < count; ++q)
	{
		double split;
		for (;;)
		{
			const int v = sites[k];
			split = ((input[q] + double(q) * q) - (input[v] + double(v) * v)) / (2.0 * (q - v));
			if (split > cuts[k] || k == 0) break;
			--k;
		}
		++k; sites[k] = q; cuts[k] = split; cuts[k + 1] = 1e30;
	}
	k = 0;
	for (int q = 0; q < count; ++q)
	{
		while (cuts[k + 1] < q) ++k;
		const double delta = q - sites[k];
		output[q] = delta * delta + input[sites[k]];
	}
}

} // namespace
ImageStatus ComputeRegionFeatures20(const std::vector<Run>& runs, const RegionFeatures& shape,
	const RegionMoments& moments, const RegionTopology& topology, int originX, int originY,
	int boxWidth, int boxHeight, RegionFeatures20& result)
{
	const std::uint64_t width64 = static_cast<std::uint64_t>(boxWidth) + 2;
	const std::uint64_t height64 = static_cast<std::uint64_t>(boxHeight) + 2;
	if (width64 * height64 > 4194304) return ImageStatus::ResourceLimit;
	const int width = static_cast<int>(width64), height = static_cast<int>(height64);
	const std::size_t size = static_cast<std::size_t>(width) * height;
	std::vector<unsigned char> mask(size, 0);
	for (const Run& run : runs)
		for (int x = run.colBegin; x <= run.colEnd; ++x)
			mask[static_cast<std::size_t>(run.row - originY + 1) * width + x - originX + 1] = 1;

	RegionFeatures20 value = {};
	value.structSize = sizeof(value); value.apiVersion = 1;
	value.provisionalMask = (1u << 12) | (1u << 13) | (1u << 17) | (1u << 18) | (1u << 19);
	value.policyFlags = (GLIM_RF20_EXTERNAL_CONTOUR ? 1u : 0u) |
		(GLIM_RF20_RECT_PERIMETER ? 2u : 0u) | (GLIM_RF20_RADIUS_CELL ? 4u : 0u);
	double* v = value.values;
	v[0] = moments.anisometry; v[1] = double(shape.area); v[2] = double(topology.areaHoles);
	v[3] = moments.bulkiness; v[4] = shape.circularity; v[5] = shape.column;
	v[6] = originX; v[7] = double(originX) + boxWidth - 1; v[8] = shape.compactness;
	v[9] = double(topology.numConnected); v[10] = shape.contLength; v[11] = shape.convexity;
	v[14] = double(topology.eulerNumber); v[15] = boxHeight; v[16] = double(topology.numHoles);


#if GLIM_RF20_EXTERNAL_CONTOUR
	std::vector<unsigned char> external;
	external.assign(size, 0);
	std::vector<Run> copied = runs;
	core::RegionData region; region.Assign(copied);
	core::ContourSet contours;
	if (!core::TraceOuterContours(region, contours)) return ImageStatus::ProcessingFailed;
	for (const auto& contour : contours.m_contours)
		for (const auto& p : contour)
			external[static_cast<std::size_t>(p.row - originY + 1) * width + p.col - originX + 1] = 1;
#endif
	double mean = 0, m2 = 0, count = 0;
	std::vector<int> heights(width, 0), stack;
	stack.reserve(width);
	long long bestScore = -1;
	std::tuple<int, int, int, int> bestRect;
	for (int y = 1; y < height - 1; ++y)
	{
		for (int x = 1; x < width - 1; ++x)
		{
			const std::size_t at = static_cast<std::size_t>(y) * width + x;
			heights[x] = mask[at] ? heights[x] + 1 : 0;
			if (!mask[at]) continue;
			bool boundary = false;
#if GLIM_RF20_EXTERNAL_CONTOUR
			boundary = external[at] != 0;
#else
			for (int dy = -1; dy <= 1; ++dy)
				for (int dx = -1; dx <= 1; ++dx)
					if (!mask[static_cast<std::size_t>(y + dy) * width + x + dx]) boundary = true;
#endif
			if (boundary)
			{
				const double dy = y + originY - 1 - shape.row, dx = x + originX - 1 - shape.column;
				const double distance = std::sqrt(dx * dx + dy * dy), delta = distance - mean;
				++count; mean += delta / count; m2 += delta * (distance - mean);
			}
		}
		stack.clear(); stack.push_back(0);
		for (int x = 1; x < width; ++x)
		{
			while (stack.size() > 1 && heights[stack.back()] > heights[x])
			{
				const int h = heights[stack.back()]; stack.pop_back();
				const int left = stack.back() + 1, w = x - left;
				const long long score = GLIM_RF20_RECT_PERIMETER ? static_cast<long long>(h) + w : static_cast<long long>(h) * w;
				const auto rect = std::make_tuple(y - h + 1, left, y, x - 1);
				if (score > bestScore || (score == bestScore && rect < bestRect))
				{
					bestScore = score; bestRect = rect; v[17] = h; v[19] = w;
				}
			}
			stack.push_back(x);
		}
	}
	if (count == 0) return ImageStatus::ProcessingFailed;
	v[12] = std::sqrt((std::max)(0.0, m2 / count)); v[13] = mean;
	const int length = (std::max)(width, height);
	std::vector<double> distances(size), input(length), output(length), cuts(length + 1);
	std::vector<int> sites(length);
	// Finite infinity exceeds every possible squared distance under the size limit.
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x) input[x] = mask[static_cast<std::size_t>(y) * width + x] ? 1e16 : 0;
		DistanceTransform(input, width, output, sites, cuts);
		for (int x = 0; x < width; ++x) distances[static_cast<std::size_t>(y) * width + x] = output[x];
	}
	double maxSquared = 0;
	for (int x = 1; x < width - 1; ++x)
	{
		for (int y = 0; y < height; ++y) input[y] = distances[static_cast<std::size_t>(y) * width + x];
		DistanceTransform(input, height, output, sites, cuts);
		for (int y = 1; y < height - 1; ++y)
			if (mask[static_cast<std::size_t>(y) * width + x]) maxSquared = (std::max)(maxSquared, output[y]);
	}
	const double distance = std::sqrt(maxSquared);
	v[18] = GLIM_RF20_RADIUS_CELL ? (std::max)(0.0, distance - 0.5) : (std::max)(0.0, std::ceil(distance) - 1);
	for (double entry : value.values) if (!std::isfinite(entry)) return ImageStatus::ProcessingFailed;
	result = value;
	return ImageStatus::Ok;
}

}}}
