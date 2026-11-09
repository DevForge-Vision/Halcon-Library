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
struct DefectResult
{
	int m_width, m_height;
	std::vector<DefectFeatures> m_features;
	std::vector<std::vector<int>> m_members;
	std::vector<std::vector<Run>> m_runs;
};
}}
