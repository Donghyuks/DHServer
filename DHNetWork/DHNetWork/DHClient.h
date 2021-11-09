#pragma once

#include "NetWorkBase.h"

class DHClient : public NetWorkBase
{
private:
	unsigned short			PORT = 9000;
	std::string				IP = "127.0.0.1";	// 서버 아이피
	CRITICAL_SECTION		g_CCS;			// 클라이언트 내에서 쓰는 크리티컬 섹션

	/// Recv시 데이터를 저장 해둘 부분.
	Concurrency::concurrent_queue<Network_Message*> Recv_Data_Queue;

	/// 서버 소켓에대한 포인터.
	std::shared_ptr<Socket_Struct> g_Server_Socket;
	std::vector<std::thread*> g_Work_Thread;
	std::thread* g_Connect_Client_Thread = nullptr;
	//std::thread* g_Exit_Check_Thread = nullptr;

	/// 서버에 접속이 성공했는지에 대한 여부.
	bool Is_Server_Connect_Success = false;

	/// IOCP 핸들 (내부적으로 Queue를 생성한다.)
	HANDLE	g_IOCP = nullptr;
	BOOL	g_Is_Exit = FALSE;

public:
	/// 기본생성자로 쓰일 부분..
	DHClient();
	~DHClient();


public:
	virtual bool Start();
	virtual bool Send(Packet_Header* Send_Packet);
	virtual bool Recv(std::vector<Network_Message*>& _Message_Vec);
	virtual bool Connect(unsigned short _Port, std::string _IP);
	virtual bool End();

private:
	/// WorkThread를 CLIENT_THREAD_COUNT 개수만큼 생성.
	void CreateWorkThread();

	/// 클라이언트의 WorkThread 로직.
	void WorkThread();

	/// 클라이언트의 재접속 체크를 위한 로직.
	void ConnectThread();

	/// 클라이언트의 종료를 체크하기 위한 로직.
	void ExitThread();

	/// 현재 참조되고있는 Socket이 1개일때만 종료를 호출 할 수 있도록 만든 함수.
	void Safe_CloseSocket();

	/// WSAReceive를 걸어두는 작업. ( 한번은 걸어둬야 처리에 대한 응답이 왔을 때 대응 가능 )
	bool Reserve_WSAReceive(SOCKET socket, Overlapped_Struct* psOverlapped = nullptr);

	/// IOType 에 대한 처리함수들.
	void IOFunction_Recv(DWORD dwNumberOfBytesTransferred, Overlapped_Struct* psOverlapped, Socket_Struct* psSocket);
	void IOFunction_Send(DWORD dwNumberOfBytesTransferred, Overlapped_Struct* psOverlapped, Socket_Struct* psSocket);
};

