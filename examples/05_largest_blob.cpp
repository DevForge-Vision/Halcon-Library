//====================================================================================
// examples/05_largest_blob.cpp — 검사기 연동 원샷 API 사용법과 교차 검증
//
// 이 예제가 보여주는 것 (그리고 매 빌드마다 검사하는 것)
//   1) 덩어리가 3개인 마스크 → ComputeLargestBlobFeatures 가 **최대 것만** 고르는지
//   2) 최대 면적이 동률일 때의 동작 (스캔 순서상 먼저인 것)
//   3) 전경이 전혀 없는 마스크 → false 반환 + 전 필드 0
//   4) 널 포인터 / 0 < stride < width → false 반환
//   5) ComputeLargestBlobFeatures 결과 == FromMask + SelectLargestBlob + ComputeFeatures
//      수동 조합 결과 (비트 단위 == 비교)
//
// 하나라도 어긋나면 exit code 1 을 돌려준다 (회귀 감시용).
//
// 외부 의존성 0. 표준 라이브러리만 쓴다.
//
// 빌드 (MSVC 개발자 명령 프롬프트, 저장소 루트에서)
//   cl /nologo /EHsc /W4 /D_MBCS /I include /I src\Core /I src\Feature ^
//      src\Core\*.cpp src\Feature\*.cpp src\Facade\*.cpp examples\05_largest_blob.cpp ^
//      /Fe:largest.exe
//====================================================================================
#include "GlimHalcon.h"

#include <cstddef>
#include <cstdio>
#include <vector>

using namespace glim::halcon;

namespace {

int gFailCount = 0;

//------------------------------------------------------------------------------------
// 마스크 버퍼 (examples/02, 04 와 같은 형태)
//------------------------------------------------------------------------------------
class MaskBuffer
{
public:
	MaskBuffer(int width, int height, int stride)
		: m_width(width)
		, m_height(height)
		, m_stride((stride > 0) ? stride : width)
		, m_data(static_cast<std::size_t>((stride > 0) ? stride : width) * static_cast<std::size_t>(height), 0)
	{
	}

	void FillRect(int rowBegin, int rowEnd, int colBegin, int colEnd, unsigned char value)
	{
		for (int row = rowBegin; row <= rowEnd; ++row)
		{
			for (int col = colBegin; col <= colEnd; ++col)
				Set(row, col, value);
		}
	}

	void Set(int row, int col, unsigned char value)
	{
		if (row < 0 || row >= m_height || col < 0 || col >= m_width)
			return;
		m_data[static_cast<std::size_t>(row) * static_cast<std::size_t>(m_stride) + static_cast<std::size_t>(col)] = value;
	}

	const unsigned char* GetData() const { return &m_data[0]; }
	int GetWidth()  const { return m_width; }
	int GetHeight() const { return m_height; }
	int GetStride() const { return m_stride; }

private:
	int m_width;
	int m_height;
	int m_stride;
	std::vector<unsigned char> m_data;
};

//------------------------------------------------------------------------------------
// 두 RegionFeatures 가 비트 단위로 같은지
//------------------------------------------------------------------------------------
bool SameFeatures(const RegionFeatures& a, const RegionFeatures& b)
{
	return
		(a.area == b.area) &&
		(a.row == b.row) &&
		(a.column == b.column) &&
		(a.contLength == b.contLength) &&
		(a.circularity == b.circularity) &&
		(a.compactness == b.compactness) &&
		(a.convexity == b.convexity);
}

void PrintFeatures(const char* label, const RegionFeatures& f)
{
	std::printf("%-24s area=%-6ld row=%9.4f col=%9.4f cont=%10.4f circ=%.6f comp=%.6f conv=%.6f\n",
				label, f.area, f.row, f.column, f.contLength, f.circularity, f.compactness, f.convexity);
}

//------------------------------------------------------------------------------------
// 원샷 API == 수동 조합(FromMask + SelectLargestBlob + ComputeFeatures) 대조
//------------------------------------------------------------------------------------
void CheckOneShotEqualsManual(const char* name, const MaskBuffer& buffer)
{
	// (1) 원샷
	RegionFeatures oneShot;
	const bool ok = ComputeLargestBlobFeatures(
		buffer.GetData(), buffer.GetWidth(), buffer.GetHeight(), buffer.GetStride(), oneShot);

	// (2) 수동 조합
	Region full = Region::FromMask(buffer.GetData(), buffer.GetWidth(), buffer.GetHeight(), buffer.GetStride());
	Region largest = SelectLargestBlob(full);
	RegionFeatures manual;
	ComputeFeatures(largest, manual);

	const bool same = SameFeatures(oneShot, manual);
	// 원샷이 성공했으면 수동 리전도 비어 있지 않아야 하고, 그 반대도 성립해야 한다.
	const bool okConsistent = (ok == !largest.IsEmpty());

	PrintFeatures(name, oneShot);

	if (!same || !okConsistent)
	{
		++gFailCount;
		std::printf("   *** MISMATCH ***  ok=%s  largest.IsEmpty=%s\n",
					ok ? "true" : "false", largest.IsEmpty() ? "true" : "false");
		PrintFeatures("   oneShot", oneShot);
		PrintFeatures("   manual ", manual);
	}
}

//------------------------------------------------------------------------------------
// 케이스 1 — 덩어리 3개. 최대(가운데 큰 사각형)만 뽑혀야 한다.
//------------------------------------------------------------------------------------
void Case_ThreeBlobs_PickLargest()
{
	std::printf("[1] 덩어리 3개 -> 최대만 선택 ----------------------------------------------------\n");

	const int canvas = 60;
	MaskBuffer buffer(canvas, canvas, canvas);
	buffer.FillRect(2, 6, 2, 6, 255);			// 작은 것 : 5x5 = 25
	buffer.FillRect(20, 39, 20, 39, 255);		// 큰 것   : 20x20 = 400
	buffer.FillRect(50, 55, 50, 55, 255);		// 중간 것 : 6x6 = 36

	// 원샷의 최대 blob 면적이 400 이어야 한다.
	RegionFeatures f;
	const bool ok = ComputeLargestBlobFeatures(buffer.GetData(), canvas, canvas, canvas, f);
	if (!ok || f.area != 400)
	{
		++gFailCount;
		std::printf("   *** 최대 blob 면적이 400 이 아님: ok=%s area=%ld ***\n", ok ? "true" : "false", f.area);
	}

	CheckOneShotEqualsManual("ThreeBlobs", buffer);
	std::printf("\n");
}

//------------------------------------------------------------------------------------
// 케이스 2 — 면적 동률. 스캔 순서상 먼저인 것(위쪽)을 택해야 한다.
//------------------------------------------------------------------------------------
void Case_TieBreak_ScanOrder()
{
	std::printf("[2] 면적 동률 -> 스캔 순서상 먼저(위쪽) 선택 --------------------------------------\n");

	const int canvas = 60;
	MaskBuffer buffer(canvas, canvas, canvas);
	// 같은 크기(10x10=100) 두 개. 위쪽이 먼저 스캔된다.
	buffer.FillRect(5, 14, 5, 14, 255);			// 위쪽 : row 5..14
	buffer.FillRect(40, 49, 40, 49, 255);		// 아래쪽 : row 40..49

	RegionFeatures f;
	const bool ok = ComputeLargestBlobFeatures(buffer.GetData(), canvas, canvas, canvas, f);

	// 위쪽 사각형의 무게중심 row 는 9.5, 아래쪽은 44.5. 위쪽이 선택돼야 한다.
	if (!ok || f.area != 100)
	{
		++gFailCount;
		std::printf("   *** 면적이 100 이 아님: ok=%s area=%ld ***\n", ok ? "true" : "false", f.area);
	}
	if (!(f.row < 20.0))
	{
		++gFailCount;
		std::printf("   *** 위쪽(row 약 9.5)이 선택되지 않음: row=%.4f ***\n", f.row);
	}

	CheckOneShotEqualsManual("TieBreak(top wins)", buffer);
	std::printf("\n");
}

//------------------------------------------------------------------------------------
// 케이스 3 — 단일 blob. SelectLargestBlob 이 값을 바꾸면 안 된다.
//   (마스크가 이미 단일 성분이면 FromMask 결과와 최대 blob 선택 결과가 동일해야 한다)
//------------------------------------------------------------------------------------
void Case_SingleBlob_Unchanged()
{
	std::printf("[3] 단일 blob -> 값 불변 (SelectLargestBlob 이 통과만) ---------------------------\n");

	const int canvas = 40;
	MaskBuffer buffer(canvas, canvas, canvas);
	buffer.FillRect(5, 24, 10, 29, 255);		// 정사각형 20x20 (예제 02 의 Rectangle 과 동일)

	Region full = Region::FromMask(buffer.GetData(), canvas, canvas, canvas);
	Region largest = SelectLargestBlob(full);

	RegionFeatures fFull;
	RegionFeatures fLargest;
	ComputeFeatures(full, fFull);
	ComputeFeatures(largest, fLargest);

	PrintFeatures("full (FromMask)", fFull);
	PrintFeatures("largest (선택 후)", fLargest);

	if (!SameFeatures(fFull, fLargest))
	{
		++gFailCount;
		std::printf("   *** 단일 blob 에서 값이 변함 ***\n");
	}
	std::printf("\n");
}

//------------------------------------------------------------------------------------
// 케이스 4 — 전경이 전혀 없는 마스크 -> false + 전 필드 0
//------------------------------------------------------------------------------------
void Case_EmptyForeground()
{
	std::printf("[4] 전경 없음 -> false + 전 필드 0 ----------------------------------------------\n");

	const int canvas = 32;
	MaskBuffer buffer(canvas, canvas, canvas);	// 전부 0

	RegionFeatures f;
	const bool ok = ComputeLargestBlobFeatures(buffer.GetData(), canvas, canvas, canvas, f);

	const bool allZero =
		(f.area == 0) && (f.row == 0.0) && (f.column == 0.0) && (f.contLength == 0.0) &&
		(f.circularity == 0.0) && (f.compactness == 0.0) && (f.convexity == 0.0);

	std::printf("   ok=%s (기대 false), allZero=%s (기대 true)\n",
				ok ? "true" : "false", allZero ? "true" : "false");

	if (ok || !allZero)
	{
		++gFailCount;
		std::printf("   *** 전경 없는 마스크 처리 실패 ***\n");
	}
	std::printf("\n");
}

//------------------------------------------------------------------------------------
// 케이스 5 — 잘못된 입력: 널 포인터 / 0 < stride < width -> false + 전 필드 0
//------------------------------------------------------------------------------------
void Case_InvalidInput()
{
	std::printf("[5] 잘못된 입력 -> false + 전 필드 0 --------------------------------------------\n");

	const int width = 40;
	const int height = 40;
	std::vector<unsigned char> raw(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 255);

	// (a) 널 포인터
	{
		RegionFeatures f;
		const bool ok = ComputeLargestBlobFeatures(0, width, height, width, f);
		const bool allZero = (f.area == 0 && f.row == 0.0 && f.contLength == 0.0);
		std::printf("   null data          : ok=%s (기대 false)\n", ok ? "true" : "false");
		if (ok || !allZero) { ++gFailCount; std::printf("   *** null data 처리 실패 ***\n"); }
	}

	// (b) width <= 0
	{
		RegionFeatures f;
		const bool ok = ComputeLargestBlobFeatures(&raw[0], 0, height, 0, f);
		std::printf("   width = 0          : ok=%s (기대 false)\n", ok ? "true" : "false");
		if (ok) { ++gFailCount; std::printf("   *** width<=0 처리 실패 ***\n"); }
	}

	// (c) 0 < stride < width
	{
		RegionFeatures f;
		const bool ok = ComputeLargestBlobFeatures(&raw[0], width, height, width - 8, f);
		const bool allZero = (f.area == 0 && f.row == 0.0 && f.contLength == 0.0);
		std::printf("   stride=%d (<width) : ok=%s (기대 false)\n", width - 8, ok ? "true" : "false");
		if (ok || !allZero) { ++gFailCount; std::printf("   *** 0<stride<width 처리 실패 ***\n"); }
	}

	// (d) stride <= 0 은 width 로 간주 -> 정상 성공해야 한다 (기존 규약 회귀 감시)
	{
		RegionFeatures f;
		const bool ok = ComputeLargestBlobFeatures(&raw[0], width, height, 0, f);
		std::printf("   stride = 0         : ok=%s (기대 true, width 로 간주)  area=%ld\n",
					ok ? "true" : "false", f.area);
		if (!ok || f.area != static_cast<long>(width) * height)
		{
			++gFailCount;
			std::printf("   *** stride<=0 규약 회귀 ***\n");
		}
	}
	std::printf("\n");
}

}	// anonymous namespace

//====================================================================================
int main()
{
	std::printf("====================================================================================\n");
	std::printf(" GlimHalcon 예제 05 - 최대 blob 선택 + 원샷 특징값 API\n");
	std::printf("====================================================================================\n\n");

	Case_ThreeBlobs_PickLargest();
	Case_TieBreak_ScanOrder();
	Case_SingleBlob_Unchanged();
	Case_EmptyForeground();
	Case_InvalidInput();

	std::printf("[쓰는 법] --------------------------------------------------------------------------\n");
	std::printf("  RegionFeatures f;\n");
	std::printf("  if (ComputeLargestBlobFeatures(mask, w, h, stride, f)) {\n");
	std::printf("      // f.area / f.row / f.column / f.contLength / f.circularity / f.compactness / f.convexity\n");
	std::printf("  } else {\n");
	std::printf("      // 입력이 잘못됐거나 전경이 없다 (f 는 전 필드 0)\n");
	std::printf("  }\n\n");

	if (gFailCount != 0)
	{
		std::printf("FAIL %d\n", gFailCount);
		return 1;
	}

	std::printf("ALL OK\n");
	return 0;
}
