#pragma once

#include "NetWorkBase.h"

class DHClient : public NetWorkBase
{
private:
	unsigned short			PORT = 729;
	std::string				IP = "127.0.0.1";	// ���� ������
	CRITICAL_SECTION		g_CCS;			// Ŭ���̾�Ʈ ������ ���� ũ��Ƽ�� ����

	// Recv�� �����͸� ť�� �־�ΰ� �����Ѵ�.
	tbb::concurrent_queue<Network_Message*> Recv_Data_Queue;
	// Send�� �����͸� ť�� �ְ�ΰ� �����Ѵ�.
	tbb::concurrent_queue<Packet_Header*> Send_Data_Queue;
	// Overlapped Pool �� �̿��Ͽ� malloc ȣ�� Ƚ���� ���δ�.
	ObjectPool<Overlapped_Struct>* Available_Overlapped;

	// ���� ���Ͽ����� ������.
	std::shared_ptr<Socket_Struct> g_Server_Socket;
	std::vector<std::thread*> g_Work_Thread;
	std::thread* g_Connect_Send_Client_Thread = nullptr;

	// ������ ������ �����ߴ����� ���� ����.
	bool Is_Server_Connect_Success = false;

	// IOCP �ڵ� (���������� Queue�� �����Ѵ�.)
	HANDLE	g_IOCP = nullptr;
	BOOL	g_Is_Exit = FALSE;

public:
	// �⺻�����ڷ� ���� �κ�..
	DHClient();
	~DHClient();

public:
	virtual BOOL Send(Packet_Header* Send_Packet, SOCKET _Socket = INVALID_SOCKET) override;
	virtual BOOL Recv(std::vector<Network_Message>& _Message_Vec) override;
	virtual BOOL Connect(unsigned short _Port, std::string _IP) override;
	virtual BOOL End() override;

private:
	/// Thread Function
	// WorkThread�� CLIENT_THREAD_COUNT ������ŭ ����.
	void CreateWorkThread();
	// Ŭ���̾�Ʈ�� WorkThread ����.
	void WorkThread();
	// Ŭ���̾�Ʈ�� ������ üũ�� ���� ����.
	void ConnectSendThread();

	/// Socket Function
	// ���� �����ǰ��ִ� Socket�� 1���϶��� ���Ḧ ȣ�� �� �� �ֵ��� ���� �Լ�.
	void Safe_CloseSocket();

	/// Recv, Send Function
	// WSAReceive�� �ɾ�δ� �۾�. ( �ѹ��� �ɾ�־� ó���� ���� ������ ���� �� ���� ���� )
	bool Reserve_WSAReceive(SOCKET socket, Overlapped_Struct* psOverlapped = nullptr);
	// �ϳ��� �����͸� �޾ƿ��� ����ť�� �ִ� �κ�.
	bool Push_RecvData(Packet_Header* _Data_Packet, Socket_Struct* _Socket_Struct, Overlapped_Struct* _Overlapped_Struct, size_t _Pull_Size);
	// Overlapped�� ������ ���� �����͸� ����ϰ� �ʱ�ȭ�ϴ� �Լ�.
	bool BackUp_Overlapped(Overlapped_Struct* psOverlapped);
	// Ŭ���̾�Ʈ�� Send ť�� ���Ǹ� �޼����� ������ ����.
	void SendFunction();

	/// IOType �� ���� ó���Լ���.
	void IOFunction_Recv(DWORD dwNumberOfBytesTransferred, Overlapped_Struct* psOverlapped, Socket_Struct* psSocket);
	void IOFunction_Send(DWORD dwNumberOfBytesTransferred, Overlapped_Struct* psOverlapped, Socket_Struct* psSocket);
};

