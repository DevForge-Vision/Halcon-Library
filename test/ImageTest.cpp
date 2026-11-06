#include "GlimImage.h"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <memory>
#include <vector>

using namespace glim::halcon;
namespace {
int failures = 0;
void Check(bool value, const char* message)
{
	if (!value) { ++failures; std::printf("FAIL: %s\n", message); }
}
using ResultPtr = std::unique_ptr<ImageResult, decltype(&ReleaseImageResult)>;
ImageView View(std::vector<unsigned char>& bytes, int width, int height, int stride)
{
	ImageView view = {};
	view.structSize = sizeof(view); view.data = bytes.data(); view.dataBytes = bytes.size();
	view.width = width; view.height = height; view.strideBytes = stride; view.format = PixelFormat::Gray8;
	return view;
}
void Put32(std::vector<unsigned char>& bytes, std::size_t offset, unsigned int value)
{
	for (int i = 0; i < 4; ++i) bytes[offset + i] = static_cast<unsigned char>((value >> (8 * i)) & 255);
}
void Write(const char* path, const std::vector<unsigned char>& bytes)
{
	std::ofstream file(path, std::ios::binary);
	file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
	Check(static_cast<bool>(file), "fixture write");
}
void CheckBmp(bool topDown, bool palette)
{
	// 3x2 with one bright top-left pixel; row padding and row reversal are observable.
	const unsigned int offset = palette ? 62 : 54;
	const unsigned int stride = palette ? 4 : 12;
	std::vector<unsigned char> bytes(offset + stride * 2, 0);
	bytes[0] = 'B'; bytes[1] = 'M'; Put32(bytes, 2, static_cast<unsigned int>(bytes.size()));
	Put32(bytes, 10, offset); Put32(bytes, 14, 40); Put32(bytes, 18, 3);
	Put32(bytes, 22, topDown ? 0xfffffffeu : 2);
	bytes[26] = 1; bytes[28] = palette ? 8 : 24;
	if (palette)
	{
		Put32(bytes, 46, 2);
		// Palette index zero is white: decoder must not treat indices as intensity.
		bytes[54] = bytes[55] = bytes[56] = 255;
		for (unsigned int y = 0; y < 2; ++y)
			for (unsigned int x = 0; x < 3; ++x) bytes[offset + y * stride + x] = 1;
		bytes[offset + (topDown ? 0 : stride)] = 0;
	}
	else
	{
		const unsigned int pixel = offset + (topDown ? 0 : stride);
		bytes[pixel] = bytes[pixel + 1] = bytes[pixel + 2] = 255;
	}
	const char* path = "glim_image_test_fixture.bmp";
	Write(path, bytes);
	ImageSettings settings = {};
	GetDefaultImageSettings(&settings, sizeof(settings));
	settings.thresholdMode = ThresholdMode::Fixed; settings.threshold = 128; settings.minArea = 1;
	ImageResult* raw = nullptr;
	Check(AnalyzeBmpFile(path, &settings, &raw) == ImageStatus::Ok, "BMP decode");
	ResultPtr result(raw, &ReleaseImageResult);
	int count = -1; GetBlobCount(result.get(), &count);
	Check(count == 1, "BMP blob count");
	BlobFeatures feature = {};
	Check(GetBlobFeatures(result.get(), 0, &feature, sizeof(feature)) == ImageStatus::Ok, "BMP result");
	Check(feature.shape.area == 1 && feature.x == 0 && feature.y == 0 && feature.meanGray == 255, "BMP orientation/palette/padding");
	bytes.resize(bytes.size() - 1); Write(path, bytes);
	raw = nullptr;
	Check(AnalyzeBmpFile(path, &settings, &raw) == ImageStatus::UnsupportedFormat && !raw, "truncated BMP rejected");
	std::remove(path);
}
}

int main()
{
	ImageSettings settings = {};
	Check(GetDefaultImageSettings(&settings, sizeof(settings) - 1) == ImageStatus::VersionMismatch, "settings ABI size");
	Check(GetDefaultImageSettings(&settings, sizeof(settings)) == ImageStatus::Ok, "defaults");
	settings.thresholdMode = ThresholdMode::Fixed; settings.threshold = 100; settings.minArea = 1;
	// Two separate blobs, padded rows; no maximum-blob shortcut is permitted.
	std::vector<unsigned char> pixels(8 * 5, 0);
	pixels[1 * 8 + 1] = 120; pixels[1 * 8 + 2] = 140;
	pixels[3 * 8 + 5] = 200;
	ImageView view = View(pixels, 6, 5, 8);
	ImageResult* raw = nullptr;
	Check(AnalyzeImage(&view, &settings, &raw) == ImageStatus::Ok, "analyze two blobs");
	ResultPtr result(raw, &ReleaseImageResult);
	int count = -1;
	Check(GetBlobCount(result.get(), &count) == ImageStatus::Ok && count == 2, "all blobs returned");
	BlobFeatures blob = {};
	Check(GetBlobFeatures(result.get(), 0, &blob, sizeof(blob)) == ImageStatus::Ok, "first blob");
	Check(blob.shape.area == 2 && blob.shape.row == 1 && blob.shape.column == 1.5, "area and center");
	Check(blob.meanGray == 130 && std::abs(blob.deviationGray - 10) < 1e-12, "original gray statistics");
	std::vector<unsigned char> mask(40, 77);
	Check(CopyBlobMask(result.get(), 0, mask.data(), mask.size(), 8) == ImageStatus::Ok, "copy mask");
	Check(mask[9] == 255 && mask[10] == 255 && mask[29] == 0 && mask[6] == 77, "mask membership/padding");
	Check(CopyBlobMask(result.get(), 0, mask.data(), 37, 8) == ImageStatus::InvalidArgument, "short output buffer");
	Check(GetBlobFeatures(result.get(), 2, &blob, sizeof(blob)) == ImageStatus::InvalidArgument, "invalid result index");
	settings.maxBlobs = 1; raw = nullptr;
	Check(AnalyzeImage(&view, &settings, &raw) == ImageStatus::ResourceLimit && !raw, "no silent truncation");
	settings.maxBlobs = 10; settings.roiX = 1; settings.roiY = 1; settings.roiWidth = 2; settings.roiHeight = 1;
	Check(AnalyzeImage(&view, &settings, &raw) == ImageStatus::Ok, "ROI");
	ResultPtr roi(raw, &ReleaseImageResult);
	GetBlobFeatures(roi.get(), 0, &blob, sizeof(blob));
	Check(blob.shape.column == 1.5 && blob.touchesRoiBorder && !blob.touchesImageBorder, "global coordinates and ROI boundary");
	settings.roiWidth = 100;
	Check(AnalyzeImage(&view, &settings, &raw) == ImageStatus::InvalidArgument && !raw, "invalid ROI");
	GetDefaultImageSettings(&settings, sizeof(settings));
	settings.minArea = 1;
	std::vector<unsigned char> uniform(25, 50);
	ImageView flat = View(uniform, 5, 5, 5);
	Check(AnalyzeImage(&flat, &settings, &raw) == ImageStatus::Ok, "uniform local mean");
	ResultPtr empty(raw, &ReleaseImageResult);
	GetBlobCount(empty.get(), &count); Check(count == 0, "valid empty result");
	uniform[12] = 0; settings.foreground = Foreground::Dark;
	Check(AnalyzeImage(&flat, &settings, &raw) == ImageStatus::Ok, "dark local contrast");
	ResultPtr dark(raw, &ReleaseImageResult);
	GetBlobCount(dark.get(), &count); Check(count == 1, "dark blob count");
	flat.dataBytes = 24;
	Check(AnalyzeImage(&flat, &settings, &raw) == ImageStatus::InvalidArgument && !raw, "short input buffer");
	settings.apiVersion = 99;
	Check(AnalyzeImage(&view, &settings, &raw) == ImageStatus::VersionMismatch && !raw, "version rejection");
	GetDefaultImageSettings(&settings, sizeof(settings));
	settings.minArea = 1; settings.thresholdMode = ThresholdMode::NonZeroMask;
	std::vector<unsigned char> diagonal = {1, 0, 0, 1};
	ImageView diagonalView = View(diagonal, 2, 2, 2);
	Check(AnalyzeImage(&diagonalView, &settings, &raw) == ImageStatus::Ok, "nonzero mask");
	ResultPtr connected(raw, &ReleaseImageResult);
	GetBlobCount(connected.get(), &count);
	Check(count == 1, "diagonal is 8-connected");
	GetBlobFeatures(connected.get(), 0, &blob, sizeof(blob));
	Check(blob.shape.area == 2, "mask preserves non-255 foreground");
	std::vector<unsigned char> color = {0, 0, 255, 0};
	ImageView colorView = View(color, 1, 1, 4);
	colorView.format = PixelFormat::Bgra32;
	settings.thresholdMode = ThresholdMode::Fixed; settings.threshold = 76;
	Check(AnalyzeImage(&colorView, &settings, &raw) == ImageStatus::Ok, "BGRA input");
	ResultPtr colorResult(raw, &ReleaseImageResult);
	GetBlobFeatures(colorResult.get(), 0, &blob, sizeof(blob));
	Check(blob.meanGray == 77 && blob.shape.area == 1, "BGR weighting and ignored alpha");
	settings.threshold = 77;
	Check(AnalyzeImage(&colorView, &settings, &raw) == ImageStatus::Ok, "fixed equality");
	ResultPtr equalResult(raw, &ReleaseImageResult);
	GetBlobCount(equalResult.get(), &count); Check(count == 0, "strict threshold boundary");
	CheckBmp(false, false); CheckBmp(true, false); CheckBmp(false, true); CheckBmp(true, true);
	ReleaseImageResult(nullptr);
	std::printf("Image contract failures: %d\n", failures);
	return failures ? 1 : 0;
}
