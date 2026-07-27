//====================================================================================
// src/Facade/Inspection.cpp — 검사기 연동용 편의 API
//
//   SelectLargestBlob          : 최대 연결성분(8-연결) 하나만 남긴다
//   ComputeLargestBlobFeatures : 이진 마스크 한 장 → 최대 blob 의 특징값 7개 (원샷)
//
// HALCON 원명 대응이 **없는** 함수들이다. 존재 이유는 하나다.
//   검사기는 이진화된 crop 한 장을 던지고 "이 불량 하나"의 특징값을 원한다.
//   그런데 Region::FromMask 는 마스크 전체를 **하나의 리전**으로 만든다.
//   덩어리가 여럿이면 전부 합쳐져 (HALCON 의 connection 이 없으므로) 값이 섞인다.
//   그 사이를 메우는 것이 이 파일이다.
//
// 설계 원칙
//   - 계산 로직을 새로 쓰지 않는다. FromMask → SelectLargestBlob → ComputeFeatures 조합이다.
//   - 성분 라벨링은 Contour.cpp 의 LabelConnectedRuns() 를 **그대로 재사용**한다.
//     윤곽 추적이 보는 성분과 여기서 고르는 성분이 어긋날 여지를 만들지 않기 위해서다.
//   - 예외를 공개 경계 밖으로 던지지 않는다.
//====================================================================================
#include "GlimHalcon.h"

#include "Region.h"
#include "Contour.h"

#include <cstddef>
#include <vector>

namespace glim { namespace halcon {

namespace {

//------------------------------------------------------------------------------------
// 전 필드 0. 실패 경로와 진입 경로가 **같은 함수**를 쓰도록 한 곳에 둔다.
//------------------------------------------------------------------------------------
void ClearFeatures(RegionFeatures& features)
{
	features.area = 0;
	features.row = 0.0;
	features.column = 0.0;
	features.contLength = 0.0;
	features.circularity = 0.0;
	features.compactness = 0.0;
	features.convexity = 0.0;
}

}	// anonymous namespace

//------------------------------------------------------------------------------------
// 최대 연결성분(8-연결) 하나만 남긴 리전을 돌려준다.
//
//   "최대" 의 기준은 **면적(픽셀 개수)** 이다.
//   동률이면 스캔 순서(위→아래, 왼→오른쪽)상 먼저 나온 성분을 택한다.
//     → LabelConnectedRuns 가 라벨을 "성분의 최초 런 인덱스 오름차순" 으로 부여하므로
//       동률에서 **더 작은 라벨** 을 고르는 것이 곧 스캔 순서상 먼저인 것을 고르는 것이다.
//
//   빈 리전이거나 실패하면 빈 리전을 돌려준다.
//
//   성분이 이미 하나뿐이면 결과는 입력과 **픽셀 단위로 동일**하다
//   (같은 런을 그대로 다시 담을 뿐이다). 즉 단일 blob 마스크에서는 특징값이 변하지 않는다.
//------------------------------------------------------------------------------------
Region SelectLargestBlob(const Region& region)
{
	Region result;

	try
	{
		const core::RegionData* data = RegionInternal::GetData(region);
		if (data == 0 || data->IsEmpty())
			return result;		// 빈 리전 규약

		const std::vector<Run>& runs = data->GetRuns();

		std::vector<int> labelOfRun;
		std::vector<int> firstRunOfLabel;
		std::vector<long long> pixelsOfLabel;

		// ★ 윤곽 추적(TraceOuterContours)이 쓰는 것과 동일한 라벨링 함수다.
		if (!core::LabelConnectedRuns(runs, labelOfRun, firstRunOfLabel, pixelsOfLabel))
			return result;

		if (pixelsOfLabel.empty())
			return result;

		// 성분이 이미 하나뿐이면 고를 것이 없다. 원본을 그대로 돌려준다.
		//   Region 복사는 shared_ptr 공유라 O(1) 이고, 리전은 불변이라 안전하다.
		//   런을 다시 담아 재정규화(O(n log n))하는 낭비를 피한다. 값은 완전히 동일하다.
		if (pixelsOfLabel.size() == 1)
			return region;

		// 최대 면적 성분. 동률이면 라벨이 작은 쪽(= 스캔 순서상 먼저)을 유지한다.
		std::size_t bestLabel = 0;
		for (std::size_t label = 1; label < pixelsOfLabel.size(); ++label)
		{
			if (pixelsOfLabel[label] > pixelsOfLabel[bestLabel])
				bestLabel = label;
		}

		const int bestLabelId = static_cast<int>(bestLabel);

		std::vector<Run> picked;
		picked.reserve(runs.size());
		for (std::size_t i = 0; i < runs.size(); ++i)
		{
			if (i < labelOfRun.size() && labelOfRun[i] == bestLabelId)
				picked.push_back(runs[i]);
		}

		if (picked.empty())
			return result;		// 방어: 정상 입력에서는 도달하지 않는다

		std::shared_ptr<core::RegionData> picked2(new core::RegionData());
		picked2->Assign(picked);	// 정규형은 이미 만족하지만, 생성 경로를 통일한다

		result = RegionInternal::Create(picked2);
	}
	catch (...)
	{
		result = Region();		// 실패 시 빈 리전
	}

	return result;
}

//------------------------------------------------------------------------------------
// 이진 마스크 한 장에서 최대 blob 을 골라 특징값 7개를 전부 계산한다.
//
// 내부는 FromMask → SelectLargestBlob → ComputeFeatures 조합이다.
// 계산식은 **한 줄도 새로 쓰지 않았다.** 값은 세 함수를 손으로 조합한 것과 완전히 같다.
//
// 반환값이 bool 인 이유는 함수 선언부 주석(include/GlimHalcon.h) 참조.
//------------------------------------------------------------------------------------
bool ComputeLargestBlobFeatures(const unsigned char* data,
								int width, int height, int stride,
								RegionFeatures& features)
{
	ClearFeatures(features);

	try
	{
		// 입력 검증 — 여기서 false 를 돌려주는 것이 이 API 의 존재 이유의 절반이다.
		// FromMask 도 같은 조건을 빈 리전으로 거르지만, 그러면 "잘못된 입력" 과
		// "전경이 없는 정상 마스크" 가 호출자에게 똑같이 보인다. 그 둘을 구분해 준다.
		if (data == 0 || width <= 0 || height <= 0)
			return false;

		// 0 < stride < width 는 행 우선 버퍼로 성립할 수 없다 (Region::FromMask 계약 참조).
		if (stride > 0 && stride < width)
			return false;

		const Region full = Region::FromMask(data, width, height, stride);
		if (full.IsEmpty())
			return false;		// 전경이 하나도 없다

		const Region largest = SelectLargestBlob(full);
		if (largest.IsEmpty())
			return false;		// 방어: 위에서 비어 있지 않았으므로 정상 입력에선 도달하지 않는다

		ComputeFeatures(largest, features);
		return true;
	}
	catch (...)
	{
		ClearFeatures(features);
		return false;
	}
}

}}	// namespace glim::halcon
