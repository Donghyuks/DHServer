#pragma once

/*
	2021/11/24 12:48 - CDH
	
	< ������� >
		1. Buffer���� ū �����Ͱ� ���� ���� ����Ͽ� ó���ϴ� �Լ� �߰�. 
		
*/

#include "NetWorkBase.h"

class DHServer : public NetWorkBase
{
	/// ���� ���� �� ���� �Լ�.
public:
	static unsigned short	MAX_USER_COUNT;

private:
	unsigned short			PORT = 729;

	/// Overlapped IO �� �̸� �����ϰ� ���� ����.
	ObjectPool<Overlapped_Struct>* Available_Overlapped;	// ��밡���� ��������

	// Recv�� �����͸� ���� �صΰ� ó���ϱ� ����.
	tbb::concurrent_queue<Network_Message*> Recv_Data_Queue;
	// ������ �����Ͱ� �ִٸ� �۾����� ������ �־�ΰ� �ٿ��ش�.
	tbb::concurrent_hash_map<SOCKET, Packet_Header*> Merging_Big_Data;
	// �ش� �ؽ� �ʿ� �����ϱ� ���� typedef
	typedef tbb::concurrent_hash_map<SOCKET, Packet_Header*> Big_Data_Find_Map;

	/// Ŭ���̾�Ʈ ���Ͽ����� ������.
	//std::list<std::shared_ptr<Socket_Struct>> g_Client_Socket_List;
	//////////////////////////////////////////////////////////////////////////
	// 	   Shared_ptr�� �غ���;����� ������ ���� �������� �־���..
	// 	   1. WorkThread���� reinterpret_cast�� Socket_Struct* �δ� ��ȯ�̵ǳ�, Shared_ptr�δ� ��ȯ�̾ȵ�..
	// 	   2. �ش� Socket_Struct�� ����Ʈ�ν� ������ �ϴٰ�, ���� Ŭ���̾�Ʈ�ϳ��� �����ԵǸ� ����ī��Ʈ�� 0�̵Ǹ� �Ҹ��ڸ� ȣ���ع���.
	//////////////////////////////////////////////////////////////////////////

	/// ������Ʈ Ǯ Ŭ������ ����Ϸ� ������ ������ ���� �������� ����.
	///	1. ���ϰ� ���� ����ü�� ó�� ��ϵ� ���Ŀ�, 1:1�� ���ΰ��踦 ������ IOCP�� ����� �س���.
	/// 2. 1�� ������ ���� Queue�� Ȱ���� ������ ������ ����� �� ��� ���ŷο�.
	// ���Ͽ� ���� ���� ���� ����ü�� 1�� 1�� ���ε� ���� ����ü Ǯ.
	tbb::concurrent_hash_map<SOCKET, Socket_Struct*> g_Socket_Struct_Pool;
	// Ŭ���̾�Ʈ�� �����ϸ� �ش� Ŭ���̾�Ʈ�� ���� ����.
	tbb::concurrent_hash_map<SOCKET, Socket_Struct*> g_Connected_Client;
	// �ش� �ؽ� �ʿ� �����ϱ� ���� typedef
	typedef tbb::concurrent_hash_map<SOCKET, Socket_Struct*> Client_Map;

	/// ������ ���� ����.
	std::shared_ptr<SOCKET> g_Listen_Socket;
	std::vector<std::thread*> g_Work_Thread;

	/// Accept ó���� ���� ������.
	std::thread* g_Accept_Client_Thread = nullptr;
	std::thread* g_Exit_Check_Thread = nullptr;


	/// IOCP �ڵ� (���������� Queue�� �����Ѵ�.)
	HANDLE	g_IOCP = nullptr;
	BOOL	g_Is_Exit = FALSE;

public:
	/// �⺻�����ڷ� ���� �κ�..
	DHServer();
	~DHServer();


public:
	/// �� ��Ʈ��ũ�� ������ ����Ѵٴ� ���� �˷��ִ� �÷��׳� �ٸ�����. (Accept�� ȣ��Ʈ ������ �Լ�)
	virtual BOOL Accept(unsigned short _Port, unsigned short _Max_User_Count) override;
	/// SOCKET �� Invaild ��� ��� Ŭ���̾�Ʈ���� �޼��� ����. / �� �ܿ��� �ش� ���Ͽ� �޼��� ����.
	virtual BOOL Send(Packet_Header* Send_Packet, SOCKET _Socket = INVALID_SOCKET) override;
	virtual BOOL Recv(std::vector<Network_Message>& _Message_Vec) override;
	virtual BOOL Disconnect(SOCKET _Socket) override;
	virtual BOOL End() override;

private:
	/// Thread Function
	// Ŭ���̾�Ʈ�� WorkThread ����.
	void WorkThread();
	// Ŭ���̾�Ʈ�� ���Ḧ üũ�ϱ� ���� ����.
	void ExitThread();
	/// Thread Create
	// WorkThread�� CLIENT_THREAD_COUNT ������ŭ ����.
	void CreateWorkThread();

	/// Socket Function
	// Ŭ���̾�Ʈ ������ ����Ʈ�� �߰��ϴ� �Լ�.
	bool AddClientSocket(Socket_Struct* psSocket);
	// ������ ��Ȱ���ϱ� ���� Disconnect�� �Ŵ� �Լ�.
	void Reuse_Socket(SOCKET _Socket);
	// Ŭ���̾�Ʈ ���ϸ���Ʈ���� �ش� ������ �����ϴ� �Լ�.
	void Delete_in_Socket_List(Socket_Struct* psSocket);
	// �ش� ������ ���� �������ִ��� �˻��ϴ� �Լ�.
	bool FindSocketOnClient(SOCKET _Target);

	/// Recv , Send Function
	// WSAReceive�� �ɾ�δ� �۾�. ( �ѹ��� �ɾ�־� ó���� ���� ������ ���� �� ���� ���� )
	bool Reserve_WSAReceive(SOCKET socket, Overlapped_Struct* psOverlapped = nullptr);
	// �ش� ���Ͽ� �޼����� ������ �Լ�.
	bool SendTargetSocket(SOCKET socket, Packet_Header* psPacket);
	// ��� ���Ͽ� �޼����� ������ �Լ�.
	bool BroadCastMessage(Packet_Header* psPacket);
	// Overlapped�� ������ ���� �����͸� ����ϰ� �ʱ�ȭ�ϴ� �Լ�.
	bool BackUp_Overlapped(Overlapped_Struct* psOverlapped);
	// �ϳ��� �����͸� �޾ƿ��� ����ť�� �ִ� �κ�.
	bool Push_RecvData(Packet_Header* _Data_Packet, Socket_Struct* _Socket_Struct, Overlapped_Struct* _Overlapped_Struct, size_t _Pull_Size);

	/// IOType �� ���� ó���Լ���.
	void IOFunction_Recv(DWORD dwNumberOfBytesTransferred, Overlapped_Struct* psOverlapped, Socket_Struct* psSocket);
	void IOFunction_Send(DWORD dwNumberOfBytesTransferred, Overlapped_Struct* psOverlapped, Socket_Struct* psSocket);
	void IOFunction_Accept(Overlapped_Struct* psOverlapped);
	void IOFunction_Disconnect(Overlapped_Struct* psOverlapped);
};

