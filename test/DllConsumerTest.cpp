#include "GlimImage.h"
#include <type_traits>
using namespace glim::halcon;
using AnalyzeFunction = ImageStatus (GLIM_HALCON_CALL*)(const ImageView*, const ImageSettings*, ImageResult**);
static_assert(std::is_same<decltype(&AnalyzeImage), AnalyzeFunction>::value, "Explicit calling convention");
int __cdecl main()
{
	ImageSettings settings{};
	if (GetDefaultImageSettings(&settings, sizeof(settings)) != ImageStatus::Ok) return 1;
	settings.thresholdMode = ThresholdMode::NonZeroMask; settings.minArea = 1;
	const unsigned char pixels[] = {0, 255, 0, 0, 255, 0, 0, 0, 0};
	ImageView image{sizeof(ImageView), pixels, sizeof(pixels), 3, 3, 3, PixelFormat::Gray8};
	ImageResult* result = nullptr;
	if (AnalyzeImage(&image, &settings, &result) != ImageStatus::Ok) return 2;
	int count = 0; BlobFeatures features{};
	bool ok = GetBlobCount(result, &count) == ImageStatus::Ok && count == 1 &&
		GetBlobFeatures(result, 0, &features, sizeof(features)) == ImageStatus::Ok && features.shape.area == 2;
	DefectGroupingSettings grouping{}; DefectResult* defects = nullptr;
	ok = ok && GetDefaultDefectGroupingSettings(&grouping, sizeof(grouping)) == ImageStatus::Ok &&
		GroupBlobDefects(result, &grouping, &defects) == ImageStatus::Ok;
	ReleaseImageResult(result);
	DefectFeatures defect{};
	ok = ok && GetDefectFeatures(defects, 0, &defect, sizeof(defect)) == ImageStatus::Ok && defect.memberCount == 1 && defect.shape.area == 2;
	ReleaseDefectResult(defects);
	return ok ? 0 : 3;
}
