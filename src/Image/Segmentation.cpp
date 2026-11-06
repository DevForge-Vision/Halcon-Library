#include "ImageInternal.h"
#include <algorithm>
#include <cmath>

namespace glim { namespace halcon { namespace image {
ImageStatus Segment(const GrayImage& input, const ImageSettings& settings, std::vector<Run>& runs)
{
	const int x0 = settings.roiX, y0 = settings.roiY;
	const int width = settings.roiWidth, height = settings.roiHeight;
	const int pitch = width + 1;
	std::vector<std::uint64_t> integral;
	if (settings.thresholdMode == ThresholdMode::LocalMean)
	{
		integral.assign(static_cast<std::size_t>(pitch) * (height + 1), 0);
		for (int y = 0; y < height; ++y)
		{
			std::uint64_t sum = 0;
			for (int x = 0; x < width; ++x)
			{
				sum += input.pixels[static_cast<std::size_t>(y + y0) * input.width + x + x0];
				integral[static_cast<std::size_t>(y + 1) * pitch + x + 1] = sum + integral[static_cast<std::size_t>(y) * pitch + x + 1];
			}
		}
	}
	runs.clear();
	for (int y = 0; y < height; ++y)
	{
		int begin = -1;
		for (int x = 0; x <= width; ++x)
		{
			bool selected = false;
			if (x < width)
			{
				const double value = input.pixels[static_cast<std::size_t>(y + y0) * input.width + x + x0];
				double reference = settings.threshold;
				if (settings.thresholdMode == ThresholdMode::LocalMean)
				{
					const int left = (std::max)(0, x - settings.backgroundRadius);
					const int right = (std::min)(width, x + settings.backgroundRadius + 1);
					const int top = (std::max)(0, y - settings.backgroundRadius);
					const int bottom = (std::min)(height, y + settings.backgroundRadius + 1);
					const auto at = [&](int r, int c) { return integral[static_cast<std::size_t>(r) * pitch + c]; };
					const double mean = static_cast<double>(at(bottom, right) - at(top, right) - at(bottom, left) + at(top, left)) / ((right - left) * (bottom - top));
					reference = mean + (settings.foreground == Foreground::Bright ? settings.threshold : -settings.threshold);
				}
				selected = settings.thresholdMode == ThresholdMode::NonZeroMask ? value != 0 :
					(settings.foreground == Foreground::Bright ? value > reference : value < reference);
			}
			if (selected && begin < 0) begin = x;
			if (!selected && begin >= 0)
			{
				runs.push_back(Run{y + y0, begin + x0, x - 1 + x0});
				begin = -1;
				if (runs.size() > 1000000) return ImageStatus::ResourceLimit;
			}
		}
	}
	return ImageStatus::Ok;
}
}}}
