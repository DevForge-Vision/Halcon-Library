#include "ImageResultInternal.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <new>

namespace glim { namespace halcon {
namespace {
struct Bounds { double begin, end, minAcross, maxAcross; };
struct Edge { double gap, offset; int first, second; };
int Find(std::vector<int>& parents, int i)
{
	while (parents[i] != i) { parents[i] = parents[parents[i]]; i = parents[i]; }
	return i;
}
ImageStatus Validate(const DefectGroupingSettings* s)
{
	if (!s) return ImageStatus::InvalidArgument;
	if (s->structSize != sizeof(*s) || s->apiVersion != 1) return ImageStatus::VersionMismatch;
	if (!std::isfinite(s->axisAngleDegrees) || s->axisAngleDegrees < 0 || s->axisAngleDegrees >= 180 ||
		!std::isfinite(s->maxAlongGap) || s->maxAlongGap < 0 || s->maxAlongGap > image::MaxPixels ||
		!std::isfinite(s->maxAcrossOffset) || s->maxAcrossOffset < 0 || s->maxAcrossOffset > image::MaxPixels ||
		!std::isfinite(s->maxGroupLength) || s->maxGroupLength < 0 || s->maxGroupLength > image::MaxPixels ||
		s->maxInputBlobs < 1 || s->maxInputBlobs > 4096) return ImageStatus::InvalidArgument;
	return ImageStatus::Ok;
}
ImageStatus Build(const ImageResult& source, const DefectGroupingSettings& settings, DefectResult** output)
{
	const int count = static_cast<int>(source.m_features.size());
	if (count > settings.maxInputBlobs) return ImageStatus::ResourceLimit;
	const double radians = settings.axisAngleDegrees * 3.14159265358979323846 / 180;
	double dx = std::cos(radians), dy = std::sin(radians);
	if (settings.axisAngleDegrees == 0) { dx = 1; dy = 0; }
	if (settings.axisAngleDegrees == 90) { dx = 0; dy = 1; }
	std::vector<Bounds> bounds;
	std::vector<int> parents;
	for (int i = 0; i < count; ++i)
	{
		Bounds b = {(std::numeric_limits<double>::max)(), -(std::numeric_limits<double>::max)(), 0, 0};
		for (const Run& run : source.m_runs[i])
		{
			const double a = run.colBegin * dx + run.row * dy;
			const double z = run.colEnd * dx + run.row * dy;
			b.begin = (std::min)(b.begin, (std::min)(a, z)); b.end = (std::max)(b.end, (std::max)(a, z));
		}
		const BlobFeatures& f = source.m_features[i];
		b.minAcross = b.maxAcross = -f.shape.column * dy + f.shape.row * dx;
		bounds.push_back(b); parents.push_back(i);
	}
	std::vector<Edge> edges;
	for (int i = 0; i < count; ++i)
		for (int j = i + 1; j < count; ++j)
		{
			const Bounds& a = bounds[i]; const Bounds& b = bounds[j];
			const double gap = (std::max)(0.0, (std::max)(a.begin, b.begin) - (std::min)(a.end, b.end));
			const double offset = std::abs(a.minAcross - b.minAcross);
			if (gap <= settings.maxAlongGap && offset <= settings.maxAcrossOffset)
			{
				if (edges.size() >= 1000000) return ImageStatus::ResourceLimit;
				edges.push_back(Edge{gap, offset, i, j});
			}
		}
	std::sort(edges.begin(), edges.end(), [](const Edge& a, const Edge& b) {
		if (a.gap != b.gap) return a.gap < b.gap;
		if (a.offset != b.offset) return a.offset < b.offset;
		if (a.first != b.first) return a.first < b.first;
		return a.second < b.second;
	});
	for (const Edge& edge : edges)
	{
		int a = Find(parents, edge.first), b = Find(parents, edge.second);
		if (a == b) continue;
		if (a > b) std::swap(a, b);
		const Bounds merged = {(std::min)(bounds[a].begin, bounds[b].begin), (std::max)(bounds[a].end, bounds[b].end),
			(std::min)(bounds[a].minAcross, bounds[b].minAcross), (std::max)(bounds[a].maxAcross, bounds[b].maxAcross)};
		if (merged.maxAcross - merged.minAcross > settings.maxAcrossOffset ||
			(settings.maxGroupLength > 0 && merged.end - merged.begin > settings.maxGroupLength)) continue;
		parents[b] = a; bounds[a] = merged;
	}
	std::unique_ptr<DefectResult> result(new DefectResult());
	result->m_width = source.m_width; result->m_height = source.m_height;
	std::vector<int> groupOfRoot(count, -1);
	for (int i = 0; i < count; ++i)
	{
		const int root = Find(parents, i);
		if (groupOfRoot[root] < 0)
		{
			groupOfRoot[root] = static_cast<int>(result->m_members.size()); result->m_members.emplace_back();
		}
		result->m_members[groupOfRoot[root]].push_back(i);
	}
	for (const std::vector<int>& members : result->m_members)
	{
		std::vector<Run> runs;
		std::size_t runCount = 0;
		for (int member : members) runCount += source.m_runs[member].size();
		runs.reserve(runCount);
		for (int member : members) runs.insert(runs.end(), source.m_runs[member].begin(), source.m_runs[member].end());
		core::RegionData region; region.Assign(runs);
		BlobFeatures computed = {};
		if (!image::ComputeChecked(region, computed, static_cast<int>(members.size()))) return ImageStatus::ProcessingFailed;
		DefectFeatures f = {};
		f.structSize = sizeof(f); f.defectIndex = static_cast<int>(result->m_features.size()); f.memberCount = static_cast<int>(members.size());
		f.shape = computed.shape; f.moments = computed.moments; f.topology = computed.topology;
		int minY, maxY, minX, maxX;
		if (!region.GetBoundingBox(minY, maxY, minX, maxX)) return ImageStatus::ProcessingFailed;
		f.x = minX; f.y = minY; f.width = maxX - minX + 1; f.height = maxY - minY + 1;
		double total = 0, mean = 0, m2 = 0;
		// Combine population variance, including between-fragment variation.
		for (int member : members)
		{
			const BlobFeatures& blob = source.m_features[member];
			const double n = static_cast<double>(blob.shape.area), next = total + n, delta = blob.meanGray - mean;
			m2 += n * blob.deviationGray * blob.deviationGray + delta * delta * total * n / next;
			mean += delta * n / next; total = next;
			f.touchesImageBorder |= blob.touchesImageBorder; f.touchesRoiBorder |= blob.touchesRoiBorder;
		}
		f.meanGray = mean; f.deviationGray = std::sqrt((std::max)(0.0, m2 / total));
		result->m_runs.push_back(region.GetRuns()); result->m_features.push_back(f);
	}
	*output = result.release(); return ImageStatus::Ok;
}
}
ImageStatus GLIM_HALCON_CALL GetDefaultDefectGroupingSettings(DefectGroupingSettings* settings, std::uint32_t bytes)
{
	if (!settings) return ImageStatus::InvalidArgument;
	if (bytes != sizeof(*settings)) return ImageStatus::VersionMismatch;
	*settings = DefectGroupingSettings{};
	settings->structSize = sizeof(*settings); settings->apiVersion = 1;
	settings->axisAngleDegrees = 90; settings->maxAlongGap = 12;
	settings->maxAcrossOffset = 4; settings->maxInputBlobs = 4096;
	return ImageStatus::Ok;
}
ImageStatus GLIM_HALCON_CALL GroupBlobDefects(const ImageResult* source, const DefectGroupingSettings* settings, DefectResult** output)
{
	if (!output) return ImageStatus::InvalidArgument;
	*output = nullptr;
	try
	{
		const ImageStatus status = Validate(settings);
		if (status != ImageStatus::Ok) return status;
		if (!source) return ImageStatus::InvalidArgument;
		return Build(*source, *settings, output);
	}
	catch (const std::bad_alloc&) { return ImageStatus::OutOfMemory; }
	catch (...) { return ImageStatus::ProcessingFailed; }
}
ImageStatus GLIM_HALCON_CALL GetDefectCount(const DefectResult* result, int* count)
{
	if (!count) return ImageStatus::InvalidArgument;
	*count = 0;
	if (!result) return ImageStatus::InvalidArgument;
	*count = static_cast<int>(result->m_features.size()); return ImageStatus::Ok;
}
ImageStatus GLIM_HALCON_CALL GetDefectFeatures(const DefectResult* result, int index, DefectFeatures* features, std::uint32_t bytes)
{
	if (!features) return ImageStatus::InvalidArgument;
	if (bytes != sizeof(*features)) return ImageStatus::VersionMismatch;
	*features = DefectFeatures{}; features->structSize = sizeof(*features);
	if (!result || index < 0 || static_cast<std::size_t>(index) >= result->m_features.size()) return ImageStatus::InvalidArgument;
	*features = result->m_features[index]; return ImageStatus::Ok;
}
ImageStatus GLIM_HALCON_CALL GetDefectMemberBlobIndex(const DefectResult* result, int index, int memberIndex, int* blobIndex)
{
	if (!blobIndex) return ImageStatus::InvalidArgument;
	*blobIndex = -1;
	if (!result || index < 0 || static_cast<std::size_t>(index) >= result->m_members.size() ||
		memberIndex < 0 || static_cast<std::size_t>(memberIndex) >= result->m_members[index].size()) return ImageStatus::InvalidArgument;
	*blobIndex = result->m_members[index][memberIndex]; return ImageStatus::Ok;
}
ImageStatus GLIM_HALCON_CALL CopyDefectMask(const DefectResult* result, int index, unsigned char* data, std::uint64_t bytes, int stride)
{
	if (!result || !data || index < 0 || static_cast<std::size_t>(index) >= result->m_runs.size() || stride < result->m_width) return ImageStatus::InvalidArgument;
	const std::uint64_t required = static_cast<std::uint64_t>(result->m_height - 1) * stride + result->m_width;
	if (bytes < required || required > (std::numeric_limits<std::size_t>::max)()) return ImageStatus::InvalidArgument;
	for (int y = 0; y < result->m_height; ++y) std::memset(data + static_cast<std::size_t>(y) * stride, 0, result->m_width);
	for (const Run& run : result->m_runs[index]) std::memset(data + static_cast<std::size_t>(run.row) * stride + run.colBegin, 255, run.colEnd - run.colBegin + 1);
	return ImageStatus::Ok;
}
void GLIM_HALCON_CALL ReleaseDefectResult(DefectResult* result)
{
	try { delete result; }
	catch (...) { }
}
}}
