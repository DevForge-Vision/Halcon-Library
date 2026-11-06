#include "GlimImage.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <memory>
#include <vector>
using namespace glim::halcon;
namespace {
int failures = 0;
void Check(bool condition, const char* label)
{
	if (!condition) { ++failures; std::printf("FAIL: %s\n", label); }
}
using ImagePtr = std::unique_ptr<ImageResult, decltype(&ReleaseImageResult)>;
using DefectPtr = std::unique_ptr<DefectResult, decltype(&ReleaseDefectResult)>;
ImagePtr Analyze(std::vector<unsigned char>& pixels)
{
	ImageSettings settings = {}; GetDefaultImageSettings(&settings, sizeof(settings));
	settings.thresholdMode = ThresholdMode::NonZeroMask; settings.minArea = 1;
	ImageView view = {}; view.structSize = sizeof(view); view.width = 12; view.height = 12;
	view.strideBytes = 12; view.format = PixelFormat::Gray8; view.data = pixels.data(); view.dataBytes = pixels.size();
	ImageResult* raw = nullptr;
	Check(AnalyzeImage(&view, &settings, &raw) == ImageStatus::Ok, "analysis");
	return ImagePtr(raw, &ReleaseImageResult);
}
DefectPtr Group(const ImageResult* image, const DefectGroupingSettings& settings, int expected)
{
	DefectResult* raw = nullptr;
	Check(GroupBlobDefects(image, &settings, &raw) == ImageStatus::Ok, "grouping");
	DefectPtr result(raw, &ReleaseDefectResult);
	int count = -1;
	Check(GetDefectCount(result.get(), &count) == ImageStatus::Ok && count == expected, "defect count");
	return result;
}
}
int main()
{
	DefectGroupingSettings settings = {};
	Check(GetDefaultDefectGroupingSettings(&settings, sizeof(settings)) == ImageStatus::Ok, "defaults");
	settings.maxAlongGap = 3; settings.maxAcrossOffset = 0;
	std::vector<unsigned char> pixels(144, 0);
	pixels[1 * 12 + 2] = pixels[2 * 12 + 2] = 100;
	pixels[5 * 12 + 2] = pixels[6 * 12 + 2] = 200;
	pixels[1 * 12 + 8] = pixels[2 * 12 + 8] = 150;
	ImagePtr image = Analyze(pixels);
	DefectPtr result = Group(image.get(), settings, 2);
	DefectFeatures feature = {};
	Check(GetDefectFeatures(result.get(), 0, &feature, sizeof(feature)) == ImageStatus::Ok, "features");
	Check(feature.memberCount == 2 && feature.shape.area == 4 && feature.topology.numConnected == 2, "union without filling gap");
	Check(feature.shape.row == 3.5 && feature.shape.column == 2 && feature.height == 6, "global union geometry");
	Check(feature.meanGray == 150 && std::abs(feature.deviationGray - 50) < 1e-9, "between-fragment variance");
	// Compare union moments against the existing region API, not a sum of moments.
	int rows[] = {1,2,5,6}, columns[] = {2,2,2,2};
	Region region = Region::FromRuns(rows, columns, columns, 4);
	RegionMoments moments = {}; ComputeMoments(region, moments);
	Check(std::abs(feature.moments.m20 - moments.m20) < 1e-9 && feature.moments.ra == moments.ra, "union moments");
	int member = -1;
	Check(GetDefectMemberBlobIndex(result.get(), 0, 1, &member) == ImageStatus::Ok && member == 2, "original blob membership");
	Check(GetDefectMemberBlobIndex(result.get(), 0, 2, &member) == ImageStatus::InvalidArgument && member == -1, "member range");
	settings.maxAlongGap = 2.99; Group(image.get(), settings, 3);
	settings.maxAlongGap = 3; settings.maxGroupLength = 4; Group(image.get(), settings, 3);
	settings.maxGroupLength = 0;
	image.reset(); // Defect result owns its own data.
	std::vector<unsigned char> mask(144, 0);
	Check(CopyDefectMask(result.get(), 0, mask.data(), mask.size(), 12) == ImageStatus::Ok, "result survives source release");
	Check(mask[14] == 255 && mask[26] == 255 && mask[38] == 0 && mask[62] == 255 && mask[20] == 0, "exact union mask");
	Check(CopyDefectMask(result.get(), 0, mask.data(), 143, 12) == ImageStatus::InvalidArgument, "short mask buffer");
	std::fill(pixels.begin(), pixels.end(), static_cast<unsigned char>(0));
	pixels[1 * 12 + 1] = pixels[4 * 12 + 2] = pixels[7 * 12 + 3] = 100;
	image = Analyze(pixels); settings.maxAcrossOffset = 1;
	Group(image.get(), settings, 2); // A-B-C lateral drift must not merge all three.
	settings.maxAcrossOffset = 2; Group(image.get(), settings, 1);
	settings.axisAngleDegrees = 0; settings.maxAcrossOffset = 0; Group(image.get(), settings, 3);
	settings.axisAngleDegrees = 90; settings.maxAcrossOffset = 2;
	settings.maxInputBlobs = 2;
	DefectResult* bad = nullptr;
	Check(GroupBlobDefects(image.get(), &settings, &bad) == ImageStatus::ResourceLimit && !bad, "input limit");
	settings.maxInputBlobs = 4096; settings.axisAngleDegrees = (std::numeric_limits<double>::quiet_NaN)();
	Check(GroupBlobDefects(image.get(), &settings, &bad) == ImageStatus::InvalidArgument && !bad, "NaN rejection");
	settings.axisAngleDegrees = 90; settings.apiVersion = 99;
	Check(GroupBlobDefects(image.get(), &settings, &bad) == ImageStatus::VersionMismatch && !bad, "version rejection");
	settings.apiVersion = 1;
	std::fill(pixels.begin(), pixels.end(), static_cast<unsigned char>(0)); image = Analyze(pixels); Group(image.get(), settings, 0);
	ReleaseDefectResult(nullptr);
	std::printf("Defect grouping failures: %d\n", failures);
	return failures ? 1 : 0;
}
