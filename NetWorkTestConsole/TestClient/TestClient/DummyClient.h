#pragma once

#define CONNECT_IP "221.163.91.100"
// #define CONNECT_IP "192.168.0.56"

#include <thread>
#include <vector>
#include <atomic>

class DHKeyIO;
class DHLogger;

class DummyClient
{
private:
	/// 현재 진행중인 테스트케이스 번호.
	enum TestCaseNumber
	{
		NonSet_TestCase,
		Case1,
		Case2,
		Case3,
	};

private:
	DHKeyIO* Key_IO = nullptr;
	DHLogger* Logger = nullptr;

	TestCaseNumber Current_TestCase = TestCaseNumber::NonSet_TestCase;
	int Make_Thread_Count = 0;
	bool End_Flag = false;
	std::vector<std::thread*> Thread_List;

	/// C++ 20 에서 추가된 atomic double
	std::atomic<double> Total_Time = 0.f;
	// 로직을 실행한 횟수
	std::atomic_int32_t Total_Count = 0;

	// Case내에 테스트경우가 여러개일 경우를 대비하여 더 만들어둠.
	std::atomic<double> Total_Time_1 = 0.f;
	std::atomic<double> Total_Time_2 = 0.f;
	std::atomic_int32_t Total_Count_1 = 0;
	std::atomic_int32_t Total_Count_2 = 0;

private:
	int GetCoreCount();
	// 무한히 보내고 받기만 하는 Function
	void BoundlessSendFunction();
	// 무한히 접속하고 종료하는 Function
	void BoundlessEndFunction();
	// 접속 후 메세지를 하나 보내고 종료하는 Function
	void SendEndFunction();
	// 다양한 크기의 패킷을 보내는 Function
	void SendVariousPacketFunction();
	// 모든 쓰레드를 종료
	void StopAllThread();
	// 키보드를 통한 테스트 케이스 재생.
	void KeyInputFunction();

	// 테스트 케이스 재생.
	void StartCase1();
	void StartCase2();
	/// 추후 구현..
	//void StartCase3();
public:
	DummyClient();
	~DummyClient();
};

