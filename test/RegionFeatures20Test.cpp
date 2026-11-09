#include "GlimImage.h"
#include <cmath>
#include <cstdio>
#include <cstddef>
#include <memory>
#include <vector>
using namespace glim::halcon;
static_assert(sizeof(RegionFeatures20) == 176, "RF20 ABI size");
static_assert(offsetof(RegionFeatures20, values) == 16, "RF20 ABI offset");
namespace {
int failures = 0;
void Check(bool ok, const char* name) { if (!ok) { ++failures; std::printf("FAIL: %s\n", name); } }
void Near(double a, double b, const char* name) { Check(std::abs(a - b) < 1e-9, name); }
using ImagePtr = std::unique_ptr<ImageResult, decltype(&ReleaseImageResult)>;
ImagePtr Analyze(std::vector<unsigned char>& pixels, int width, int height)
{
	ImageSettings settings = {};
	GetDefaultImageSettings(&settings, sizeof(settings));
	settings.thresholdMode = ThresholdMode::NonZeroMask; settings.minArea = 1;
	// Nonzero ROI origin verifies that all coordinates remain image-global.
	settings.roiX = 1; settings.roiY = 1; settings.roiWidth = width - 1; settings.roiHeight = height - 1;
	ImageView view = {sizeof(ImageView), pixels.data(), pixels.size(), width, height, width, PixelFormat::Gray8};
	ImageResult* raw = nullptr;
	Check(AnalyzeImage(&view, &settings, &raw) == ImageStatus::Ok, "analyze fixture");
	return ImagePtr(raw, &ReleaseImageResult);
}
void Rectangle(std::vector<unsigned char>& pixels, int y1, int y2)
{
	for (int y = y1; y <= y2; ++y) for (int x = 3; x <= 5; ++x) pixels[y * 12 + x] = 255;
}
}
int main()
{
	std::vector<unsigned char> pixels(12 * 12, 0);
	pixels[2 * 12 + 3] = 255;
	auto single = Analyze(pixels, 12, 12);
	RegionFeatures20 f = {};
	Check(GetBlobRegionFeatures20(single.get(), 0, &f, sizeof(f)) == ImageStatus::Ok, "single query");
	Check(f.structSize == 176 && f.apiVersion == 1, "metadata");
	Check(f.provisionalMask == ((1u << 12) | (1u << 13) | (1u << 17) | (1u << 18) | (1u << 19)), "provisional bits");
	Near(f.values[1], 1, "single area"); Near(f.values[5], 3, "global centroid");
	Near(f.values[6], 3, "global min column"); Near(f.values[7], 3, "global max column");
	Near(f.values[12], 0, "single sigma"); Near(f.values[13], 0, "single mean");
	Near(f.values[17], 1, "single inner height"); Near(f.values[19], 1, "single inner width");
	Near(f.values[18], (f.policyFlags & 4) ? 0.5 : 0, "single radius policy");
	Rectangle(pixels, 2, 4);
	auto square = Analyze(pixels, 12, 12);
	Check(GetBlobRegionFeatures20(square.get(), 0, &f, sizeof(f)) == ImageStatus::Ok, "square query");
	Near(f.values[1], 9, "square area"); Near(f.values[5], 4, "square centroid");
	Near(f.values[12], (std::sqrt(2.0) - 1) / 2, "square sigma");
	Near(f.values[13], (std::sqrt(2.0) + 1) / 2, "square mean");
	Near(f.values[17], 3, "square inner height"); Near(f.values[19], 3, "square inner width");
	Near(f.values[18], (f.policyFlags & 4) ? 1.5 : 1, "square radius");
	BlobFeatures base = {};
	GetBlobFeatures(square.get(), 0, &base, sizeof(base));
	const double expected[] = {base.moments.anisometry, double(base.shape.area), double(base.topology.areaHoles),
		base.moments.bulkiness, base.shape.circularity, base.shape.column, double(base.x), double(base.x + base.width - 1),
		base.shape.compactness, double(base.topology.numConnected), base.shape.contLength, base.shape.convexity};
	for (int i = 0; i < 12; ++i) Near(f.values[i], expected[i], "existing field mapping");
	Near(f.values[14], base.topology.eulerNumber, "euler mapping");
	Near(f.values[15], base.height, "height mapping"); Near(f.values[16], base.topology.numHoles, "hole mapping");
	Check(GetBlobRegionFeatures20(square.get(), 1, &f, sizeof(f)) == ImageStatus::InvalidArgument, "invalid index");
	Near(f.values[1], 0, "failure clears output");
	Check(GetBlobRegionFeatures20(nullptr, 0, &f, sizeof(f)) == ImageStatus::InvalidArgument, "null handle");
	Check(GetBlobRegionFeatures20(square.get(), 0, nullptr, sizeof(f)) == ImageStatus::InvalidArgument, "null output");
	f.values[1] = 77;
	Check(GetBlobRegionFeatures20(square.get(), 0, &f, sizeof(f) - 1) == ImageStatus::VersionMismatch, "wrong bytes");
	Near(f.values[1], 77, "size mismatch does not write");
	Rectangle(pixels, 7, 9);
	auto fragments = Analyze(pixels, 12, 12);
	DefectGroupingSettings settings = {}; GetDefaultDefectGroupingSettings(&settings, sizeof(settings));
	DefectResult* raw = nullptr;
	Check(GroupBlobDefects(fragments.get(), &settings, &raw) == ImageStatus::Ok, "group fragments");
	std::unique_ptr<DefectResult, decltype(&ReleaseDefectResult)> grouped(raw, &ReleaseDefectResult);
	fragments.reset();
	Check(GetDefectRegionFeatures20(grouped.get(), 0, &f, sizeof(f)) == ImageStatus::Ok, "independent group query");
	Near(f.values[1], 18, "group original area"); Near(f.values[9], 2, "group components");
	Near(f.values[14], 2, "group euler"); Near(f.values[15], 8, "group bbox height");
	Near(f.values[17], 3, "gap not filled"); Near(f.values[19], 3, "group inner width");
	pixels.assign(144, 0); Rectangle(pixels, 2, 4); pixels[3 * 12 + 4] = 0;
	auto ring = Analyze(pixels, 12, 12);
	Check(GetBlobRegionFeatures20(ring.get(), 0, &f, sizeof(f)) == ImageStatus::Ok, "ring query");
	Near(f.values[1], 8, "ring area"); Near(f.values[2], 1, "ring hole area");
	Near(f.values[16], 1, "ring holes"); Near(f.values[14], 0, "ring euler");
	Near(f.values[17], 1, "rectangle tie height"); Near(f.values[19], 3, "rectangle tie width");
	Check(GetDefectRegionFeatures20(grouped.get(), -1, &f, sizeof(f)) == ImageStatus::InvalidArgument, "group invalid index");
	Check(GetDefectRegionFeatures20(nullptr, 0, &f, sizeof(f)) == ImageStatus::InvalidArgument, "group null handle");
	// Two distant pixels create a large group bounding box without a large region area.
	pixels.assign(2051 * 2051, 0);
	pixels[2051 + 1] = 255; pixels[2049 * 2051 + 2049] = 255;
	auto sparse = Analyze(pixels, 2051, 2051);
	settings.maxAlongGap = 3000; settings.maxAcrossOffset = 3000;
	raw = nullptr;
	Check(GroupBlobDefects(sparse.get(), &settings, &raw) == ImageStatus::Ok, "sparse grouping");
	grouped.reset(raw);
	Check(GetDefectRegionFeatures20(grouped.get(), 0, &f, sizeof(f)) == ImageStatus::ResourceLimit, "padded bbox limit");
	Near(f.values[1], 0, "limit returns no partial values");
	return failures ? 1 : 0;
}
