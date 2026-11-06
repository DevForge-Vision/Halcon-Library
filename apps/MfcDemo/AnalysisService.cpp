#include "AnalysisService.h"
namespace demo {
std::shared_ptr<Result> Analyze(std::vector<unsigned char> pixels, int width, int height, ImageSettings image, DefectGroupingSettings grouping)
{
	auto result = std::make_shared<Result>();
	try
	{
		ImageResult* raw = nullptr;
		ImageView view{sizeof(ImageView), pixels.data(), pixels.size(), width, height, width * 3, PixelFormat::Bgr24};
		result->status = AnalyzeImage(&view, &image, &raw);
		result->blobs.reset(raw);
		if (result->status != ImageStatus::Ok) return result;
		DefectResult* grouped = nullptr;
		result->status = GroupBlobDefects(raw, &grouping, &grouped);
		result->defects.reset(grouped);
		if (result->status != ImageStatus::Ok) return result;
		int count = 0;
		result->status = GetBlobCount(raw, &count);
		if (result->status != ImageStatus::Ok) return result;
		for (int i = 0; i < count; ++i)
		{
			BlobFeatures f = {};
			result->status = GetBlobFeatures(raw, i, &f, sizeof(f));
			if (result->status != ImageStatus::Ok) return result;
			result->blobRows.push_back(f);
		}
		result->status = GetDefectCount(grouped, &count);
		if (result->status != ImageStatus::Ok) return result;
		for (int i = 0; i < count; ++i)
		{
			DefectFeatures f = {};
			result->status = GetDefectFeatures(grouped, i, &f, sizeof(f));
			if (result->status != ImageStatus::Ok) return result;
			result->defectRows.push_back(f);
		}
	}
	catch (const std::bad_alloc&) { result->status = ImageStatus::OutOfMemory; }
	catch (...) { result->status = ImageStatus::ProcessingFailed; }
	return result;
}
}

