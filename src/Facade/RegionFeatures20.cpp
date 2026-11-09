#include "ImageResultInternal.h"
#include "RegionFeatures20.h"
#include <new>

namespace glim { namespace halcon {
namespace {
template<class Result>
ImageStatus Get(const Result* result, int index, RegionFeatures20* output, std::uint32_t bytes)
{
	if (!output) return ImageStatus::InvalidArgument;
	if (bytes != sizeof(*output)) return ImageStatus::VersionMismatch;
	*output = RegionFeatures20{}; output->structSize = sizeof(*output); output->apiVersion = 1;
	if (!result || index < 0 || static_cast<std::size_t>(index) >= result->m_features.size()) return ImageStatus::InvalidArgument;
	try
	{
		const auto& f = result->m_features[index];
		return feature::ComputeRegionFeatures20(result->m_runs[index], f.shape, f.moments,
			f.topology, f.x, f.y, f.width, f.height, *output);
	}
	catch (const std::bad_alloc&) { return ImageStatus::OutOfMemory; }
	catch (...) { return ImageStatus::ProcessingFailed; }
}
}
ImageStatus GLIM_HALCON_CALL GetBlobRegionFeatures20(const ImageResult* result, int index, RegionFeatures20* output, std::uint32_t bytes)
{
	return Get(result, index, output, bytes);
}
ImageStatus GLIM_HALCON_CALL GetDefectRegionFeatures20(const DefectResult* result, int index, RegionFeatures20* output, std::uint32_t bytes)
{
	return Get(result, index, output, bytes);
}
}}
