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
	/// ���� �������� �׽�Ʈ���̽� ��ȣ.
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

	/// C++ 20 ���� �߰��� atomic double
	std::atomic<double> Total_Time = 0.f;
	// ������ ������ Ƚ��
	std::atomic_int32_t Total_Count = 0;

	// Case���� �׽�Ʈ��찡 �������� ��츦 ����Ͽ� �� ������.
	std::atomic<double> Total_Time_1 = 0.f;
	std::atomic<double> Total_Time_2 = 0.f;
	std::atomic_int32_t Total_Count_1 = 0;
	std::atomic_int32_t Total_Count_2 = 0;

private:
	int GetCoreCount();
	// ������ ������ �ޱ⸸ �ϴ� Function
	void BoundlessSendFunction();
	// ������ �����ϰ� �����ϴ� Function
	void BoundlessEndFunction();
	// ���� �� �޼����� �ϳ� ������ �����ϴ� Function
	void SendEndFunction();
	// �پ��� ũ���� ��Ŷ�� ������ Function
	void SendVariousPacketFunction();
	// ��� �����带 ����
	void StopAllThread();
	// Ű���带 ���� �׽�Ʈ ���̽� ���.
	void KeyInputFunction();

	// �׽�Ʈ ���̽� ���.
	void StartCase1();
	void StartCase2();
	/// ���� ����..
	//void StartCase3();
public:
	DummyClient();
	~DummyClient();
};

