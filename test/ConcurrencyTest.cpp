#include "GlimImage.h"
#include <future>
#include <memory>
#include <vector>
using namespace glim::halcon;
bool Worker(const ImageResult* shared)
{
	try
	{
		for (int repeat = 0; repeat < 100; ++repeat)
		{
			DefectGroupingSettings settings{};
			if (GetDefaultDefectGroupingSettings(&settings, sizeof(settings)) != ImageStatus::Ok) return false;
			DefectResult* raw = nullptr;
			const auto status = GroupBlobDefects(shared, &settings, &raw);
			std::unique_ptr<DefectResult, decltype(&ReleaseDefectResult)> result(raw, &ReleaseDefectResult);
			if (status != ImageStatus::Ok) return false;
			DefectFeatures feature{}; int count = 0;
			if (GetDefectCount(raw, &count) != ImageStatus::Ok || count != 1 ||
				GetDefectFeatures(raw, 0, &feature, sizeof(feature)) != ImageStatus::Ok || feature.shape.area != 4 || feature.memberCount != 2) return false;
			unsigned char mask[64]{};
			if (CopyDefectMask(raw, 0, mask, sizeof(mask), 8) != ImageStatus::Ok || mask[9] != 255 || mask[25] != 0 || mask[41] != 255) return false;
		}
		return true;
	}
	catch (...) { return false; }
}
int main()
{
	try
	{
		unsigned char pixels[64]{}; pixels[9] = pixels[17] = pixels[41] = pixels[49] = 255;
		ImageView view{sizeof(ImageView), pixels, sizeof(pixels), 8, 8, 8, PixelFormat::Gray8};
		ImageSettings settings{}; GetDefaultImageSettings(&settings, sizeof(settings));
		settings.thresholdMode = ThresholdMode::NonZeroMask; settings.minArea = 1;
		ImageResult* raw = nullptr;
		if (AnalyzeImage(&view, &settings, &raw) != ImageStatus::Ok) return 1;
		std::unique_ptr<ImageResult, decltype(&ReleaseImageResult)> image(raw, &ReleaseImageResult);
		std::vector<std::future<bool>> tasks;
		for (int i = 0; i < 8; ++i) tasks.push_back(std::async(std::launch::async, Worker, raw));
		bool ok = true;
		for (auto& task : tasks) ok = task.get() && ok;
		return ok ? 0 : 2;
	}
	catch (...) { return 3; }
}
