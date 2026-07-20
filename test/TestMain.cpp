//====================================================================================
// test/TestMain.cpp — 기준 도형 S1~S7 기댓값 대조 테스트
//
// 근거: docs/03_VERIFICATION.md
//   §1 허용 오차 / §2 기준 도형 / §3 손계산 기댓값 / §5 결정적 판정 3건
//
// 종료 코드
//   0 : 모든 HARD 검사 통과
//   1 : 하나라도 실패
//
// NOTE 로 분류된 항목은 "정의서 §2 수식대로 계산하면 문서의 손계산 기댓값과
// 어긋나는 것이 정상"인 지점이다. 실패로 세지 않되 반드시 출력한다.
//====================================================================================
#include "GlimHalcon.h"
#include "Shapes.h"

#include <cstdio>
#include <cmath>
#include <string>
#include <vector>

using namespace glim::halcon;

namespace {

const double kPi = 3.14159265358979323846;

int g_passCount = 0;
int g_failCount = 0;
std::vector<std::string> g_failMessages;
std::vector<std::string> g_noteMessages;

//------------------------------------------------------------------------------------
// 검사 헬퍼
//------------------------------------------------------------------------------------
void Report(bool ok, const char* item, double got, double expect, const char* how)
{
	std::printf("  [%s] %-34s got=%18.9f  expect=%18.9f  (%s)\n",
				ok ? "PASS" : "FAIL", item, got, expect, how);

	if (ok)
	{
		++g_passCount;
	}
	else
	{
		++g_failCount;

		char buffer[512];
		std::sprintf(buffer, "%s : got=%.9f expect=%.9f (%s)", item, got, expect, how);
		g_failMessages.push_back(std::string(buffer));
	}
}

// 완전 일치 (정수 / 이산값) — 부동소수 표현 오차만 허용
void CheckExact(const char* item, double got, double expect)
{
	Report(std::fabs(got - expect) <= 1e-9, item, got, expect, "exact / 1e-9");
}

// 절대 오차
void CheckAbs(const char* item, double got, double expect, double tolerance)
{
	char how[64];
	std::sprintf(how, "abs <= %g", tolerance);
	Report(std::fabs(got - expect) <= tolerance, item, got, expect, how);
}

// 상대 오차 (곡선 도형: ±2%)
void CheckRelative(const char* item, double got, double expect, double relative)
{
	const double base = (std::fabs(expect) > 0.0) ? std::fabs(expect) : 1.0;
	char how[64];
	std::sprintf(how, "rel <= %.1f%%", relative * 100.0);
	Report(std::fabs(got - expect) <= base * relative, item, got, expect, how);
}

// 조건 검사 (판별 시험용)
void CheckCondition(bool ok, const char* item, double got, const char* how)
{
	std::printf("  [%s] %-34s got=%18.9f  %s\n", ok ? "PASS" : "FAIL", item, got, how);

	if (ok)
	{
		++g_passCount;
	}
	else
	{
		++g_failCount;

		char buffer[512];
		std::sprintf(buffer, "%s : got=%.9f (%s)", item, got, how);
		g_failMessages.push_back(std::string(buffer));
	}
}

// 정의서 기댓값과 어긋나는 지점 기록 (실패로 세지 않는다)
void Note(const char* item, double got, double docExpect, const char* reason)
{
	std::printf("  [NOTE] %-34s got=%18.9f  doc=%18.9f  %s\n", item, got, docExpect, reason);

	char buffer[512];
	std::sprintf(buffer, "%s : got=%.6f, docs=%.6f — %s", item, got, docExpect, reason);
	g_noteMessages.push_back(std::string(buffer));
}

//------------------------------------------------------------------------------------
// 한 도형의 전 특징값
//------------------------------------------------------------------------------------
struct Measured
{
	const char* name;
	long area;
	double row;
	double column;
	double contLength;
	double circularity;
	double compactness;
	double convexity;
};

Measured Measure(const char* name, const Region& region)
{
	Measured m;
	m.name = name;
	m.area = 0;
	m.row = 0.0;
	m.column = 0.0;
	m.contLength = 0.0;
	m.circularity = 0.0;
	m.compactness = 0.0;
	m.convexity = 0.0;

	AreaCenter(region, m.area, m.row, m.column);
	ContLength(region, m.contLength);
	Circularity(region, m.circularity);
	Compactness(region, m.compactness);
	Convexity(region, m.convexity);
	return m;
}

//------------------------------------------------------------------------------------
// HALCON Assertion 재현 확인 (함정 #1 클리핑 방향, 함정 #2 분자·분모 방향)
//------------------------------------------------------------------------------------
void CheckAssertions(const Measured& m)
{
	char item[128];

	std::sprintf(item, "%s assert ContLength>=0", m.name);
	CheckCondition(m.contLength >= 0.0, item, m.contLength, "ContLength >= 0");

	std::sprintf(item, "%s assert 0<=Circ<=1", m.name);
	CheckCondition(m.circularity >= 0.0 && m.circularity <= 1.0, item, m.circularity, "0 <= C <= 1");

	std::sprintf(item, "%s assert Comp>=1||==0", m.name);
	CheckCondition(m.compactness == 0.0 || m.compactness >= 1.0, item, m.compactness, "C >= 1 or C == 0");

	std::sprintf(item, "%s assert Convex<=1", m.name);
	CheckCondition(m.convexity <= 1.0, item, m.convexity, "C <= 1 (CV-1: F_c 는 픽셀 개수)");
}

}	// anonymous namespace

//====================================================================================
int main()
{
	const double kSqrt2 = std::sqrt(2.0);

	Region s1 = shapes::MakeS1Square();
	Region s2 = shapes::MakeS2Disk();
	Region s3 = shapes::MakeS3Ring();
	Region s4 = shapes::MakeS4Line();
	Region s5 = shapes::MakeS5LShape();
	Region s6 = shapes::MakeS6SinglePixel();
	Region s7 = shapes::MakeS7Empty();

	Measured m1 = Measure("S1", s1);
	Measured m2 = Measure("S2", s2);
	Measured m3 = Measure("S3", s3);
	Measured m4 = Measure("S4", s4);
	Measured m5 = Measure("S5", s5);
	Measured m6 = Measure("S6", s6);
	Measured m7 = Measure("S7", s7);

	const Measured all[7] = { m1, m2, m3, m4, m5, m6, m7 };

	//--------------------------------------------------------------------------------
	std::printf("====================================================================================\n");
	std::printf(" GlimHalcon Phase 1 - 기준 도형 S1~S7 측정값\n");
	std::printf(" (docs/03_VERIFICATION.md)\n");
	std::printf("====================================================================================\n");
	std::printf("%-4s %8s %12s %12s %14s %12s %12s %12s\n",
				"ID", "Area", "Row", "Column", "ContLength", "Circular", "Compact", "Convex");
	std::printf("------------------------------------------------------------------------------------------------\n");
	for (int i = 0; i < 7; ++i)
	{
		std::printf("%-4s %8ld %12.6f %12.6f %14.6f %12.7f %12.7f %12.7f\n",
					all[i].name, all[i].area, all[i].row, all[i].column,
					all[i].contLength, all[i].circularity, all[i].compactness, all[i].convexity);
	}
	std::printf("\n");

	//--------------------------------------------------------------------------------
	// T-1. area_center   (docs/03_VERIFICATION.md §3 T-1)
	//--------------------------------------------------------------------------------
	std::printf("[T-1] area_center -----------------------------------------------------------------\n");
	CheckExact("S1 Area",        static_cast<double>(m1.area), 10000.0);
	CheckExact("S1 Row",         m1.row,    49.5);
	CheckExact("S1 Column",      m1.column, 49.5);

	CheckExact("S4 Area",        static_cast<double>(m4.area), 100.0);
	CheckExact("S4 Row",         m4.row,    50.0);
	CheckExact("S4 Column",      m4.column, 49.5);

	CheckExact("S5 Area",        static_cast<double>(m5.area), 7500.0);
	CheckAbs  ("S5 Row",         m5.row,    (5000.0 * 49.5 + 2500.0 * 74.5) / 7500.0, 1e-9);
	CheckAbs  ("S5 Column",      m5.column, (5000.0 * 24.5 + 2500.0 * 74.5) / 7500.0, 1e-9);

	CheckExact("S6 Area",        static_cast<double>(m6.area), 1.0);
	CheckExact("S6 Row",         m6.row,    50.0);
	CheckExact("S6 Column",      m6.column, 50.0);

	CheckExact("S7 Area",        static_cast<double>(m7.area), 0.0);
	CheckExact("S7 Row",         m7.row,    0.0);
	CheckExact("S7 Column",      m7.column, 0.0);

	// 대칭 도형의 무게중심은 정확히 중심이어야 한다.
	CheckExact("S2 Row(대칭)",   m2.row,    50.0);
	CheckExact("S2 Column(대칭)",m2.column, 50.0);
	CheckExact("S3 Row(대칭)",   m3.row,    50.0);
	CheckExact("S3 Column(대칭)",m3.column, 50.0);
	std::printf("\n");

	//--------------------------------------------------------------------------------
	// T-2. contlength   (§3 T-2, 결정적 판정 P-1 / P-3)
	//--------------------------------------------------------------------------------
	std::printf("[T-2] contlength ------------------------------------------------------------------\n");
	// P-1: 픽셀 중심 체인 → S1 = 396 (경계둘레 모델이면 400)
	CheckExact("S1 ContLength (P-1)", m1.contLength, 396.0);
	CheckExact("S4 ContLength (P-1)", m4.contLength, 198.0);
	// P-1 의 가장 명확한 판별: 단일 픽셀은 스텝이 없으므로 0 (경계둘레 모델이면 4)
	CheckExact("S6 ContLength (P-1)", m6.contLength, 0.0);
	CheckExact("S7 ContLength",       m7.contLength, 0.0);

	// S5 L자: 축평행 394 스텝 + 오목 코너의 대각 1스텝
	//   상단 49 + 좌팔 우변 49 + (대각 1) + 하팔 상변 49 + 우변 49 + 하변 99 + 좌변 99
	CheckExact("S5 ContLength", m5.contLength, 394.0 + kSqrt2);

	// P-3: 구멍 윤곽 제외. 포함해 버리면 링에서 약 471 이 된다.
	CheckCondition(m3.contLength < 400.0, "S3 ContLength 구멍제외 (P-3)", m3.contLength,
				   "< 400 이어야 함 (구멍 포함시 약 471)");
	std::printf("\n");

	//--------------------------------------------------------------------------------
	// T-3. circularity   (§3 T-3)
	//--------------------------------------------------------------------------------
	std::printf("[T-3] circularity -----------------------------------------------------------------\n");
	// S1: max = sqrt(49.5^2 + 49.5^2) → max^2 = 4900.5
	CheckAbs("S1 Circularity", m1.circularity, 10000.0 / (4900.5 * kPi), 1e-9);
	// S4: max = 49.5
	CheckAbs("S4 Circularity", m4.circularity, 100.0 / (49.5 * 49.5 * kPi), 1e-9);
	CheckExact("S6 Circularity", m6.circularity, 0.0);		// max == 0 → 0 나눗셈 방어
	CheckExact("S7 Circularity", m7.circularity, 0.0);
	// 곡선 도형: ±2%
	CheckRelative("S2 Circularity", m2.circularity, 1.0,  0.02);
	CheckRelative("S3 Circularity", m3.circularity, 0.75, 0.02);
	std::printf("\n");

	//--------------------------------------------------------------------------------
	// T-4. compactness   (§3 T-4) — contlength 에 연쇄 의존
	//--------------------------------------------------------------------------------
	std::printf("[T-4] compactness -----------------------------------------------------------------\n");
	CheckAbs("S1 Compactness", m1.compactness, (396.0 * 396.0) / (4.0 * 10000.0 * kPi), 1e-9);
	CheckAbs("S4 Compactness", m4.compactness, (198.0 * 198.0) / (4.0 * 100.0 * kPi), 1e-9);
	// 단일 픽셀: L=0, F=1 → C'=0 → max(1,0) = 1.0  (정의서 §4.3 수식 그대로)
	CheckExact("S6 Compactness", m6.compactness, 1.0);
	CheckExact("S7 Compactness", m7.compactness, 0.0);
	std::printf("\n");

	//--------------------------------------------------------------------------------
	// T-5. convexity   (§3 T-5, 결정적 판정 P-2)
	//--------------------------------------------------------------------------------
	std::printf("[T-5] convexity -------------------------------------------------------------------\n");
	// P-2: F_c = hull 래스터화 픽셀 개수 → 정사각형에서 정확히 1.0
	//      폴리곤 기하 면적(99x99=9801)을 쓰면 1.0203 이 되어 여기서 즉시 실패한다.
	CheckExact("S1 Convexity (P-2)", m1.convexity, 1.0);
	CheckExact("S4 Convexity",       m4.convexity, 1.0);		// 축퇴 hull = 자기 자신
	CheckExact("S6 Convexity",       m6.convexity, 1.0);
	CheckExact("S7 Convexity",       m7.convexity, 0.0);
	CheckRelative("S2 Convexity", m2.convexity, 1.0,   0.02);
	CheckRelative("S3 Convexity", m3.convexity, 0.75,  0.02);
	CheckRelative("S5 Convexity", m5.convexity, 0.872, 0.02);	// 래스터화 해석 기준
	std::printf("\n");

	//--------------------------------------------------------------------------------
	// HALCON Assertion 재현
	//--------------------------------------------------------------------------------
	std::printf("[T-6] HALCON Assertion ------------------------------------------------------------\n");
	for (int i = 0; i < 7; ++i)
		CheckAssertions(all[i]);
	std::printf("\n");

	//--------------------------------------------------------------------------------
	// 정의서 손계산 기댓값과 어긋나는 지점 (실패 아님 / 문서 갱신 필요)
	//--------------------------------------------------------------------------------
	std::printf("[NOTE] 정의서 기댓값과의 차이 -----------------------------------------------------\n");
	Note("S3 ContLength", m3.contLength, 314.159265,
		 "문서는 연속 원주 2*pi*R. 체인코드(축1/대각sqrt2)는 디지털 원에서 약 +5.5% 편향이 원리적으로 존재");
	Note("S2 ContLength", m2.contLength, 314.159265,
		 "동일 원인 (문서 표에는 없음, 참고용)");
	Note("S2 Compactness", m2.compactness, 1.0,
		 "L 편향이 제곱으로 전파. L=2*pi*R 이면 1.0016 이 된다");
	Note("S2 Circularity", m2.circularity, 1.0,
		 "문서는 min 클리핑이 걸린다고 봤으나 디지털 원판 면적은 7845 라 클리핑 미발동 (허용오차 내)");
	std::printf("\n");

	//--------------------------------------------------------------------------------
	std::printf("====================================================================================\n");
	std::printf(" 결과 : PASS %d / FAIL %d   (NOTE %d 건)\n",
				g_passCount, g_failCount, static_cast<int>(g_noteMessages.size()));
	std::printf("====================================================================================\n");

	if (!g_failMessages.empty())
	{
		std::printf("\n실패 목록\n");
		for (size_t i = 0; i < g_failMessages.size(); ++i)
			std::printf("  - %s\n", g_failMessages[i].c_str());
	}

	if (!g_noteMessages.empty())
	{
		std::printf("\n문서 갱신 필요 목록 (docs/03_VERIFICATION.md)\n");
		for (size_t i = 0; i < g_noteMessages.size(); ++i)
			std::printf("  - %s\n", g_noteMessages[i].c_str());
	}

	return (g_failCount == 0) ? 0 : 1;
}
