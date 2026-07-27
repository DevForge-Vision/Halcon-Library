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
	//
	// 전제조건: 0 < stride 이면 stride >= width 여야 한다.
	//   0 < stride < width 는 행 우선 버퍼로 성립할 수 없는 값이다(다음 행을 침범해 읽게 되고
	//   마지막 행에서는 버퍼 끝을 넘는다). 이 경우 읽기를 시도하지 않고 **빈 리전을 반환**한다.
	//   data == 0 / width <= 0 / height <= 0 과 같은 취급이다.
	// 필요한 버퍼 크기: (0 < stride ? stride : width) * height 바이트.
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

//------------------------------------------------------------------------------------
// 배치 계산 결과 — 위 오퍼레이터 5종이 내는 값 7개를 담는다.
//
//   필드의 의미·값 규약은 대응하는 개별 오퍼레이터와 **완전히 같다.**
//   (row, column) 순서는 HALCON 관례 (Row, Column) = (y, x) 를 따른다.
//
// ⚠ 호환성 주의 (ABI)
//   이 구조체는 크기와 레이아웃이 그대로 공개 경계다. 필드를 추가하면 sizeof 가 바뀌어
//   이전 헤더로 컴파일된 호출부와 링크 시 조용히 깨진다. 그러므로
//     - 필드는 **끝에만** 추가한다 (중간 삽입·순서 변경·타입 변경 금지)
//     - 필드를 추가하는 변경은 **라이브러리와 사용처를 함께 재컴파일**해야 한다
//   POD 로 유지한다 (생성자·가상함수·표준 컨테이너를 넣지 않는다). 그래야 툴셋(v140/v143)이
//   달라도 레이아웃이 흔들리지 않는다.
//------------------------------------------------------------------------------------
struct RegionFeatures
{
	long   area;			// AreaCenter  의 Area   (픽셀 개수)
	double row;				// AreaCenter  의 Row    (무게중심 y)
	double column;			// AreaCenter  의 Column (무게중심 x)
	double contLength;		// ContLength
	double circularity;		// Circularity
	double compactness;		// Compactness
	double convexity;		// Convexity
};

// 5종 값을 한 번에 계산한다. 개별 호출과 **결과가 완전히 동일**하되,
// 내부 중간산물(윤곽 추적 · convex hull)을 리전당 1회만 계산한다.
//   개별 5회 호출: 윤곽 추적 4회 + hull 1회
//   ComputeFeatures: 윤곽 추적 1회 + hull 1회
// 5종 중 2종 이상이 필요하면 이쪽을 쓰는 편이 항상 유리하다.
// 실패·빈 리전이면 전 필드가 0 이다 (개별 오퍼레이터와 같은 규약).
void ComputeFeatures(const Region& region, RegionFeatures& features);

}}	// namespace glim::halcon

#endif	// GLIM_HALCON_H
