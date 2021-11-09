#pragma once

#include "NetWorkBase.h"

class DHClient : public NetWorkBase
{
private:
	unsigned short			PORT = 9000;
	std::string				IP = "127.0.0.1";	// ���� ������
	CRITICAL_SECTION		g_CCS;			// Ŭ���̾�Ʈ ������ ���� ũ��Ƽ�� ����

	/// Recv�� �����͸� ���� �ص� �κ�.
	Concurrency::concurrent_queue<Network_Message*> Recv_Data_Queue;

	/// ���� ���Ͽ����� ������.
	std::shared_ptr<Socket_Struct> g_Server_Socket;
	std::vector<std::thread*> g_Work_Thread;
	std::thread* g_Connect_Client_Thread = nullptr;
	//std::thread* g_Exit_Check_Thread = nullptr;

	/// ������ ������ �����ߴ����� ���� ����.
	bool Is_Server_Connect_Success = false;

	/// IOCP �ڵ� (���������� Queue�� �����Ѵ�.)
	HANDLE	g_IOCP = nullptr;
	BOOL	g_Is_Exit = FALSE;

public:
	/// �⺻�����ڷ� ���� �κ�..
	DHClient();
	~DHClient();


public:
	virtual bool Start();
	virtual bool Send(Packet_Header* Send_Packet);
	virtual bool Recv(std::vector<Network_Message*>& _Message_Vec);
	virtual bool Connect(unsigned short _Port, std::string _IP);
	virtual bool End();

private:
	/// WorkThread�� CLIENT_THREAD_COUNT ������ŭ ����.
	void CreateWorkThread();

	/// Ŭ���̾�Ʈ�� WorkThread ����.
	void WorkThread();

	/// Ŭ���̾�Ʈ�� ������ üũ�� ���� ����.
	void ConnectThread();

	/// Ŭ���̾�Ʈ�� ���Ḧ üũ�ϱ� ���� ����.
	void ExitThread();

	/// ���� �����ǰ��ִ� Socket�� 1���϶��� ���Ḧ ȣ�� �� �� �ֵ��� ���� �Լ�.
	void Safe_CloseSocket();

	/// WSAReceive�� �ɾ�δ� �۾�. ( �ѹ��� �ɾ�־� ó���� ���� ������ ���� �� ���� ���� )
	bool Reserve_WSAReceive(SOCKET socket, Overlapped_Struct* psOverlapped = nullptr);

	/// IOType �� ���� ó���Լ���.
	void IOFunction_Recv(DWORD dwNumberOfBytesTransferred, Overlapped_Struct* psOverlapped, Socket_Struct* psSocket);
	void IOFunction_Send(DWORD dwNumberOfBytesTransferred, Overlapped_Struct* psOverlapped, Socket_Struct* psSocket);
};

