#pragma once

/*
2021.06.25 CDH

	구현하고싶은 디버거 스펙
	<F1~F3 키>
		1. F1~F3 키를 눌러 테스트 케이스 중 1~3번에 대한 테스트 시작.
		2. F1~F3 키를 한번 더 누르면 테스트 종료.
		3. 측정 종료 후 각각의 데이터를 출력.
	<F4 키>
		1. F4 키를 눌러 로그를 남기기 시작.
		2. F4 키를 한번 더 누르면 로그 남기기 종료.
		3. 종료 후 로그를 남기는 쓰레드도 같이 종료.
	<F5 키>
		1. F5 키를 눌러 테스트 케이스 1~N 개의 측정 시작.
		2. 5초 / 15초 / 30초 간격으로 각 케이스에 대한 측정값 저장.
		3. 측정 종료.
		4. 측정 종료 후 각각의 데이터를 출력.

2021.06.29 CDH

	디버거 사용법.
		1.	Start_Mark_Timer 함수가 해당 테스트 케이스에 대하여 호출이 되어 있어야 한다.
		2.	End_Mark_Timer 함수가 해당 케이스에 대하여 호출이 되어 있어야 한다.
		3.	<F1~F5>를 눌러서 원하는 케이스에 대한 측정을 시작 / 종료 한다.

2021.11.02 CDH

	DH디버거 DLL 버전으로 제작.

2021.11.03 CDH
	
	개선사항
		1.	기존의 Key 값을 통한 입력방법은 효율이 좋은가? => 실용성이 떨어진다 판단..
			사용자가 원하면 작동하는 모드로 변경! (Default는 키보드를 통한 입력으로 측정 아님)
		2.	Default 상태로 특정 시간을 지정하여 작동하는 디버거로 셋팅.
*/

/// 현재의 상태를 나타내는 정의.
#define MEASURABLE		0b100	// 측정을 할 것인가에 대한 여부.
#define	MEASURE_START	0b010	// 측정을 시작하는 부분이 존재하는지?
#define MEASURE_END		0b001	// 측정을 종료하는 부분이 존재하는지?
#define MEASURABLE_STAT	0b111	// 모든 요건이 충족된 경우. (측정 가능 상태)'
#define MEASURE_DEFAULT	0b000	// 측정 초기화. (모든 조건을 0으로 셋팅)

#ifdef DHDEBBUGER_EXPORTS
#define DHDEBUG_DLL __declspec(dllexport)
#else
#define DHDEBUG_DLL __declspec(dllimport)
#endif

/// 시간 측정을위한 C++11 스펙의 chrono를 써보자.
#include <tchar.h>
#include <chrono>

namespace std
{
	class thread;
}

class DHLogger;
class DHKeyIO;

/// 디버그 매니저.
class DHDebugger
{
public:

	enum class DHDEBUG_DLL DHDebugger_Mode
	{
		/// F1 ~ F5 키를 통해 작동하는 모드.
		KeyBoard_Mode,
		/// 특정 시간을 지정하여 시간마다 기록하는 모드.
		Timer_Mode
	};

	enum DHDEBUG_DLL Test_Case_Number
	{
		CASE_01, CASE_02,
		CASE_03, CASE_04,
		CASE_05, CASE_06,
		CASE_07, CASE_08,
		CASE_09, CASE_10,
		MAX_CASE_COUNT			// 총 테스트 케이스 개수.
	};

private:
	DHLogger* m_LogManager = nullptr;
	DHKeyIO* m_KeyIO = nullptr;

	/// 측정한 횟수와 시간의 누계를 저장해둠. *map을 사용하면 아무래도 느리지.. => 새로운 구조체 만들기.
	struct Test_Data
	{
		unsigned int	Measurable = MEASURE_DEFAULT;	// 측정 가능한 상태인지를 나타냄.
		unsigned int	Test_Count = 0;					// 테스트 측정 횟수.
		std::chrono::system_clock::time_point Start_Time;
		std::chrono::duration<double>Total_Time = std::chrono::milliseconds::zero();	// 모든 테스트 측정시간의 합.
	};

	/// Timer 모드시 사용할 쓰레드 시간 측정기.
	// 측정 시작 시간
	std::chrono::system_clock::time_point Check_Start_Time;
	// 현재 시간
	std::chrono::system_clock::time_point Current_Time;
	// 지나간 시간
	std::chrono::duration<double>Pass_Time = std::chrono::milliseconds::zero();

	/// 현재 모드의 종류
	DHDebugger_Mode Current_Mode;
	/// 현재 셋팅되어있는 타이머 시간(밀리)초.
	float Current_Set_TimerSec = 5000.f;
	/// 테스트 케이스는 최대 10개까지 설정 가능.
	Test_Data Test_Case[MAX_CASE_COUNT];
	/// 종료를 알리는 플래그.
	bool Is_End = false;
	/// 전체 측정시작 및 종료
	bool Is_Total_Measure = false;
	/// 키 입력을 받을 쓰레드.
	std::thread* Use_Mode_Thread = nullptr;
	/// Log 를 Text로 남길지 여부.
	bool Use_Logger = true;

private:
	// 해당 테스트 케이스 초기화.
	void Clear_Test_Case(Test_Case_Number _Test_Case_Num);
	// 해당 테스트 케이스에 대해 측정 중인 경우와 측정 중이지 않은 경우에 대한 처리.
	void Measurable_Test_Case_Print_KeyBoard(Test_Case_Number _Test_Case_Num);
	// 타이머 모드시 동작하는 Print
	void Measurable_Test_Case_Print_Timer(Test_Case_Number _Test_Case_Num);
	//void Measurable_Test_Case_Print_KeyBoard(Test_Case_Number _Test_Case_Num);
	// 키 입력에 대한 쓰레드.
	void Key_Input_Thread();
	// 타이머에 대한 쓰레드.
	void Timer_Thread();
	// 키보드 모드일 때 초기화.
	void Initialize_KeyBoard_Mode();
	// 타이머 모드일 때 초기화.
	void Initialize_Timer_Mode();

public:
	/// <summary>
	/// 디버거 생성
	/// </summary>
	/// <param name="_Debugger_Mode"> 디버깅 모드는 키보드를통한 모드 / 시간지정을 통한 모드가 있다. </param>
	/// <param name="_Use_Logger"> Text파일로 기록을 할지 여부 </param>
	/// <param name="_Title_Name"> Text파일로 기록한다면 해당 파일의 Title 이름 지정. </param>
	/// <returns></returns>
	DHDEBUG_DLL  DHDebugger(DHDebugger_Mode _Debugger_Mode, bool _Use_Logger = true, const TCHAR* _Title_Name = _T("DHDEBUG"));
	DHDEBUG_DLL  ~DHDebugger();

	/// 기본적으로 시작점과 끝점을 남겨야 시간의 측정을 시작할 수 있다. (F1~3, F5 기능 이용시)
	// 현재 위치에 해당 Test_Case에 대한 시간측정 시작점 남긴다. 
	DHDEBUG_DLL void Start_Mark_Timer(Test_Case_Number _Test_Case_Num);
	// Test_Case 의 시간측정 종료지점.
	DHDEBUG_DLL void End_Mark_Timer(Test_Case_Number _Test_Case_Num);
	// 파일 쓰기 사용자 지정 타입을 붙여서 로그파일 기록. ex) [매우매우중요] LOG...
	DHDEBUG_DLL void WriteLog(const TCHAR* _Log_Tag, const TCHAR* _Write_Data, ...);
	// 만약 타이머 모드를 통한 디버거를 생성했을때 타이머를 몇초로 지정할지? (설정안하면 기본 5초)
	DHDEBUG_DLL void SetTimerSec(float _Timer_Sec);

};

