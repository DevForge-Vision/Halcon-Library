//====================================================================================
// GlimHalcon.h
//
// MVTec HALCON 13 의 Regions ▸ Features 오퍼레이터를 SDK 없이 재현하는 라이브러리의
// **유일한 공개 헤더**다.
//
// 규칙 (docs/01_ARCHITECTURE.md §6)
//   - 외부 타입(OpenCV 등)을 이 헤더에 노출하지 않는다. Region 은 PIMPL 이다.
//   - 좌표계는 HALCON 관례 (Row, Column) = (y, x) 를 따른다.
//   - 출력은 참조 인자로 받는다 (HALCON 시그니처와 1:1 대응).
//   - 예외를 이 경계 밖으로 던지지 않는다. 내부에서 잡고 빈 리전 규약(0)으로 축약한다.
//====================================================================================
#ifndef GLIM_HALCON_H
#define GLIM_HALCON_H

namespace glim { namespace halcon {

//------------------------------------------------------------------------------------
// Region — 런렝스(run-length) 기반 불변 리전
//   내부 표현은 PIMPL 로 완전히 감춘다. 복사는 O(1) (내부 데이터 공유).
//------------------------------------------------------------------------------------
class Region
{
public:
	Region();
	~Region();
	Region(const Region& other);
	Region& operator=(const Region& other);

	// 마스크(0 이 아닌 값 = 전경)로부터 생성한다.
	//   data   : 좌상단부터 행 우선으로 저장된 바이트 버퍼
	//   width  : 열 개수(column), height : 행 개수(row)
	//   stride : 한 행의 바이트 수 (0 이하이면 width 로 간주)
	// 좌표 대응: row = y (0..height-1), col = x (0..width-1)
	static Region FromMask(const unsigned char* data, int width, int height, int stride);

	// 런 배열로부터 생성한다. colEnd 는 **끝 포함**이다.
	// 정렬/중복/인접은 내부에서 정규화한다.
	static Region FromRuns(const int* rows, const int* colBegins, const int* colEnds, int count);

	bool IsEmpty() const;

private:
	struct Impl;
	Impl* m_pImpl;

	friend struct RegionInternal;	// 내부 구현 전용 접근자 (외부 타입 노출 없음)
};

//------------------------------------------------------------------------------------
// 오퍼레이터 (HALCON 원명 대응)
//   빈 리전 / 0 나눗셈은 전부 0 을 반환한다 (docs/02_DEFINITIONS.md §0.4).
//------------------------------------------------------------------------------------

// HALCON: area_center(Regions : : : Area, Row, Column)
void AreaCenter (const Region& region, long& area, double& row, double& column);

// HALCON: contlength(Regions : : : ContLength)        Assertion: ContLength >= 0
void ContLength (const Region& region, double& contLength);

// HALCON: circularity(Regions : : : Circularity)      Assertion: 0 <= C <= 1.0
void Circularity(const Region& region, double& circularity);

// HALCON: compactness(Regions : : : Compactness)      Assertion: C >= 1.0 || C == 0
void Compactness(const Region& region, double& compactness);

// HALCON: convexity(Regions : : : Convexity)          Assertion: C <= 1
void Convexity  (const Region& region, double& convexity);

}}	// namespace glim::halcon

#endif	// GLIM_HALCON_H
