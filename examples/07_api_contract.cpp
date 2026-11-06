#include "GlimImage.h"
#include <cstdio>
#include <memory>

int main()
{
	using namespace glim::halcon;
	try
	{
		unsigned char pixels[64] = {};
		pixels[1 * 8 + 3] = pixels[2 * 8 + 3] = 200;
		pixels[5 * 8 + 3] = pixels[6 * 8 + 3] = 200;
		ImageView image = {};
		image.structSize = sizeof(image);
		image.data = pixels;
		image.dataBytes = sizeof(pixels);
		image.width = image.height = image.strideBytes = 8;
		image.format = PixelFormat::Gray8;
		ImageSettings settings = {};
		if (GetDefaultImageSettings(&settings, sizeof(settings)) != ImageStatus::Ok) return 1;
		settings.thresholdMode = ThresholdMode::NonZeroMask;
		settings.minArea = 1;
		ImageResult* rawImage = nullptr;
		const ImageStatus imageStatus = AnalyzeImage(&image, &settings, &rawImage);
		std::unique_ptr<ImageResult, decltype(&ReleaseImageResult)> blobs(rawImage, &ReleaseImageResult);
		if (imageStatus != ImageStatus::Ok) return 2;
		int blobCount = 0;
		if (GetBlobCount(blobs.get(), &blobCount) != ImageStatus::Ok || blobCount != 2) return 3;
		BlobFeatures blob = {};
		if (GetBlobFeatures(blobs.get(), 0, &blob, sizeof(blob)) != ImageStatus::Ok || blob.shape.area != 2) return 4;
		DefectGroupingSettings grouping = {};
		if (GetDefaultDefectGroupingSettings(&grouping, sizeof(grouping)) != ImageStatus::Ok) return 5;
		grouping.maxAlongGap = 3;
		grouping.maxAcrossOffset = 0;
		DefectResult* rawDefects = nullptr;
		const ImageStatus groupStatus = GroupBlobDefects(blobs.get(), &grouping, &rawDefects);
		std::unique_ptr<DefectResult, decltype(&ReleaseDefectResult)> defects(rawDefects, &ReleaseDefectResult);
		if (groupStatus != ImageStatus::Ok) return 6;
		blobs.reset(); // The grouped result is an independent snapshot.
		int defectCount = 0;
		if (GetDefectCount(defects.get(), &defectCount) != ImageStatus::Ok || defectCount != 1) return 7;
		DefectFeatures feature = {};
		if (GetDefectFeatures(defects.get(), 0, &feature, sizeof(feature)) != ImageStatus::Ok) return 8;
		if (feature.memberCount != 2 || feature.shape.area != 4 || feature.meanGray != 200 ||
			feature.shape.row != 3.5 || feature.shape.column != 3 || feature.topology.numConnected != 2) return 9;
		unsigned char mask[64] = {};
		if (CopyDefectMask(defects.get(), 0, mask, sizeof(mask), 8) != ImageStatus::Ok) return 10;
		for (int i = 0; i < 64; ++i)
			if (mask[i] != (pixels[i] ? 255 : 0)) return 11;
		std::printf("Blobs=%d Defects=%d Members=%d Area=%ld Row=%.1f Column=%.1f Mean=%.1f\n",
			blobCount, defectCount, feature.memberCount, feature.shape.area,
			feature.shape.row, feature.shape.column, feature.meanGray);
		return 0;
	}
	catch (...) { return 12; }
}
