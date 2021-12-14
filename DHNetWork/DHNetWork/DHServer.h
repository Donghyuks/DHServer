#pragma once

/*
	2021/11/24 12:48 - CDH
	
	< 변경사항 >
		1. Buffer보다 큰 데이터가 들어올 때를 대비하여 처리하는 함수 추가. 
		
*/

#include "NetWorkBase.h"

class DHServer : public NetWorkBase
{
	/// 전역 변수 및 전역 함수.
public:
	static unsigned short	MAX_USER_COUNT;

private:
	unsigned short			PORT = 729;

	/// Overlapped IO 를 미리 생성하고 쓰기 위함.
	ObjectPool<Overlapped_Struct>* Available_Overlapped;	// 사용가능한 오버랩드

	// Recv시 데이터를 저장 해두고 처리하기 위함.
	tbb::concurrent_queue<Network_Message*> Recv_Data_Queue;
	// 조각난 데이터가 있다면 작업중인 소켓을 넣어두고 붙여준다.
	tbb::concurrent_hash_map<SOCKET, Packet_Header*> Merging_Big_Data;
	// 해당 해시 맵에 접근하기 위한 typedef
	typedef tbb::concurrent_hash_map<SOCKET, Packet_Header*> Big_Data_Find_Map;

	/// 클라이언트 소켓에대한 포인터.
	//std::list<std::shared_ptr<Socket_Struct>> g_Client_Socket_List;
	//////////////////////////////////////////////////////////////////////////
	// 	   Shared_ptr로 해보고싶었으나 다음과 같은 문제점이 있었음..
	// 	   1. WorkThread에서 reinterpret_cast로 Socket_Struct* 로는 변환이되나, Shared_ptr로는 변환이안됨..
	// 	   2. 해당 Socket_Struct를 리스트로써 관리를 하다가, 만약 클라이언트하나가 나가게되면 참조카운트가 0이되며 소멸자를 호출해버림.
	//////////////////////////////////////////////////////////////////////////

	/// 오브젝트 풀 클래스로 사용하려 했으나 다음과 같은 문제점이 존재.
	///	1. 소켓과 소켓 구조체가 처음 등록된 이후에, 1:1의 맵핑관계를 가지게 IOCP에 등록을 해놓음.
	/// 2. 1의 사유로 인해 Queue를 활용한 관리가 오히려 비용이 더 들고 번거로움.
	// 소켓에 대해 각각 소켓 구조체가 1대 1로 맵핑된 소켓 구조체 풀.
	tbb::concurrent_hash_map<SOCKET, Socket_Struct*> g_Socket_Struct_Pool;
	// 클라이언트가 접속하면 해당 클라이언트에 대한 관리.
	tbb::concurrent_hash_map<SOCKET, Socket_Struct*> g_Connected_Client;
	// 해당 해시 맵에 접근하기 위한 typedef
	typedef tbb::concurrent_hash_map<SOCKET, Socket_Struct*> Client_Map;

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
	/// 기본생성자로 쓰일 부분..
	DHServer();
	~DHServer();


public:
	/// 이 네트워크가 서버로 사용한다는 것을 알려주는 플래그나 다름없다. (Accept는 호스트 고유의 함수)
	virtual BOOL Accept(unsigned short _Port, unsigned short _Max_User_Count) override;
	/// SOCKET 이 Invaild 라면 모든 클라이언트에게 메세지 전송. / 그 외에는 해당 소켓에 메세지 전송.
	virtual BOOL Send(Packet_Header* Send_Packet, SOCKET _Socket = INVALID_SOCKET) override;
	virtual BOOL Recv(std::vector<Network_Message>& _Message_Vec) override;
	virtual BOOL Disconnect(SOCKET _Socket) override;
	virtual BOOL End() override;

private:
	/// Thread Function
	// 클라이언트의 WorkThread 로직.
	void WorkThread();
	// 클라이언트의 종료를 체크하기 위한 로직.
	void ExitThread();
	/// Thread Create
	// WorkThread를 CLIENT_THREAD_COUNT 개수만큼 생성.
	void CreateWorkThread();

	/// Socket Function
	// 클라이언트 소켓을 리스트에 추가하는 함수.
	bool AddClientSocket(Socket_Struct* psSocket);
	// 소켓을 재활용하기 위해 Disconnect를 거는 함수.
	void Reuse_Socket(SOCKET _Socket);
	// 클라이언트 소켓리스트에서 해당 소켓을 제외하는 함수.
	void Delete_in_Socket_List(Socket_Struct* psSocket);
	// 해당 소켓이 현재 접속해있는지 검색하는 함수.
	bool FindSocketOnClient(SOCKET _Target);

	/// Recv , Send Function
	// WSAReceive를 걸어두는 작업. ( 한번은 걸어둬야 처리에 대한 응답이 왔을 때 대응 가능 )
	bool Reserve_WSAReceive(SOCKET socket, Overlapped_Struct* psOverlapped = nullptr);
	// 해당 소켓에 메세지를 보내는 함수.
	bool SendTargetSocket(SOCKET socket, Packet_Header* psPacket);
	// 모든 소켓에 메세지를 보내는 함수.
	bool BroadCastMessage(Packet_Header* psPacket);
	// Overlapped에 이전에 들어온 데이터를 백업하고 초기화하는 함수.
	bool BackUp_Overlapped(Overlapped_Struct* psOverlapped);
	// 하나의 데이터를 받아오면 수신큐에 넣는 부분.
	bool Push_RecvData(Packet_Header* _Data_Packet, Socket_Struct* _Socket_Struct, Overlapped_Struct* _Overlapped_Struct, size_t _Pull_Size);

	/// IOType 에 대한 처리함수들.
	void IOFunction_Recv(DWORD dwNumberOfBytesTransferred, Overlapped_Struct* psOverlapped, Socket_Struct* psSocket);
	void IOFunction_Send(DWORD dwNumberOfBytesTransferred, Overlapped_Struct* psOverlapped, Socket_Struct* psSocket);
	void IOFunction_Accept(Overlapped_Struct* psOverlapped);
	void IOFunction_Disconnect(Overlapped_Struct* psOverlapped);
};

