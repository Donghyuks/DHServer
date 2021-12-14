#pragma once

#include "NetWorkBase.h"

class DHClient : public NetWorkBase
{
private:
	unsigned short			PORT = 729;
	std::string				IP = "127.0.0.1";	// 서버 아이피
	CRITICAL_SECTION		g_CCS;			// 클라이언트 내에서 쓰는 크리티컬 섹션

	// Recv시 데이터를 큐에 넣어두고 관리한다.
	tbb::concurrent_queue<Network_Message*> Recv_Data_Queue;
	// Send시 데이터를 큐에 넣고두고 관리한다.
	tbb::concurrent_queue<Packet_Header*> Send_Data_Queue;
	// Overlapped Pool 을 이용하여 malloc 호출 횟수를 줄인다.
	ObjectPool<Overlapped_Struct>* Available_Overlapped;

	// 서버 소켓에대한 포인터.
	std::shared_ptr<Socket_Struct> g_Server_Socket;
	std::vector<std::thread*> g_Work_Thread;
	std::thread* g_Connect_Send_Client_Thread = nullptr;

	// 서버에 접속이 성공했는지에 대한 여부.
	bool Is_Server_Connect_Success = false;

	// IOCP 핸들 (내부적으로 Queue를 생성한다.)
	HANDLE	g_IOCP = nullptr;
	BOOL	g_Is_Exit = FALSE;

public:
	// 기본생성자로 쓰일 부분..
	DHClient();
	~DHClient();

public:
	virtual BOOL Send(Packet_Header* Send_Packet, SOCKET _Socket = INVALID_SOCKET) override;
	virtual BOOL Recv(std::vector<Network_Message>& _Message_Vec) override;
	virtual BOOL Connect(unsigned short _Port, std::string _IP) override;
	virtual BOOL End() override;

private:
	/// Thread Function
	// WorkThread를 CLIENT_THREAD_COUNT 개수만큼 생성.
	void CreateWorkThread();
	// 클라이언트의 WorkThread 로직.
	void WorkThread();
	// 클라이언트의 재접속 체크를 위한 로직.
	void ConnectSendThread();

	/// Socket Function
	// 현재 참조되고있는 Socket이 1개일때만 종료를 호출 할 수 있도록 만든 함수.
	void Safe_CloseSocket();

	/// Recv, Send Function
	// WSAReceive를 걸어두는 작업. ( 한번은 걸어둬야 처리에 대한 응답이 왔을 때 대응 가능 )
	bool Reserve_WSAReceive(SOCKET socket, Overlapped_Struct* psOverlapped = nullptr);
	// 하나의 데이터를 받아오면 수신큐에 넣는 부분.
	bool Push_RecvData(Packet_Header* _Data_Packet, Socket_Struct* _Socket_Struct, Overlapped_Struct* _Overlapped_Struct, size_t _Pull_Size);
	// Overlapped에 이전에 들어온 데이터를 백업하고 초기화하는 함수.
	bool BackUp_Overlapped(Overlapped_Struct* psOverlapped);
	// 클라이언트의 Send 큐를 살피며 메세지를 보내기 위함.
	void SendFunction();

	/// IOType 에 대한 처리함수들.
	void IOFunction_Recv(DWORD dwNumberOfBytesTransferred, Overlapped_Struct* psOverlapped, Socket_Struct* psSocket);
	void IOFunction_Send(DWORD dwNumberOfBytesTransferred, Overlapped_Struct* psOverlapped, Socket_Struct* psSocket);
};

