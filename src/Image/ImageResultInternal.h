#pragma once
#include "ImageInternal.h"
namespace glim { namespace halcon {
struct ImageResult
{
	int m_width;
	int m_height;
	std::vector<BlobFeatures> m_features;
	std::vector<std::vector<Run>> m_runs;
};
}}
