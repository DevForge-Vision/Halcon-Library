#include "GlimImage.h"
#include <cstdio>
#include <memory>

int main(int argc, char** argv)
{
	using namespace glim::halcon;
	if (argc != 2)
	{
		std::printf("Usage: 06_image_blobs image.bmp\n");
		return 1;
	}
	ImageSettings settings = {};
	if (GetDefaultImageSettings(&settings, sizeof(settings)) != ImageStatus::Ok) return 2;
	ImageResult* raw = nullptr;
	const ImageStatus status = AnalyzeBmpFile(argv[1], &settings, &raw);
	std::unique_ptr<ImageResult, decltype(&ReleaseImageResult)> result(raw, &ReleaseImageResult);
	if (status != ImageStatus::Ok)
	{
		std::printf("Analysis failed: %d\n", static_cast<int>(status));
		return 3;
	}
	int count = 0;
	if (GetBlobCount(result.get(), &count) != ImageStatus::Ok) return 4;
	std::printf("Blob,Area,Row,Column,MeanGray,DeviationGray,Ra,Rb\n");
	for (int i = 0; i < count; ++i)
	{
		BlobFeatures blob = {};
		if (GetBlobFeatures(result.get(), i, &blob, sizeof(blob)) != ImageStatus::Ok) return 5;
		std::printf("%d,%ld,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g\n",
			i, blob.shape.area, blob.shape.row, blob.shape.column,
			blob.meanGray, blob.deviationGray, blob.moments.ra, blob.moments.rb);
	}
	return 0;
}
