#include "ImageResultInternal.h"
#include "ImageInternal.h"
#include "Contour.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <new>
#include <limits>

namespace glim { namespace halcon {
namespace {
ImageStatus ValidateSettings(const ImageSettings* settings)
{
	if (!settings) return ImageStatus::InvalidArgument;
	if (settings->structSize != sizeof(ImageSettings) || settings->apiVersion != 1) return ImageStatus::VersionMismatch;
	if ((settings->thresholdMode != ThresholdMode::Fixed && settings->thresholdMode != ThresholdMode::LocalMean &&
		settings->thresholdMode != ThresholdMode::NonZeroMask) ||
		(settings->foreground != Foreground::Bright && settings->foreground != Foreground::Dark) ||
		!std::isfinite(settings->threshold) || settings->threshold < 0 || settings->threshold > 255 ||
		settings->backgroundRadius < 1 || settings->backgroundRadius > 1024 || settings->minArea < 1 ||
		settings->maxArea < 0 || (settings->maxArea && settings->maxArea < settings->minArea) ||
		settings->maxBlobs < 1 || settings->maxBlobs > 100000 || settings->roiX < 0 || settings->roiY < 0 ||
		settings->roiWidth < 0 || settings->roiHeight < 0) return ImageStatus::InvalidArgument;
	return ImageStatus::Ok;
}
ImageStatus AnalyzeGray(const image::GrayImage& gray, ImageSettings settings, ImageResult** output)
{
	if (!settings.roiWidth && !settings.roiHeight)
	{
		if (settings.roiX || settings.roiY) return ImageStatus::InvalidArgument;
		settings.roiWidth = gray.width;
		settings.roiHeight = gray.height;
	}
	if (settings.roiWidth <= 0 || settings.roiHeight <= 0 || settings.roiX >= gray.width || settings.roiY >= gray.height ||
		settings.roiWidth > gray.width - settings.roiX || settings.roiHeight > gray.height - settings.roiY)
		return ImageStatus::InvalidArgument;
	std::vector<Run> runs;
	ImageStatus status = image::Segment(gray, settings, runs);
	if (status != ImageStatus::Ok) return status;
	std::unique_ptr<ImageResult> result(new ImageResult());
	result->m_width = gray.width;
	result->m_height = gray.height;
	if (!runs.empty())
	{
		std::vector<int> labels, first;
		std::vector<long long> areas;
		if (!core::LabelConnectedRuns(runs, labels, first, areas)) return ImageStatus::ProcessingFailed;
		std::vector<int> selected(areas.size(), -1);
		for (std::size_t i = 0; i < areas.size(); ++i)
		{
			if (areas[i] < settings.minArea || (settings.maxArea && areas[i] > settings.maxArea)) continue;
			if (result->m_runs.size() >= static_cast<std::size_t>(settings.maxBlobs)) return ImageStatus::ResourceLimit;
			selected[i] = static_cast<int>(result->m_runs.size());
			result->m_runs.emplace_back();
		}
		for (std::size_t i = 0; i < runs.size(); ++i)
		{
			const int target = selected[labels[i]];
			if (target >= 0) result->m_runs[target].push_back(runs[i]);
		}
		result->m_features.reserve(result->m_runs.size());
		for (std::size_t i = 0; i < result->m_runs.size(); ++i)
		{
			core::RegionData region;
			// Keep a separate input for the legacy mutable-reference Assign contract.
			std::vector<Run> regionRuns = result->m_runs[i];
			region.Assign(regionRuns);
			BlobFeatures features = {};
			features.structSize = sizeof(BlobFeatures);
			features.blobIndex = static_cast<int>(i);
			if (!image::ComputeChecked(region, features)) return ImageStatus::ProcessingFailed;
			int y1, y2, x1, x2;
			if (!region.GetBoundingBox(y1, y2, x1, x2)) return ImageStatus::ProcessingFailed;
			features.x = x1; features.y = y1;
			features.width = x2 - x1 + 1; features.height = y2 - y1 + 1;
			features.touchesImageBorder = x1 == 0 || y1 == 0 || x2 == gray.width - 1 || y2 == gray.height - 1;
			features.touchesRoiBorder = x1 == settings.roiX || y1 == settings.roiY ||
				x2 == settings.roiX + settings.roiWidth - 1 || y2 == settings.roiY + settings.roiHeight - 1;
			double sum = 0, squareSum = 0;
			for (const Run& run : result->m_runs[i])
				for (int x = run.colBegin; x <= run.colEnd; ++x)
				{
					const double value = gray.pixels[static_cast<std::size_t>(run.row) * gray.width + x];
					sum += value; squareSum += value * value;
				}
			features.meanGray = sum / features.shape.area;
			features.deviationGray = std::sqrt((std::max)(0.0, squareSum / features.shape.area - features.meanGray * features.meanGray));
			result->m_features.push_back(features);
		}
	}
	*output = result.release();
	return ImageStatus::Ok;
}
}

ImageStatus GLIM_HALCON_CALL GetDefaultImageSettings(ImageSettings* settings, std::uint32_t bytes)
{
	if (!settings) return ImageStatus::InvalidArgument;
	if (bytes != sizeof(ImageSettings)) return ImageStatus::VersionMismatch;
	ImageSettings value = {};
	value.structSize = sizeof(value); value.apiVersion = 1;
	value.thresholdMode = ThresholdMode::LocalMean;
	value.foreground = Foreground::Bright;
	value.threshold = 10; value.backgroundRadius = 9;
	value.minArea = 12; value.maxBlobs = 4096;
	*settings = value;
	return ImageStatus::Ok;
}

ImageStatus GLIM_HALCON_CALL AnalyzeImage(const ImageView* input, const ImageSettings* settings, ImageResult** output)
{
	if (!output) return ImageStatus::InvalidArgument;
	*output = nullptr;
	try
	{
		ImageStatus status = ValidateSettings(settings);
		if (status != ImageStatus::Ok) return status;
		if (!input) return ImageStatus::InvalidArgument;
		image::GrayImage gray;
		status = image::ConvertImage(*input, gray);
		return status == ImageStatus::Ok ? AnalyzeGray(gray, *settings, output) : status;
	}
	catch (const std::bad_alloc&) { return ImageStatus::OutOfMemory; }
	catch (...) { return ImageStatus::ProcessingFailed; }
}

ImageStatus GLIM_HALCON_CALL AnalyzeBmpFile(const char* path, const ImageSettings* settings, ImageResult** output)
{
	if (!output) return ImageStatus::InvalidArgument;
	*output = nullptr;
	try
	{
		ImageStatus status = ValidateSettings(settings);
		if (status != ImageStatus::Ok) return status;
		image::GrayImage gray;
		status = image::DecodeBmp(path, gray);
		return status == ImageStatus::Ok ? AnalyzeGray(gray, *settings, output) : status;
	}
	catch (const std::bad_alloc&) { return ImageStatus::OutOfMemory; }
	catch (...) { return ImageStatus::ProcessingFailed; }
}

ImageStatus GLIM_HALCON_CALL GetBlobCount(const ImageResult* result, int* count)
{
	if (!count) return ImageStatus::InvalidArgument;
	*count = 0;
	if (!result) return ImageStatus::InvalidArgument;
	*count = static_cast<int>(result->m_features.size());
	return ImageStatus::Ok;
}
ImageStatus GLIM_HALCON_CALL GetBlobFeatures(const ImageResult* result, int index, BlobFeatures* features, std::uint32_t bytes)
{
	if (!features) return ImageStatus::InvalidArgument;
	if (bytes != sizeof(BlobFeatures)) return ImageStatus::VersionMismatch;
	*features = BlobFeatures{};
	features->structSize = sizeof(BlobFeatures);
	if (!result || index < 0 || static_cast<std::size_t>(index) >= result->m_features.size()) return ImageStatus::InvalidArgument;
	*features = result->m_features[index];
	return ImageStatus::Ok;
}
ImageStatus GLIM_HALCON_CALL CopyBlobMask(const ImageResult* result, int index, unsigned char* data, std::uint64_t bytes, int stride)
{
	if (!result || !data || index < 0 || static_cast<std::size_t>(index) >= result->m_runs.size() || stride < result->m_width)
		return ImageStatus::InvalidArgument;
	const std::uint64_t required = static_cast<std::uint64_t>(result->m_height - 1) * stride + result->m_width;
	if (bytes < required || required > (std::numeric_limits<std::size_t>::max)()) return ImageStatus::InvalidArgument;
	for (int y = 0; y < result->m_height; ++y)
		std::memset(data + static_cast<std::size_t>(y) * stride, 0, result->m_width);
	for (const Run& run : result->m_runs[index])
		std::memset(data + static_cast<std::size_t>(run.row) * stride + run.colBegin, 255, run.colEnd - run.colBegin + 1);
	return ImageStatus::Ok;
}
void GLIM_HALCON_CALL ReleaseImageResult(ImageResult* result)
{
	try { delete result; }
	catch (...) { /* Destruction must not cross the DLL boundary. */ }
}
}}
