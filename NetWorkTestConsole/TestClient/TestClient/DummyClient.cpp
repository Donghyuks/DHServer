#include "DummyClient.h"
#include "C2NetworkAPI.h"
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
	// ���� ������ ������ ��ǻ�� �ھ� ���� -1 *2 �� ��ŭ �����Ѵ�.
	// Logger �� ���� ������ �ϳ��� �Ҵ��ϱ� ���ؼ� �Ѱ��� �� �Ҵ���.
	//Make_Thread_Count = (GetCoreCount() - 1) * 2;
	Make_Thread_Count = 1;

	Key_IO = new DHKeyIO;
	Logger = new DHLogger(_T("DummyClient"));

	// �׽�Ʈ���̽� ����.
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
	// �����带 ����� �غ� �Ǿ��ְ�, �̹� �׽�Ʈ���̽��� ���������� ������
	if (End_Flag == true)
	{
		printf("[DummyClient] CASE_%d ���� �����ϴ� ���Դϴ�. ��� �� �ٽ� �õ����ּ���. \n", (int)Current_TestCase);
		return;
	}

	if (Thread_List.size() != 0)
	{
		printf("[DummyClient] �̹� CASE_%d ���� �������Դϴ�. ���� �� �õ����ּ���. \n ���� Ű�� [Q] �Դϴ�.\n", (int)Current_TestCase);
		return;
	}

	// ���� �׽�Ʈ���̽��� 1���̶�� ����.
	Current_TestCase = TestCaseNumber::Case1;

	for (int i = 0; i < Make_Thread_Count; i++)
	{
		// �޼����� ������ �����ϴ� ������ ������ �ݺ�.
		std::thread* _New_Thread = new std::thread(std::bind(&DummyClient::SendEndFunction, this));
		Thread_List.push_back(_New_Thread);
	}
}

void DummyClient::StartCase2()
{
	// �����带 ����� �غ� �Ǿ��ְ�, �̹� �׽�Ʈ���̽��� ���������� ������
	if (End_Flag == true)
	{
		printf("[DummyClient] CASE_%d ���� �����ϴ� ���Դϴ�. ��� �� �ٽ� �õ����ּ���. \n", (int)Current_TestCase);
		return;
	}

	if (Thread_List.size() != 0)
	{
		printf("[DummyClient] �̹� CASE_%d ���� �������Դϴ�. ���� �� �õ����ּ���. \n ���� Ű�� [Q] �Դϴ�.\n", (int)Current_TestCase);
		return;
	}

	// ���� �׽�Ʈ���̽��� 2���̶�� ����.
	Current_TestCase = TestCaseNumber::Case2;

	for (int i = 0; i < Make_Thread_Count / 10; i++)
	{
		// ������ ������ ������� �� �������� 10���� 1��ŭ ����.
		std::thread* _Send_Thread = new std::thread(std::bind(&DummyClient::BoundlessSendFunction, this));
		Thread_List.push_back(_Send_Thread);
	}

	for (int i = 0; i < (Make_Thread_Count - (Make_Thread_Count / 10)); i++)
	{
		// ������ ������ ���Ḹ �ϴ� ������ 10���� 9��ŭ ����.
		std::thread* _End_Thread = new std::thread(std::bind(&DummyClient::BoundlessEndFunction, this));
		Thread_List.push_back(_End_Thread);
	}
}
//
//void DummyClient::StartCase3()
//{
//	// �����带 ����� �غ� �Ǿ��ְ�, �̹� �׽�Ʈ���̽��� ���������� ������
//	if (End_Flag == true)
//	{
//		printf("[DummyClient] CASE_%d ���� �����ϴ� ���Դϴ�. ��� �� �ٽ� �õ����ּ���. \n", (int)Current_TestCase);
//		return;
//	}
//
//	if (Thread_List.size() != 0)
//	{
//		printf("[DummyClient] �̹� CASE_%d ���� �������Դϴ�. ���� �� �õ����ּ���. \n ���� Ű�� [Q] �Դϴ�.\n", (int)Current_TestCase);
//		return;
//	}
//
//	// ���� �׽�Ʈ���̽��� 2���̶�� ����.
//	Current_TestCase = TestCaseNumber::Case3;
//
//	for (int i = 0; i < Make_Thread_Count; i++)
//	{
//		// �پ��� ��Ŷ�� ������ ������.
//		std::thread* _Send_Various_Packet_Thread = new std::thread(std::bind(&DummyClient::SendVariousPacketFunction, this));
//		Thread_List.push_back(_Send_Various_Packet_Thread);
//	}
//}

void DummyClient::BoundlessSendFunction()
{
	C2S_Message* _msg = new C2S_Message;
	std::vector<Network_Message*> Message_Vec;
	strcpy_s(_msg->Message_Buffer, std::string("TestClient").c_str());
	C2NetWorkAPI* my_NetWork = nullptr;
	std::chrono::time_point _Start_Time = std::chrono::system_clock::now();

	my_NetWork = new C2NetWorkAPI();
	my_NetWork->Initialize(C2NetWork_Name::DHNet);
	// Connect ���� ���..
	//while (!my_NetWork->Connect(9000, "192.168.0.56")) {}
	while (!my_NetWork->Connect(729, CONNECT_IP)) {}

	while (!End_Flag)
	{
		/// Send �ð� ����.
		_Start_Time = std::chrono::system_clock::now();

		my_NetWork->Send(_msg);

		std::chrono::duration<double> _Proceed_Time = std::chrono::system_clock::now() - _Start_Time;
		double _Poceed_Time_Ms = _Proceed_Time.count() * 1000;

		/// 1ȸ �����ߴٰ� Ƚ������.
		Total_Count_1.fetch_add(1);
		/// 1ȸ ����� �ɸ� �ð��� ����صд�.
		Total_Time_1.fetch_add(_Poceed_Time_Ms, std::memory_order_relaxed);

		/// �ʹ� ���̺����� ���Ȯ���� �������.. sleep 10��ŭ �ش�. �����ü�� ���ǹ���.
		Sleep(10);
	}

	/// ���� �����÷��׸� �״µ� connect�۾����� ��Ʈ��ũ�� �����ϸ� ������.
	if (my_NetWork != nullptr)
	{
		my_NetWork->End();
		my_NetWork = nullptr;
	}
}

void DummyClient::BoundlessEndFunction()
{
	C2S_Message* _msg = new C2S_Message;
	std::vector<Network_Message*> Message_Vec;
	strcpy_s(_msg->Message_Buffer, std::string("TestClient").c_str());
	C2NetWorkAPI* my_NetWork = nullptr;
	std::chrono::time_point _Start_Time = std::chrono::system_clock::now();

	while (!End_Flag)
	{
		if (my_NetWork == nullptr)
		{
			_Start_Time = std::chrono::system_clock::now();
			my_NetWork = new C2NetWorkAPI();
			my_NetWork->Initialize(C2NetWork_Name::DHNet);
		}
		// Connect ���� ���..
		if (!my_NetWork->Connect(9000, CONNECT_IP))
		{
			continue;
		}
		my_NetWork->End();

		std::chrono::duration<double> _Proceed_Time = std::chrono::system_clock::now() - _Start_Time;
		double _Poceed_Time_Ms = _Proceed_Time.count() * 1000;

		/// 1ȸ �����ߴٰ� Ƚ������.
		Total_Count_2.fetch_add(1);
		/// 1ȸ ����� �ɸ� �ð��� ����صд�.
		Total_Time_2.fetch_add(_Poceed_Time_Ms, std::memory_order_relaxed);

		my_NetWork = nullptr;

		Sleep(0);
	}

	/// ���� �����÷��׸� �״µ� connect�۾����� ��Ʈ��ũ�� �����ϸ� ������.
	if (my_NetWork != nullptr)
	{
		my_NetWork->End();
		my_NetWork = nullptr;
	}
}

void DummyClient::SendEndFunction()
{
	C2S_Message* _msg = new C2S_Message;
	strcpy_s(_msg->Message_Buffer, std::string("TestClient").c_str());
	C2NetWorkAPI* my_NetWork = nullptr;
	std::chrono::time_point _Start_Time = std::chrono::system_clock::now();

	while (!End_Flag)
	{
		if (my_NetWork == nullptr)
		{
			_Start_Time = std::chrono::system_clock::now();
			my_NetWork = new C2NetWorkAPI();
			my_NetWork->Initialize(C2NetWork_Name::DHNet);
		}
		// Connect ���� ���..
		bool _Test_Result = my_NetWork->Connect(9000, CONNECT_IP);

		if (!my_NetWork->Connect(9000, CONNECT_IP))
		{
			continue;
		}
		my_NetWork->Send(_msg);
		my_NetWork->End();

		std::chrono::duration<double> _Proceed_Time = std::chrono::system_clock::now() - _Start_Time;
		double _Poceed_Time_Ms = _Proceed_Time.count() * 1000;

		/// 1ȸ �����ߴٰ� Ƚ������.
		Total_Count.fetch_add(1);
		/// 1ȸ ����� �ɸ� �ð��� ����صд�.
		Total_Time.fetch_add(_Poceed_Time_Ms, std::memory_order_relaxed);

		my_NetWork = nullptr;
		Sleep(0);
	}

	/// ���� �����÷��׸� �״µ� connect�۾����� ��Ʈ��ũ�� �����ϸ� ������.
	if (my_NetWork != nullptr)
	{
		my_NetWork->End();
		my_NetWork = nullptr;
	}
}

void DummyClient::SendVariousPacketFunction()
{

}

void DummyClient::StopAllThread()
{
	// �ٸ� ���̽��� ������ ���Ͽ� ������Ű�� ����.
	End_Flag = true;

	if (Thread_List.size() == 0)
	{
		// ���� �������� �׽�Ʈ���̽��� �������� ����
		printf("[DummyClient] �׽�Ʈ ���̽��� ���� �ϰ����� �ʽ��ϴ�. \n");
	}
	else
	{
		for (int i = 0; i < Thread_List.size(); i++)
		{
			// �׽�Ʈ ���̽��� ���Ǿ��� �����带 �����ϰ� �����Ѵ�.
			Thread_List[i]->join();
			delete Thread_List[i];
		}

		// ��� �����尡 ����Ǿ����� ���͸� ����ش�.
		Thread_List.clear();

		// ���� �׽�Ʈ ���̽��� ���� ����� ������ش�.
		switch (Current_TestCase)
		{
		case DummyClient::Case1:
		{
			TCHAR Write_Result[MAX_WORD];
			// ���̽� 1���� ���� ���
			printf("[DummyClient] CASE_%d �� ������. [Thread : %d ��]\n[����ð�] %f ms\t[����Ƚ��] %d ȸ\t[��� ����ð�] %f ms\n", 
				(int)Current_TestCase ,Make_Thread_Count, Total_Time.load(), Total_Count.load(), Total_Time / (double)Total_Count);

			_stprintf_s(Write_Result, _T("[DummyClient] CASE_%d �� ������. [Thread : %d ��]\n[����ð�] %f ms\t[����Ƚ��] %d ȸ\t[��� ����ð�] %f ms\n"),
				(int)Current_TestCase, Make_Thread_Count, Total_Time.load(), Total_Count.load(), Total_Time / (double)Total_Count);

			Logger->WriteLog(LogType::COMPLETE_Log, Write_Result);
		}
			break;
		case DummyClient::Case2:
		{
			TCHAR Write_Result[MAX_WORD];
			// ���̽� 2���� ���� ���
			printf("[DummyClient] CASE_%d �� ������. [Thread : %d ��]\n", (int)Current_TestCase, Make_Thread_Count);

			printf("[Send Thread : %d ��][����ð�] %f ms\t[����Ƚ��] %d ȸ\t[��� ����ð�] %f ms\n",
				Make_Thread_Count / 10, Total_Time_1.load(), Total_Count_1.load(), Total_Time_1 / (double)Total_Count_1);

			printf("[StartEnd Thread : %d ��][����ð�] %f ms\t[����Ƚ��] %d ȸ\t[��� ����ð�] %f ms\n",
				(Make_Thread_Count - (Make_Thread_Count / 10)), Total_Time_2.load(), Total_Count_2.load(), Total_Time_2 / (double)Total_Count_2);

			_stprintf_s(Write_Result, _T("[DummyClient] CASE_%d �� ������. [Thread : %d ��]\n[SendRecv Thread : %d ��][����ð�] %f ms\t[����Ƚ��] %d ȸ\t[��� ����ð�] %f ms\n[StartEnd Thread : %d ��][����ð�] %f ms\t[����Ƚ��] %d ȸ\t[��� ����ð�] %f ms\n"),
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

		/// ���� �ʱ�ȭ.
		Total_Time = 0.f;
		Total_Count = 0;

		Total_Time_1 = 0.f;
		Total_Time_2 = 0.f;
		Total_Count_1 = 0;
		Total_Count_2 = 0;

		// �ٸ� �׽�Ʈ���̽��� ����غ� �Ǿ��ٰ� �˷���.
		printf("[DummyClient] CASE_%d ���� ����Ǿ����ϴ�. ���ο� �׽�Ʈ���̽��� ���డ�� �մϴ�. \n", (int)Current_TestCase);
		Current_TestCase = TestCaseNumber::NonSet_TestCase;
	}

	// �ٸ� ���̽��� ������ ���� �غ� �Ϸ��.
	End_Flag = false;
}

void DummyClient::KeyInputFunction()
{
	printf("[DummyClient] ���� Ŭ���̾�Ʈ �׽�Ʈ�� �����մϴ�. \n");
	printf(" => CASE1[ ���� -> ������ -> ���� ���ѹݺ�] \t\t\t ���� Ű : Key[1] \n");
	printf(" => CASE2[ ���� -> ���� // ���� -> ������ ���ѹݺ�] \t\t ���� Ű : Key[2] \n");
	//printf(" => CASE3[ ���� -> ������(�پ��� ��Ŷ) ���ѹݺ�] \t\t ���� Ű : Key[3] \n");

	while (true)
	{
		/// ���� QŰ�� ������ ���� �������� �׽�Ʈ���̽��� �����Ѵ�.
		if(Key_IO->Key_Down('Q'))
		{
			StopAllThread();
		}

		/// ���� �׽�Ʈ���̽� 1,2,3 �� ����.
		if (Key_IO->Key_Down('1'))
		{
			StartCase1();
		}
		if (Key_IO->Key_Down('2'))
		{
			StartCase2();
		}

		Sleep(0);
	}
}
