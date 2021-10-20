#pragma once

#ifdef NETWORK_EXPORTS
#define NETWORK_DLL __declspec(dllexport)
#else
#define NETWORK_DLL __declspec(dllimport)
#endif

#include "SharedNetWorkStruct.h"
#include "SharedPacket.h"
#include <memory>
#include <vector>
#include <list>
#include <map>
#include <thread>
#include <concurrent_queue.h>

class NETWORK_DLL Server
{
	/// 전역 변수 및 전역 함수.
public:
	static unsigned short	MAX_USER_COUNT;
	/// Accept시 반영할 함수.
	static int CALLBACK AcceptFunction(LPWSABUF lpCallerId, LPWSABUF lpCallerData, LPQOS lpSQOS, LPQOS lpGQOS, LPWSABUF lpCalleeId, LPWSABUF lpCalleeData, GROUP FAR* g, DWORD_PTR dwCallbackData);

private:
	unsigned short			PORT = 9000;
	CRITICAL_SECTION		g_SCS;			// 서버 내에서 쓰는 크리티컬 섹션
	CRITICAL_SECTION		g_Aceept_CS;	// 서버 Accept데이터를 접근하기 위해 생성한 크리티컬 섹션.

	/// Recv시 데이터를 저장 해둘 부분.
	Concurrency::concurrent_queue<Packet_Header*> Recv_Data_Queue;

	/// 클라이언트 소켓에대한 포인터.
	std::list<std::shared_ptr<Socket_Struct>> g_Client_Socket_List;

	/// 서버의 리슨 소켓.
	std::shared_ptr<SOCKET> g_Listen_Socket;
	std::vector<std::thread*> g_Work_Thread;

	/// Accept 처리를 위한 쓰레드.
	std::thread* g_Accept_Client_Thread = nullptr;
	std::thread* g_Exit_Check_Thread = nullptr;

	/// IOCP 핸들 (내부적으로 Queue를 생성한다.)
	HANDLE	g_IOCP = nullptr;
	BOOL	g_Is_Exit = FALSE;

public:
	Server(unsigned short _PORT, unsigned short _MAX_USER_COUNT) { PORT = _PORT; MAX_USER_COUNT = _MAX_USER_COUNT; }
	/// 기본생성자로 쓰일 부분..
	Server();
	~Server();


public:
	virtual bool Start();
	/// 모든 클라이언트에게 메세지를 보냄.
	virtual bool Send(Packet_Header* Send_Packet);
	virtual bool Recv(Packet_Header** Recv_Packet, char* MsgBuff);
	virtual bool End();

protected:
	void err_display(const char* const cpcMSG);

private:
	// Thread Function
	/// 클라이언트의 WorkThread 로직.
	void WorkThread();
	/// 클라이언트의 재접속 체크를 위한 로직.
	void AcceptThread();
	/// 클라이언트의 종료를 체크하기 위한 로직.
	void ExitThread();
	// Thread Create
	/// WorkThread를 CLIENT_THREAD_COUNT 개수만큼 생성.
	void CreateWorkThread();

	// Socket Function
	/// 클라이언트 소켓을 리스트에 추가하는 함수.
	bool AddClientSocket(std::shared_ptr<Socket_Struct> psSocket);
	/// 리스트에 있는 소켓을 삭제하는 함수.
	bool RemoveClientSocket(std::shared_ptr<Socket_Struct> psSocket);
	/// 현재 참조되고있는 Socket이 1개일때만 종료를 호출 할 수 있도록 만든 함수.
	void Safe_CloseSocket(std::shared_ptr<Socket_Struct> psSocket);
	/// shared_ptr 형태가아닌 일반 포인터형도 삭제 가능하도록
	void Safe_CloseSocket(Socket_Struct* psSocket);

	// Recv , Send Function
	/// WSAReceive를 걸어두는 작업. ( 한번은 걸어둬야 처리에 대한 응답이 왔을 때 대응 가능 )
	bool Reserve_WSAReceive(SOCKET socket, Overlapped_Struct* psOverlapped = nullptr);
	/// 해당 소켓에 메세지를 보내는 함수.
	bool SendTargetSocket(SOCKET socket, Packet_Header* psPacket);

	/// IOType 에 대한 처리함수들.
	void IOFunction_Recv(DWORD dwNumberOfBytesTransferred, Overlapped_Struct* psOverlapped, Socket_Struct* psSocket);
	void IOFunction_Send(DWORD dwNumberOfBytesTransferred, Overlapped_Struct* psOverlapped, Socket_Struct* psSocket);
};

