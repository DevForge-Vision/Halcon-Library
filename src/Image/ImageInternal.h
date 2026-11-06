#ifndef GLIM_IMAGE_INTERNAL_H
#define GLIM_IMAGE_INTERNAL_H
#include "GlimImage.h"
#include "Region.h"
#include <vector>
namespace glim { namespace halcon { namespace image {
// Conservative bound for CPU/memory use, also avoids legacy long-area overflow.
const int MaxPixels = 16 * 1024 * 1024;
struct GrayImage
{
	int width;
	int height;
	std::vector<unsigned char> pixels;
};
ImageStatus DecodeBmp(const char* path, GrayImage& output);
ImageStatus ConvertImage(const ImageView& input, GrayImage& output);
ImageStatus Segment(const GrayImage& input, const ImageSettings& settings,
	std::vector<Run>& runs);
bool ComputeChecked(const core::RegionData& data, BlobFeatures& output, int expectedComponents = 1);
}}}
#endif
