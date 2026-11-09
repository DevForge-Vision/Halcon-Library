#ifndef GLIM_IMAGE_H
#define GLIM_IMAGE_H

#pragma pack(push, 8)
#include "GlimHalcon.h"
#pragma pack(pop)
#include <cstddef>
#include <cstdint>

namespace glim { namespace halcon {

enum class ImageStatus : int
{
	Ok = 0, InvalidArgument = 1, UnsupportedFormat = 2, FileError = 3,
	ResourceLimit = 4, ProcessingFailed = 5, OutOfMemory = 6, VersionMismatch = 7
};
enum class PixelFormat : int { Gray8 = 1, Bgr24 = 3, Bgra32 = 4 };
enum class ThresholdMode : int { Fixed = 0, LocalMean = 1, NonZeroMask = 2 };
enum class Foreground : int { Bright = 0, Dark = 1 };

// ABI v1: packing is fixed; do not change these structures in place.
#pragma pack(push, 8)
struct ImageView
{
	std::uint32_t structSize;
	const unsigned char* data;
	std::uint64_t dataBytes;
	int width;
	int height;
	int strideBytes; // Positive, top-down rows. Buffer remains owned by caller.
	PixelFormat format;
};
struct ImageSettings
{
	std::uint32_t structSize;
	std::uint32_t apiVersion;
	ThresholdMode thresholdMode;
	Foreground foreground;
	double threshold; // Fixed: gray level. LocalMean: positive contrast offset.
	int backgroundRadius;
	int minArea;
	int maxArea; // Zero: unlimited.
	int maxBlobs; // Exceeding limit fails the entire call; never truncates.
	int roiX;
	int roiY;
	int roiWidth; // Both width/height zero: full image.
	int roiHeight;
};
struct BlobFeatures
{
	std::uint32_t structSize;
	int blobIndex;
	int x;
	int y;
	int width;
	int height;
	int touchesImageBorder;
	int touchesRoiBorder;
	double meanGray;
	double deviationGray; // Population standard deviation, original gray pixels.
	RegionFeatures shape;
	RegionMoments moments;
	RegionTopology topology;
};
#pragma pack(pop)

struct ImageResult; // Created and destroyed only inside the library.
// Every result is independent. Concurrent reads are allowed; never release while
// another thread is reading. Use only live handles returned by this DLL.
// Analyze calls set *result=null on failure; release any previous handle first.
GLIM_HALCON_API ImageStatus GLIM_HALCON_CALL GetDefaultImageSettings(ImageSettings* settings, std::uint32_t bytes);
GLIM_HALCON_API ImageStatus GLIM_HALCON_CALL AnalyzeImage(const ImageView* image, const ImageSettings* settings, ImageResult** result);
// Windows narrow paths use the current ANSI code page (MFC multibyte).
// BMP only: uncompressed 8-bit palette, 24-bit BGR, 32-bit BGRX.
GLIM_HALCON_API ImageStatus GLIM_HALCON_CALL AnalyzeBmpFile(const char* path, const ImageSettings* settings, ImageResult** result);
GLIM_HALCON_API ImageStatus GLIM_HALCON_CALL GetBlobCount(const ImageResult* result, int* count);
GLIM_HALCON_API ImageStatus GLIM_HALCON_CALL GetBlobFeatures(const ImageResult* result, int index, BlobFeatures* features, std::uint32_t bytes);
// Writes one selected blob as 0/255 into a full-image, top-down Gray8 buffer.
GLIM_HALCON_API ImageStatus GLIM_HALCON_CALL CopyBlobMask(const ImageResult* result, int index, unsigned char* data, std::uint64_t bytes, int stride);
GLIM_HALCON_API void GLIM_HALCON_CALL ReleaseImageResult(ImageResult* result);

// Optional grouping; existing image structures and functions are unchanged.
#pragma pack(push, 8)
struct DefectGroupingSettings
{
	std::uint32_t structSize;
	std::uint32_t apiVersion;
	double axisAngleDegrees; // 0=right, 90=down, [0,180), image coordinates.
	double maxAlongGap; // Projected pixel-center interval distance, inclusive.
	double maxAcrossOffset; // Maximum transverse centroid spread of entire group.
	double maxGroupLength; // Projected span; zero disables this merge constraint.
	int maxInputBlobs; // 1..4096. Exceeding limits returns no partial result.
};
struct DefectFeatures
{
	std::uint32_t structSize;
	int defectIndex;
	int memberCount;
	int x, y, width, height;
	int touchesImageBorder;
	int touchesRoiBorder;
	double meanGray;
	double deviationGray;
	RegionFeatures shape;
	RegionMoments moments;
	RegionTopology topology; // numConnected remains the fragment count.
};
#pragma pack(pop)
struct DefectResult;
// HALCON 13 region_features names 1..20; enum values are zero-based indices.
enum class RegionFeature20 : int
{
	Anisometry, Area, AreaHoles, Bulkiness, Circularity, Column, Column1, Column2,
	Compactness, ConnectNum, Contlength, Convexity, DistDeviation, DistMean,
	EulerNumber, Height, HolesNum, InnerHeight, InnerRadius, InnerWidth
};
#pragma pack(push, 8)
struct RegionFeatures20
{
	std::uint32_t structSize;
	std::uint32_t apiVersion;
	std::uint32_t provisionalMask; // Bit i: definition still needs HALCON calibration.
	std::uint32_t policyFlags; // 1=external contour, 2=rectangle perimeter, 4=cell radius.
	double values[20]; // Read only when the API returns Ok. No HALCON parity claim.
};
#pragma pack(pop)
// On-demand calculation; existing result layouts and analysis behavior are unchanged.
// Bounding box plus 1-pixel padding is limited to 4,194,304 pixels.
GLIM_HALCON_API ImageStatus GLIM_HALCON_CALL GetBlobRegionFeatures20(const ImageResult* result, int index, RegionFeatures20* features, std::uint32_t bytes);
GLIM_HALCON_API ImageStatus GLIM_HALCON_CALL GetDefectRegionFeatures20(const DefectResult* result, int index, RegionFeatures20* features, std::uint32_t bytes);
GLIM_HALCON_API ImageStatus GLIM_HALCON_CALL GetDefaultDefectGroupingSettings(DefectGroupingSettings* settings, std::uint32_t bytes);
// Independent snapshot, valid after releasing the source. No pixels are invented.
GLIM_HALCON_API ImageStatus GLIM_HALCON_CALL GroupBlobDefects(const ImageResult* source, const DefectGroupingSettings* settings, DefectResult** result);
GLIM_HALCON_API ImageStatus GLIM_HALCON_CALL GetDefectCount(const DefectResult* result, int* count);
GLIM_HALCON_API ImageStatus GLIM_HALCON_CALL GetDefectFeatures(const DefectResult* result, int index, DefectFeatures* features, std::uint32_t bytes);
GLIM_HALCON_API ImageStatus GLIM_HALCON_CALL GetDefectMemberBlobIndex(const DefectResult* result, int index, int memberIndex, int* blobIndex);
GLIM_HALCON_API ImageStatus GLIM_HALCON_CALL CopyDefectMask(const DefectResult* result, int index, unsigned char* data, std::uint64_t bytes, int stride);
GLIM_HALCON_API void GLIM_HALCON_CALL ReleaseDefectResult(DefectResult* result);

}} // namespace glim::halcon
#endif
