#pragma once
#include "GlimImage.h"
#include <memory>
#include <string>
#include <vector>
namespace demo {
using namespace glim::halcon;
struct Result
{
	ImageStatus status = ImageStatus::ProcessingFailed;
	std::unique_ptr<ImageResult, decltype(&ReleaseImageResult)> blobs{nullptr, &ReleaseImageResult};
	std::unique_ptr<DefectResult, decltype(&ReleaseDefectResult)> defects{nullptr, &ReleaseDefectResult};
	std::vector<BlobFeatures> blobRows;
	std::vector<DefectFeatures> defectRows;
	std::vector<RegionFeatures20> blobFeatures20, defectFeatures20;
	std::vector<ImageStatus> blobFeatures20Status, defectFeatures20Status;
};
std::shared_ptr<Result> Analyze(std::vector<unsigned char> pixels, int width, int height, ImageSettings image, DefectGroupingSettings grouping);
}

