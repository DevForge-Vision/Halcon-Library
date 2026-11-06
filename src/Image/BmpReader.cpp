#include "ImageInternal.h"
#include <fstream>
#include <limits>

namespace glim { namespace halcon { namespace image {
namespace {
unsigned int U16(const unsigned char* p)
{
	return p[0] | (static_cast<unsigned int>(p[1]) << 8);
}
std::uint32_t U32(const unsigned char* p)
{
	return U16(p) | (static_cast<std::uint32_t>(U16(p + 2)) << 16);
}
unsigned char Gray(const unsigned char* p)
{
	return static_cast<unsigned char>((29 * p[0] + 150 * p[1] + 77 * p[2] + 128) >> 8);
}
}

ImageStatus ConvertImage(const ImageView& input, GrayImage& output)
{
	if (input.structSize != sizeof(ImageView)) return ImageStatus::VersionMismatch;
	const int channels = static_cast<int>(input.format);
	if (channels != 1 && channels != 3 && channels != 4) return ImageStatus::UnsupportedFormat;
	if (!input.data || input.width <= 0 || input.height <= 0 || input.strideBytes <= 0)
		return ImageStatus::InvalidArgument;
	const std::uint64_t pixels = static_cast<std::uint64_t>(input.width) * input.height;
	if (pixels > MaxPixels) return ImageStatus::ResourceLimit;
	const std::uint64_t rowBytes = static_cast<std::uint64_t>(input.width) * channels;
	const std::uint64_t required = static_cast<std::uint64_t>(input.height - 1) * input.strideBytes + rowBytes;
	if (rowBytes > static_cast<std::uint64_t>(input.strideBytes) || required > input.dataBytes ||
		required > (std::numeric_limits<std::size_t>::max)()) return ImageStatus::InvalidArgument;
	output.width = input.width;
	output.height = input.height;
	output.pixels.resize(static_cast<std::size_t>(pixels));
	for (int y = 0; y < input.height; ++y)
	{
		const unsigned char* row = input.data + static_cast<std::size_t>(y) * input.strideBytes;
		for (int x = 0; x < input.width; ++x)
			output.pixels[static_cast<std::size_t>(y) * input.width + x] = channels == 1 ? row[x] : Gray(row + x * channels);
	}
	return ImageStatus::Ok;
}

ImageStatus DecodeBmp(const char* path, GrayImage& output)
{
	if (!path || !*path) return ImageStatus::InvalidArgument;
	std::ifstream stream(path, std::ios::binary | std::ios::ate);
	if (!stream) return ImageStatus::FileError;
	const std::streamoff fileBytes = stream.tellg();
	if (fileBytes < 54) return ImageStatus::UnsupportedFormat;
	stream.seekg(0);
	unsigned char header[54] = {};
	if (!stream.read(reinterpret_cast<char*>(header), sizeof(header))) return ImageStatus::FileError;
	if (header[0] != 'B' || header[1] != 'M') return ImageStatus::UnsupportedFormat;
	const std::uint32_t offset = U32(header + 10);
	const std::uint32_t dibSize = U32(header + 14);
	const std::uint32_t width = U32(header + 18);
	const std::uint32_t rawHeight = U32(header + 22);
	const bool topDown = (rawHeight & 0x80000000u) != 0;
	const std::uint32_t height = topDown ? (~rawHeight + 1u) : rawHeight;
	const unsigned int bits = U16(header + 28);
	if ((dibSize != 40 && dibSize != 108 && dibSize != 124) || U16(header + 26) != 1 ||
		U32(header + 30) != 0 || (bits != 8 && bits != 24 && bits != 32)) return ImageStatus::UnsupportedFormat;
	if (!width || !height || width > MaxPixels || height > MaxPixels ||
		static_cast<std::uint64_t>(width) * height > MaxPixels) return ImageStatus::ResourceLimit;
	const std::uint64_t rowBytes = ((static_cast<std::uint64_t>(width) * bits + 31) / 32) * 4;
	const std::uint64_t end = offset + rowBytes * height;
	if (offset < 14u + dibSize || end > static_cast<std::uint64_t>(fileBytes)) return ImageStatus::UnsupportedFormat;
	unsigned char palette[256] = {};
	std::uint32_t paletteSize = 0;
	if (bits == 8)
	{
		paletteSize = U32(header + 46);
		if (!paletteSize) paletteSize = 256;
		if (paletteSize > 256 || 14ull + dibSize + paletteSize * 4ull > offset) return ImageStatus::UnsupportedFormat;
		stream.seekg(14u + dibSize);
		for (std::uint32_t i = 0; i < paletteSize; ++i)
		{
			unsigned char entry[4];
			if (!stream.read(reinterpret_cast<char*>(entry), 4)) return ImageStatus::FileError;
			palette[i] = Gray(entry);
		}
	}
	output.width = static_cast<int>(width);
	output.height = static_cast<int>(height);
	output.pixels.resize(static_cast<std::size_t>(width) * height);
	std::vector<unsigned char> row(static_cast<std::size_t>(rowBytes));
	stream.seekg(offset);
	for (std::uint32_t y = 0; y < height; ++y)
	{
		if (!stream.read(reinterpret_cast<char*>(row.data()), static_cast<std::streamsize>(row.size()))) return ImageStatus::FileError;
		const std::uint32_t targetY = topDown ? y : height - 1 - y;
		for (std::uint32_t x = 0; x < width; ++x)
		{
			if (bits == 8 && row[x] >= paletteSize) return ImageStatus::UnsupportedFormat;
			output.pixels[static_cast<std::size_t>(targetY) * width + x] = bits == 8 ? palette[row[x]] : Gray(&row[x * (bits / 8)]);
		}
	}
	return ImageStatus::Ok;
}
}}}
