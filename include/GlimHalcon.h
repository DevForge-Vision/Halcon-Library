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
// 2차 모멘트 계열 5종 (HALCON 원명 대응)
//
// 다섯 종이 **하나의 2차 중심 모멘트 코어**를 공유한다. 그러나 정규화 인자가 서로 다르다.
//   MomentsRegion2nd       : 정규화 **없음** (순수 합)
//   MomentsRegion2ndInvar  : **F^2** 로 나눔
//   EllipticAxis 내부      : **F** 로 나눔
// 이 셋을 섞으면 다섯 종이 통째로 틀린다 (docs/02_DEFINITIONS.md §6~§10).
//
// ⚠ 픽셀은 면적 1의 사각형이 아니라 **픽셀 중심의 무한소 점**으로 취급된다.
//    그래서 1픽셀 두께 리전(가로선/세로선)과 단일 픽셀은 Rb = 0 이 되고,
//    Anisometry 가 문서상 Assertion(>= 1.0)을 어기고 **0** 을 반환한다.
//    이것은 HALCON 원문이 명시한 동작이며 **일부러 그대로 재현한 것**이다.
//------------------------------------------------------------------------------------

// HALCON: moments_region_2nd(Regions : : : M11, M20, M02, Ia, Ib)
//   M11 = SUM (r0-r)(c0-c),  M20 = SUM (r0-r)^2,  M02 = SUM (c0-c)^2   (r0,c0 = 무게중심)
//   Ia, Ib = 공분산 행렬 [[M20,M11],[M11,M02]] 의 고유값 (Ia >= Ib >= 0)
//   ⚠ **면적으로 나누지 않는다.** 값이 도형 크기의 4제곱으로 커진다
//   ⚠ 원문 파라미터 설명은 Ia/Ib 를 "length of the axis" 라 부르지만 차원은 길이²다.
//      실제 "반지름" 은 EllipticAxis 의 Ra/Rb 이며 Ra = 2*sqrt(Ia/Area) 관계다
void MomentsRegion2nd(const Region& region,
					  double& m11, double& m20, double& m02, double& ia, double& ib);

// HALCON: moments_region_2nd_invar(Regions : : : M11, M20, M02)
//   위 값을 **F^2 (면적의 제곱)** 으로 나눈 것. 지수는 고정 2다.
//   스케일 불변: n x n 정사각형이면 M20 = M02 = (n^2-1)/(12*n^2) → 크기와 무관하게 약 1/12
//   빈 리전이면 0 나눗셈을 피해 전부 0.
void MomentsRegion2ndInvar(const Region& region,
						   double& m11, double& m20, double& m02);

// HALCON: elliptic_axis(Regions : : : Ra, Rb, Phi)
//   Assertion: Ra >= 0 / 0 <= Rb <= Ra / -pi/2 < Phi <= pi/2
//   Ra, Rb : 입력 리전과 같은 방향·같은 종횡비를 갖는 타원의 주/부 반지름
//   Phi    : 주축과 x축(= column 축) 사이의 각 [라디안]
//            ⚠ 부호가 통념과 반대다: Phi = -0.5*atan2(2*M11, M02 - M20)
//            주축 단위벡터는 (col, row) 기준 (cos Phi, -sin Phi) 다
//            (row 축이 아래로 향하므로 화면상 반시계 방향이 양(+)이 된다)
//   빈 리전·단일 픽셀 → Ra = Rb = 0. 점들이 한 직선 위에 있으면 → Rb = 0.
void EllipticAxis(const Region& region, double& ra, double& rb, double& phi);

// HALCON: eccentricity(Regions : : : Anisometry, Bulkiness, StructureFactor)
//   Anisometry      = Ra / Rb          (원이면 1.0.  ⚠ Rb == 0 이면 **0**)
//   Bulkiness       = pi * Ra * Rb / Area
//   StructureFactor = Anisometry * Bulkiness - 1
//   ⚠ Rb == 0 인 리전(단일 픽셀 / 1픽셀 두께 직선)에서는
//        Anisometry = 0, Bulkiness = 0, StructureFactor = **-1.0** 이 된다.
//      빈 리전은 셋 다 0.0 이다. **-1.0 과 0.0 을 구분해야 한다** (전자는 축퇴, 후자는 빈 리전).
void Eccentricity(const Region& region,
				  double& anisometry, double& bulkiness, double& structureFactor);

// HALCON: orientation_region(Regions : : : Phi)      Assertion: -pi <= Phi < pi
//   EllipticAxis 의 Phi 에서 출발해, 무게중심에서 가장 먼 **외곽 윤곽 픽셀**이
//   회전 좌표계의 음(-) 방향에 있으면 pi 를 더한다. 즉 축(180도 모호)이 아니라
//   **방향**을 돌려준다.
//   ⚠ 대칭 도형(정사각형·원·직선 등)은 최원점이 동률이라 Phi 또는 Phi±pi 중
//      어느 쪽이 나올지 원리적으로 정해지지 않는다. 두 값 모두 옳다.
void OrientationRegion(const Region& region, double& phi);

//------------------------------------------------------------------------------------
// 배치 계산 결과 — 1차 배치 오퍼레이터 5종이 내는 값 7개를 담는다.
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

//------------------------------------------------------------------------------------
// 2차 배치 계산 결과 — 2차 모멘트 계열 5종이 내는 값 15개를 담는다.
//
// ⚠ 왜 RegionFeatures 에 필드를 덧붙이지 않고 **새 구조체**를 만들었는가
//   RegionFeatures 는 이미 배포된 공개 경계다. 끝에 필드를 15개 붙이면 sizeof 가
//   커져서, **옛 헤더로 컴파일된 호출부가 링크는 되면서 스택을 넘겨 쓴다.**
//   헤더 주석이 "함께 재컴파일하면 된다" 고 허용하고는 있지만,
//   이 라이브러리는 검사기 여러 대에 소스로 배포되므로 재컴파일 누락이 실제로 일어난다.
//   구조체를 나누면 그 사고가 **원천적으로 불가능**해진다. 대신 중간산물 공유는
//   아래 ComputeFeaturesAndMoments 로 보장한다 — 나눠도 성능 손해가 없다.
//
// 필드 규약은 RegionFeatures 와 같다. POD 로 유지하고 **필드는 끝에만** 추가한다.
//------------------------------------------------------------------------------------
struct RegionMoments
{
	// MomentsRegion2nd — 정규화 없음 (순수 합)
	double m11;					// SUM (r0-r)(c0-c)
	double m20;					// SUM (r0-r)^2
	double m02;					// SUM (c0-c)^2
	double ia;					// 공분산 큰 고유값
	double ib;					// 공분산 작은 고유값

	// MomentsRegion2ndInvar — 위 세 값을 F^2 으로 나눈 것
	double m11Invar;
	double m20Invar;
	double m02Invar;

	// EllipticAxis
	double ra;
	double rb;
	double phi;					// -pi/2 < phi <= pi/2

	// Eccentricity
	double anisometry;			// Rb == 0 이면 0
	double bulkiness;
	double structureFactor;		// Rb == 0 이고 리전이 비지 않았으면 -1.0

	// OrientationRegion
	double orientation;			// -pi <= orientation < pi
};

// 2차 모멘트 계열 5종을 한 번에 계산한다. 개별 호출과 **결과가 완전히 동일**하되,
// 모멘트 누적·윤곽 추적을 리전당 1회만 한다.
// 실패·빈 리전이면 전 필드가 0 이다 (개별 오퍼레이터와 같은 규약).
void ComputeMoments(const Region& region, RegionMoments& moments);

// 1차 7값 + 2차 15값을 **컨텍스트 하나로** 계산한다.
//   ComputeFeatures 와 ComputeMoments 를 따로 부르면 윤곽 추적이 2회 일어난다.
//   이쪽은 1회다. 값은 어느 경로로 부르든 완전히 동일하다.
// 진입 즉시 두 출력 구조체의 전 필드가 0 으로 초기화된다.
void ComputeFeaturesAndMoments(const Region& region,
							   RegionFeatures& features,
							   RegionMoments& moments);

//------------------------------------------------------------------------------------
// 검사기 연동 편의 API — HALCON 원명 대응이 없다 (이 라이브러리가 추가한 것)
//
// 검사기는 이진화된 crop 한 장을 던지고 "그 안의 불량 하나"의 특징값을 원한다.
// 그런데 Region::FromMask 는 마스크 전체를 하나의 리전으로 만들어, 덩어리가 여럿이면
// 값이 섞인다(이 라이브러리에는 HALCON 의 connection 오퍼레이터가 없다). 그 간극을 메운다.
//------------------------------------------------------------------------------------

// 최대 연결성분(8-연결) 하나만 남긴 리전을 돌려준다.
//   "최대" 의 기준은 **면적(픽셀 개수)** 이다. 동률이면 스캔 순서(위→아래, 왼→오른쪽)상
//   먼저 나온 성분을 택한다.
//   빈 리전이거나 실패하면 빈 리전을 돌려준다.
//   성분이 이미 하나뿐이면 결과는 입력과 픽셀 단위로 동일하다(특징값이 변하지 않는다).
Region SelectLargestBlob(const Region& region);

// 이진화 마스크 한 장에서 최대 blob 을 골라 특징값 7개를 전부 계산한다.
//   data   : 0 이 아닌 값 = 전경 (검사기의 이진화 결과). 행 우선(row-major).
//   width  : 열 개수(column), height : 행 개수(row)
//   stride : 한 행의 바이트 수. 0 이하이면 width 로 간주한다.
//   features (out) : 진입 즉시 전 필드 0 으로 초기화된다.
//
// 반환 true  = 전경 blob 을 찾아 최대 성분의 값을 계산했다
//      false = 입력이 잘못됐거나(널 포인터 / width·height <= 0 / 0 < stride < width)
//              전경이 하나도 없다. 이때 features 는 전 필드 0.
//
// ⚠ 왜 이 함수만 bool 을 반환하는가 (기존 오퍼레이터 5종·ComputeFeatures 는 void 다)
//   기존 함수들은 Region 을 받는다. Region 은 이미 검증을 통과해 만들어진 값이므로
//   "실패" 라는 개념이 사실상 빈 리전 하나뿐이고, 그래서 "실패도 0" 규약으로 충분했다.
//   그러나 이 함수는 **원시 포인터와 치수를 직접 받는다.** 널 포인터·잘못된 stride 처럼
//   입력 자체가 틀릴 수 있고, 그것은 "면적이 0 인 정상 리전(전경 없음)" 과 의미가 다르다.
//   검사기가 이 둘을 구분하지 못하면 조용히 틀린 판정을 내리게 되므로, 여기서만 bool 로
//   성패를 명시한다. 값 규약(전 필드 0) 자체는 기존과 동일하게 유지한다.
//
// 내부는 FromMask → SelectLargestBlob → ComputeFeatures 조합이다(계산식 재사용).
bool ComputeLargestBlobFeatures(const unsigned char* data,
								int width, int height, int stride,
								RegionFeatures& features);

}}	// namespace glim::halcon

#endif	// GLIM_HALCON_H
