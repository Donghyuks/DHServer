#include "DummyClient.h"
#include "DHNetworkAPI.h"
#include "SharedDataStruct.h"

#pragma comment(lib, "DHLogger.lib")
#pragma comment(lib, "DHKeyIO.lib")

#include "DHLogger.h"
#include "DHKeyIO.h"
#include <chrono>

int DummyClient::GetCoreCount()
{
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION pProcessorInformations = NULL;
	DWORD length = 0;

	BOOL result = GetLogicalProcessorInformation(pProcessorInformations, &length);

	if (!result)
	{
		if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
			pProcessorInformations = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)new BYTE[length];
		}
	}

	result = GetLogicalProcessorInformation(pProcessorInformations, &length);

	if (!result)
	{
		// error
		return -1;
	}

	int numOfCores = 0;

	for (int i = 0; i < length / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION); i++)
	{
		if (pProcessorInformations[i].Relationship == RelationProcessorCore)
		{
			numOfCores++;
		}
	}

	delete[] pProcessorInformations;

	return numOfCores;
}

DummyClient::DummyClient()
{
	// 만들 쓰레드 개수는 적절하게.. 10개정도로
	//Make_Thread_Count = (GetCoreCount() - 1) * 2;	// 컴퓨터 코어 개수만큼 생성
	Make_Thread_Count = 10;

	Key_IO = new DHKeyIO;
	Logger = new DHLogger(_T("DummyClient"));

	// 테스트케이스 실행.
	KeyInputFunction();
}

DummyClient::~DummyClient()
{
	StopAllThread();
	delete Key_IO;
	Logger->Release();
}

void DummyClient::StartCase1()
{
	// 쓰레드를 사용할 준비가 되어있고, 이미 테스트케이스가 진행중이지 않으면
	if (End_Flag == true)
	{
		printf("[DummyClient] CASE_%d 번을 종료하는 중입니다. 잠시 후 다시 시도해주세요. \n", (int)Current_TestCase);
		return;
	}

	if (Thread_List.size() != 0)
	{
		printf("[DummyClient] 이미 CASE_%d 번이 실행중입니다. 종료 후 시도해주세요. \n 종료 키는 [Q] 입니다.\n", (int)Current_TestCase);
		return;
	}

	// 현재 테스트케이스는 1번이라고 저장.
	Current_TestCase = TestCaseNumber::Case1;

	for (int i = 0; i < Make_Thread_Count; i++)
	{
		// 메세지를 보내고 종료하는 과정을 무한히 반복.
		std::thread* _New_Thread = new std::thread(std::bind(&DummyClient::SendEndFunction, this));
		Thread_List.push_back(_New_Thread);
	}
}

void DummyClient::StartCase2()
{
	// 쓰레드를 사용할 준비가 되어있고, 이미 테스트케이스가 진행중이지 않으면
	if (End_Flag == true)
	{
		printf("[DummyClient] CASE_%d 번을 종료하는 중입니다. 잠시 후 다시 시도해주세요. \n", (int)Current_TestCase);
		return;
	}

	if (Thread_List.size() != 0)
	{
		printf("[DummyClient] 이미 CASE_%d 번이 실행중입니다. 종료 후 시도해주세요. \n 종료 키는 [Q] 입니다.\n", (int)Current_TestCase);
		return;
	}

	// 현재 테스트케이스는 2번이라고 저장.
	Current_TestCase = TestCaseNumber::Case2;

	for (int i = 0; i < Make_Thread_Count / 10; i++)
	{
		// 무한히 보내는 쓰레드는 총 쓰레드의 10분의 1만큼 생성.
		std::thread* _Send_Thread = new std::thread(std::bind(&DummyClient::BoundlessEndFunction, this));
		Thread_List.push_back(_Send_Thread);
	}

	for (int i = 0; i < (Make_Thread_Count - (Make_Thread_Count / 10)); i++)
	{
		// 무한히 접속후 종료만 하는 쓰레드 10분의 9만큼 생성.
		std::thread* _End_Thread = new std::thread(std::bind(&DummyClient::BoundlessSendFunction, this));
		Thread_List.push_back(_End_Thread);
	}
}

void DummyClient::StartCase3()
{
	// 쓰레드를 사용할 준비가 되어있고, 이미 테스트케이스가 진행중이지 않으면
	if (End_Flag == true)
	{
		printf("[DummyClient] CASE_%d 번을 종료하는 중입니다. 잠시 후 다시 시도해주세요. \n", (int)Current_TestCase);
		return;
	}

	if (Thread_List.size() != 0)
	{
		printf("[DummyClient] 이미 CASE_%d 번이 실행중입니다. 종료 후 시도해주세요. \n 종료 키는 [Q] 입니다.\n", (int)Current_TestCase);
		return;
	}

	// 현재 테스트케이스는 3번이라고 저장.
	Current_TestCase = TestCaseNumber::Case3;

	for (int i = 0; i < Make_Thread_Count; i++)
	{
		// 다양한 패킷을 보내는 쓰레드.
		std::thread* _Send_Various_Packet_Thread = new std::thread(std::bind(&DummyClient::SendVariousPacketFunction, this));
		Thread_List.push_back(_Send_Various_Packet_Thread);
	}
}

void DummyClient::BoundlessSendFunction()
{
	C2S_Packet* _msg = new C2S_Packet;
	std::vector<Network_Message*> Message_Vec;
	strcpy_s(_msg->Packet_Buffer, Test_Send_Msg.c_str());
	_msg->Packet_Type = C2S_Packet_Type::C2S_Packet_Type_Message;
	_msg->Packet_Size = Test_Send_Msg.size()+1;
	DHNetWorkAPI* my_NetWork = nullptr;
	std::chrono::time_point _Start_Time = std::chrono::system_clock::now();

	my_NetWork = new DHNetWorkAPI();
	my_NetWork->Initialize(DHNetWork_Name::DHNet);
	// Connect 까지 대기..
	while (!my_NetWork->Connect(CONNECT_PORT, CONNECT_IP)) {}

	while (!End_Flag)
	{
		/// Send 시간 측정.
		_Start_Time = std::chrono::system_clock::now();

		my_NetWork->Send(_msg);

		std::chrono::duration<double> _Proceed_Time = std::chrono::system_clock::now() - _Start_Time;
		double _Poceed_Time_Ms = _Proceed_Time.count() * 1000;

		/// 1회 실행했다고 횟수측정.
		Total_Count_1.fetch_add(1);
		/// 1회 실행시 걸린 시간을 기록해둔다.
		Total_Time_1.fetch_add(_Poceed_Time_Ms, std::memory_order_relaxed);

		/// 너무 많이보내면 결과확인이 어려워서.. sleep 100만큼 준다. 결과자체는 유의미함.
		Sleep(10);
	}

	/// 만약 종료플래그를 켰는데 connect작업중인 네트워크가 존재하면 끊어줌.
	if (my_NetWork != nullptr)
	{
		my_NetWork->End();
		my_NetWork = nullptr;
	}
}

void DummyClient::BoundlessEndFunction()
{
	C2S_Packet* _msg = new C2S_Packet;
	std::vector<Network_Message*> Message_Vec;
	strcpy_s(_msg->Packet_Buffer, std::string("TestClient").c_str());
	_msg->Packet_Type = C2S_Packet_Type::C2S_Packet_Type_Message;
	_msg->Packet_Size = sizeof(std::string("TestClient").c_str());
	DHNetWorkAPI* my_NetWork = nullptr;
	std::chrono::time_point _Start_Time = std::chrono::system_clock::now();

	while (!End_Flag)
	{
		if (my_NetWork == nullptr)
		{
			_Start_Time = std::chrono::system_clock::now();
			my_NetWork = new DHNetWorkAPI();
			my_NetWork->Initialize(DHNetWork_Name::DHNet);
		}
		// Connect 까지 대기..
		if (!my_NetWork->Connect(CONNECT_PORT, CONNECT_IP))
		{
			continue;
		}
		my_NetWork->End();

		std::chrono::duration<double> _Proceed_Time = std::chrono::system_clock::now() - _Start_Time;
		double _Poceed_Time_Ms = _Proceed_Time.count() * 1000;

		/// 1회 실행했다고 횟수측정.
		Total_Count_2.fetch_add(1);
		/// 1회 실행시 걸린 시간을 기록해둔다.
		Total_Time_2.fetch_add(_Poceed_Time_Ms, std::memory_order_relaxed);

		my_NetWork = nullptr;

		Sleep(0);
	}

	/// 만약 종료플래그를 켰는데 connect작업중인 네트워크가 존재하면 끊어줌.
	if (my_NetWork != nullptr)
	{
		my_NetWork->End();
		my_NetWork = nullptr;
	}
}

void DummyClient::SendEndFunction()
{
	C2S_Packet* _msg = new C2S_Packet;
	strcpy_s(_msg->Packet_Buffer, std::string("TestClient").c_str());
	_msg->Packet_Type = C2S_Packet_Type::C2S_Packet_Type_Message;
	_msg->Packet_Size = sizeof(std::string("TestClient").c_str());
	DHNetWorkAPI* my_NetWork = nullptr;
	std::chrono::time_point _Start_Time = std::chrono::system_clock::now();

	while (!End_Flag)
	{
		if (my_NetWork == nullptr)
		{
			_Start_Time = std::chrono::system_clock::now();
			my_NetWork = new DHNetWorkAPI();
			my_NetWork->Initialize(DHNetWork_Name::DHNet);
		}
		// Connect 까지 대기..
		bool _Test_Result = my_NetWork->Connect(CONNECT_PORT, CONNECT_IP);

		if (!my_NetWork->Connect(CONNECT_PORT, CONNECT_IP))
		{
			continue;
		}
		my_NetWork->Send(_msg);
		my_NetWork->End();

		std::chrono::duration<double> _Proceed_Time = std::chrono::system_clock::now() - _Start_Time;
		double _Poceed_Time_Ms = _Proceed_Time.count() * 1000;

		/// 1회 실행했다고 횟수측정.
		Total_Count.fetch_add(1);
		/// 1회 실행시 걸린 시간을 기록해둔다.
		Total_Time.fetch_add(_Poceed_Time_Ms, std::memory_order_relaxed);

		my_NetWork = nullptr;
		Sleep(0);
	}

	/// 만약 종료플래그를 켰는데 connect작업중인 네트워크가 존재하면 끊어줌.
	if (my_NetWork != nullptr)
	{
		my_NetWork->End();
		my_NetWork = nullptr;
	}
}

void DummyClient::SendVariousPacketFunction()
{
	thread_local std::mt19937 generator(std::random_device{}());

	std::uniform_int_distribution<int> distribution(0, 100000000);

	srand(distribution(generator));

	C2S_Packet* _msg = new C2S_Packet;
	std::vector<Network_Message*> Message_Vec;
	std::string Random_Range_String = "";
	_msg->Packet_Type = C2S_Packet_Type::C2S_Packet_Type_Message;

	DHNetWorkAPI* my_NetWork = nullptr;
	std::chrono::time_point _Start_Time = std::chrono::system_clock::now();

	my_NetWork = new DHNetWorkAPI();
	my_NetWork->Initialize(DHNetWork_Name::DHNet);
	// Connect 까지 대기..
	while (!my_NetWork->Connect(CONNECT_PORT, CONNECT_IP)) {}

	while (!End_Flag)
	{
		// Send 시 1~2200 사이의 랜덤한 길이의 메세지를 보냄. (Buffer의 사이즈인 2048을 초과하는 값을 보내도 20480까지 데이터는 받을 수 있다.)
		int Random_Number = rand() % 2200;
		Random_Range_String.resize(Random_Number,'A');

		strcpy_s(_msg->Packet_Buffer, Random_Range_String.c_str());
		_msg->Packet_Size = Random_Range_String.size() + 1;

		/// Send 시간 측정.
		_Start_Time = std::chrono::system_clock::now();

		my_NetWork->Send(_msg);

		std::chrono::duration<double> _Proceed_Time = std::chrono::system_clock::now() - _Start_Time;
		double _Poceed_Time_Ms = _Proceed_Time.count() * 1000;

		/// 1회 실행했다고 횟수측정.
		Total_Count_1.fetch_add(1);
		/// 1회 실행시 걸린 시간을 기록해둔다.
		Total_Time_1.fetch_add(_Poceed_Time_Ms, std::memory_order_relaxed);

		/// 너무 많이보내면 결과확인이 어려워서.. sleep 1000만큼 준다. 결과자체는 유의미함.
		Sleep(500);
	}

	/// 만약 종료플래그를 켰는데 connect작업중인 네트워크가 존재하면 끊어줌.
	if (my_NetWork != nullptr)
	{
		my_NetWork->End();
		my_NetWork = nullptr;
	}
}

void DummyClient::StopAllThread()
{
	// 다른 케이스의 실행을 위하여 중지시키는 로직.
	End_Flag = true;

	if (Thread_List.size() == 0)
	{
		// 현재 실행중인 테스트케이스가 존재하지 않음
		printf("[DummyClient] 테스트 케이스를 실행 하고있지 않습니다. \n");
	}
	else
	{
		for (int i = 0; i < Thread_List.size(); i++)
		{
			// 테스트 케이스에 사용되었던 쓰레드를 종료하고 삭제한다.
			Thread_List[i]->join();
			delete Thread_List[i];
		}

		// 모든 쓰레드가 종료되었으면 벡터를 비워준다.
		Thread_List.clear();

		// 현재 테스트 케이스에 따라 결과를 출력해준다.
		switch (Current_TestCase)
		{
		case DummyClient::Case1:
		{
			TCHAR Write_Result[MAX_WORD];
			// 케이스 1번에 대한 결과
			printf("[DummyClient] CASE_%d 번 실행결과. [Thread : %d 개]\n[실행시간] %f ms\t[실행횟수] %d 회\t[평균 응답시간] %f ms\n", 
				(int)Current_TestCase ,Make_Thread_Count, Total_Time.load(), Total_Count.load(), Total_Time / (double)Total_Count);

			_stprintf_s(Write_Result, _T("[DummyClient] CASE_%d 번 실행결과. [Thread : %d 개]\n[실행시간] %f ms\t[실행횟수] %d 회\t[평균 응답시간] %f ms\n"),
				(int)Current_TestCase, Make_Thread_Count, Total_Time.load(), Total_Count.load(), Total_Time / (double)Total_Count);

			Logger->WriteLog(LogType::COMPLETE_Log, Write_Result);
		}
			break;
		case DummyClient::Case2:
		{
			TCHAR Write_Result[MAX_WORD];
			// 케이스 2번에 대한 결과
			printf("[DummyClient] CASE_%d 번 실행결과. [Thread : %d 개]\n", (int)Current_TestCase, Make_Thread_Count);

			printf("[Send Thread : %d 개][실행시간] %f ms\t[실행횟수] %d 회\t[평균 응답시간] %f ms\n",
				Make_Thread_Count / 10, Total_Time_1.load(), Total_Count_1.load(), Total_Time_1 / (double)Total_Count_1);

			printf("[StartEnd Thread : %d 개][실행시간] %f ms\t[실행횟수] %d 회\t[평균 응답시간] %f ms\n",
				(Make_Thread_Count - (Make_Thread_Count / 10)), Total_Time_2.load(), Total_Count_2.load(), Total_Time_2 / (double)Total_Count_2);

			_stprintf_s(Write_Result, _T("[DummyClient] CASE_%d 번 실행결과. [Thread : %d 개]\n[SendRecv Thread : %d 개][실행시간] %f ms\t[실행횟수] %d 회\t[평균 응답시간] %f ms\n[StartEnd Thread : %d 개][실행시간] %f ms\t[실행횟수] %d 회\t[평균 응답시간] %f ms\n"),
				(int)Current_TestCase, Make_Thread_Count,
				Make_Thread_Count / 2, Total_Time_1.load(), Total_Count_1.load(), Total_Time_1.load() / (double)Total_Count_1,
				Make_Thread_Count / 2, Total_Time_2.load(), Total_Count_2.load(), Total_Time_2.load() / (double)Total_Count_2);

			Logger->WriteLog(LogType::COMPLETE_Log, Write_Result);
		}
			break;
		case DummyClient::Case3:
		{

		}
			break;
		default:
			break;
		}

		/// 변수 초기화.
		Total_Time = 0.f;
		Total_Count = 0;

		Total_Time_1 = 0.f;
		Total_Time_2 = 0.f;
		Total_Count_1 = 0;
		Total_Count_2 = 0;

		// 다른 테스트케이스의 사용준비가 되었다고 알려줌.
		printf("[DummyClient] CASE_%d 번이 종료되었습니다. 새로운 테스트케이스를 실행가능 합니다. \n", (int)Current_TestCase);
		Current_TestCase = TestCaseNumber::NonSet_TestCase;
	}

	// 다른 케이스의 실행을 위한 준비가 완료됨.
	End_Flag = false;
}

void DummyClient::KeyInputFunction()
{
	printf("[DummyClient] 더미 클라이언트 테스트를 시작합니다. \n");
	printf(" => CASE1[ 접속 -> 보내기 -> 종료 무한반복] \t\t\t 실행 키 : Key[1] \n");
	printf(" => CASE2[ 접속 -> 종료 // 접속 -> 보내기 무한반복] \t\t 실행 키 : Key[2] \n");
	printf(" => CASE3[ 접속 -> 보내기(다양한 패킷) 무한반복] \t\t 실행 키 : Key[3] \n");

	while (true)
	{
		/// 만약 Q키가 눌리면 현재 진행중인 테스트케이스를 종료한다.
		if(Key_IO->Key_Down('Q'))
		{
			StopAllThread();
		}

		/// 각각 테스트케이스 1,2,3 번 실행.
		if (Key_IO->Key_Down('1'))
		{
			StartCase1();
		}
		if (Key_IO->Key_Down('2'))
		{
			StartCase2();
		}
		if (Key_IO->Key_Down('3'))
		{
			StartCase3();
		}

		Sleep(0);
	}
}
