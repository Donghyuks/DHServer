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
	/// ���� ���� �� ���� �Լ�.
public:
	static unsigned short	MAX_USER_COUNT;
	/// Accept�� �ݿ��� �Լ�.
	static int CALLBACK AcceptFunction(LPWSABUF lpCallerId, LPWSABUF lpCallerData, LPQOS lpSQOS, LPQOS lpGQOS, LPWSABUF lpCalleeId, LPWSABUF lpCalleeData, GROUP FAR* g, DWORD_PTR dwCallbackData);

private:
	unsigned short			PORT = 9000;
	CRITICAL_SECTION		g_SCS;			// ���� ������ ���� ũ��Ƽ�� ����
	CRITICAL_SECTION		g_Aceept_CS;	// ���� Accept�����͸� �����ϱ� ���� ������ ũ��Ƽ�� ����.

	/// Recv�� �����͸� ���� �ص� �κ�.
	Concurrency::concurrent_queue<Packet_Header*> Recv_Data_Queue;

	/// Ŭ���̾�Ʈ ���Ͽ����� ������.
	std::list<std::shared_ptr<Socket_Struct>> g_Client_Socket_List;

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
	Server(unsigned short _PORT, unsigned short _MAX_USER_COUNT) { PORT = _PORT; MAX_USER_COUNT = _MAX_USER_COUNT; }
	/// �⺻�����ڷ� ���� �κ�..
	Server();
	~Server();


public:
	virtual bool Start();
	/// ��� Ŭ���̾�Ʈ���� �޼����� ����.
	virtual bool Send(Packet_Header* Send_Packet);
	virtual bool Recv(Packet_Header** Recv_Packet, char* MsgBuff);
	virtual bool End();

protected:
	void err_display(const char* const cpcMSG);

private:
	// Thread Function
	/// Ŭ���̾�Ʈ�� WorkThread ����.
	void WorkThread();
	/// Ŭ���̾�Ʈ�� ������ üũ�� ���� ����.
	void AcceptThread();
	/// Ŭ���̾�Ʈ�� ���Ḧ üũ�ϱ� ���� ����.
	void ExitThread();
	// Thread Create
	/// WorkThread�� CLIENT_THREAD_COUNT ������ŭ ����.
	void CreateWorkThread();

	// Socket Function
	/// Ŭ���̾�Ʈ ������ ����Ʈ�� �߰��ϴ� �Լ�.
	bool AddClientSocket(std::shared_ptr<Socket_Struct> psSocket);
	/// ����Ʈ�� �ִ� ������ �����ϴ� �Լ�.
	bool RemoveClientSocket(std::shared_ptr<Socket_Struct> psSocket);
	/// ���� �����ǰ��ִ� Socket�� 1���϶��� ���Ḧ ȣ�� �� �� �ֵ��� ���� �Լ�.
	void Safe_CloseSocket(std::shared_ptr<Socket_Struct> psSocket);
	/// shared_ptr ���°��ƴ� �Ϲ� ���������� ���� �����ϵ���
	void Safe_CloseSocket(Socket_Struct* psSocket);

	// Recv , Send Function
	/// WSAReceive�� �ɾ�δ� �۾�. ( �ѹ��� �ɾ�־� ó���� ���� ������ ���� �� ���� ���� )
	bool Reserve_WSAReceive(SOCKET socket, Overlapped_Struct* psOverlapped = nullptr);
	/// �ش� ���Ͽ� �޼����� ������ �Լ�.
	bool SendTargetSocket(SOCKET socket, Packet_Header* psPacket);

	/// IOType �� ���� ó���Լ���.
	void IOFunction_Recv(DWORD dwNumberOfBytesTransferred, Overlapped_Struct* psOverlapped, Socket_Struct* psSocket);
	void IOFunction_Send(DWORD dwNumberOfBytesTransferred, Overlapped_Struct* psOverlapped, Socket_Struct* psSocket);
};

