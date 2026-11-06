#include <cstddef>
#include <cstdint>
#include <type_traits>
#pragma pack(push, ABI_TEST_PACK)
#ifdef ABI_IMAGE_FIRST
#include "GlimImage.h"
#include "GlimHalcon.h"
#else
#include "GlimHalcon.h"
#include "GlimImage.h"
#endif
#pragma warning(push)
#pragma warning(disable: 4121) // Deliberately test caller packing.
struct CallerPackingProbe { char c; double d; };
#pragma warning(pop)
#pragma pack(pop)
using namespace glim::halcon;
static_assert(offsetof(CallerPackingProbe, d) == ABI_TEST_PACK, "Caller packing not restored");
static_assert(offsetof(RegionFeatures, row) == 8, "RegionFeatures packing");
static_assert(sizeof(RegionFeatures) == 56, "RegionFeatures size");
static_assert(sizeof(RegionMoments) == 120, "RegionMoments size");
static_assert(sizeof(RegionTopology) == 40, "RegionTopology size");
static_assert(offsetof(BlobFeatures, shape) == 48, "Blob shape offset");
static_assert(sizeof(BlobFeatures) == 264, "Blob ABI size");
static_assert(offsetof(DefectFeatures, shape) == 56, "Defect shape offset");
static_assert(sizeof(DefectFeatures) == 272, "Defect ABI size");

static_assert(sizeof(ImageView) == (sizeof(void*) == 8 ? 40 : 32), "Image view size");
static_assert(offsetof(ImageView, data) == (sizeof(void*) == 8 ? 8 : 4), "Image data offset");
static_assert(offsetof(ImageView, dataBytes) == (sizeof(void*) == 8 ? 16 : 8), "Image buffer offset");
static_assert(sizeof(ImageSettings) == 56 && offsetof(ImageSettings, threshold) == 16, "Settings ABI");
static_assert(offsetof(ImageSettings, roiHeight) == 52, "ROI ABI");
static_assert(sizeof(DefectGroupingSettings) == 48, "Grouping ABI");
static_assert(offsetof(DefectGroupingSettings, axisAngleDegrees) == 8, "Grouping angle ABI");
static_assert(offsetof(DefectGroupingSettings, maxInputBlobs) == 40, "Grouping limit ABI");
static_assert(offsetof(BlobFeatures, moments) == 104 && offsetof(BlobFeatures, topology) == 224, "Blob nested ABI");
static_assert(offsetof(DefectFeatures, moments) == 112 && offsetof(DefectFeatures, topology) == 232, "Defect nested ABI");
static_assert(sizeof(ImageStatus) == 4 && sizeof(PixelFormat) == 4 && sizeof(ThresholdMode) == 4 && sizeof(Foreground) == 4, "Enum ABI");
static_assert(std::is_standard_layout<ImageView>::value && std::is_trivially_copyable<ImageView>::value, "Image view traits");
static_assert(std::is_standard_layout<BlobFeatures>::value && std::is_trivially_copyable<BlobFeatures>::value, "Blob traits");
static_assert(std::is_standard_layout<DefectFeatures>::value && std::is_trivially_copyable<DefectFeatures>::value, "Defect traits");
static_assert(int(ImageStatus::Ok) == 0 && int(ImageStatus::InvalidArgument) == 1 && int(ImageStatus::UnsupportedFormat) == 2 && int(ImageStatus::FileError) == 3 && int(ImageStatus::ResourceLimit) == 4 && int(ImageStatus::ProcessingFailed) == 5 && int(ImageStatus::OutOfMemory) == 6 && int(ImageStatus::VersionMismatch) == 7, "Status values");
static_assert(int(PixelFormat::Gray8) == 1 && int(PixelFormat::Bgr24) == 3 && int(PixelFormat::Bgra32) == 4, "Pixel format values");
static_assert(int(ThresholdMode::Fixed) == 0 && int(ThresholdMode::LocalMean) == 1 && int(ThresholdMode::NonZeroMask) == 2 && int(Foreground::Bright) == 0 && int(Foreground::Dark) == 1, "Segmentation enum values");