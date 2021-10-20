#include "Client.h"

#include <assert.h>
#include <functional>

Client::~Client()
{
	/// Ŭ���̾�Ʈ�� ����� �� ���� CS�� ����.
	DeleteCriticalSection(&g_CCS);
	g_Server_Socket.reset();
}

bool Client::Start()
{
	/// ������ �ʱ�ȭ.
	WSADATA wsa;
	/// CS �ʱ�ȭ.
	InitializeCriticalSection(&g_CCS);

	char Error_Buffer[PACKET_BUFSIZE] = { 0, };

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		sprintf_s(Error_Buffer, "[TCP Ŭ���̾�Ʈ] ���� �߻� -- WSAStartup() :");
		err_display(Error_Buffer);

		return LOGIC_FAIL;
	}

	/// ������ ����ü�� �����Ѵ�.
	g_Server_Socket = std::make_shared<Socket_Struct>();

	/// IOCP�� �����Ѵ�.
	g_IOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	assert(nullptr != g_IOCP);

	/// Ŭ���̾�Ʈ�� ���Ḧ üũ�ϱ� ���� ������ ����.
	g_Exit_Check_Thread = new std::thread(std::bind(&Client::ExitThread, this));

	/// CLIENT_THREAD_COUNT ������ŭ WorkThread�� �����Ѵ�.
	CreateWorkThread();

	printf_s("[TCP Ŭ���̾�Ʈ] ����\n");

	/// Ŭ���̾�Ʈ�� ��������� ������ ������ ����. (������ �õ��� ��� �ϱ����ؼ�)
	g_Connect_Client_Thread = new std::thread(std::bind(&Client::ConnectThread, this));

	return LOGIC_SUCCESS;
}

bool Client::Send(Packet_Header* Send_Packet)
{
	assert(INVALID_SOCKET != g_Server_Socket->m_Socket);
	assert(nullptr != Send_Packet);

	// ��ϵ��� ���� ��Ŷ�� ������ �� ����.
	// [Ŭ��] -> [����]
	if (C2S_Packet_Type_MAX <= Send_Packet->Packet_Type)
	{
		return FALSE;
	}

	// �������� ����
	Overlapped_Struct* psOverlapped = new Overlapped_Struct;
	psOverlapped->m_IOType = Overlapped_Struct::IOType::IOType_Send;
	psOverlapped->m_Socket = g_Server_Socket->m_Socket;

	// ��Ŷ ����
	psOverlapped->m_Data_Size = 2 + Send_Packet->Packet_Size;

	if (sizeof(psOverlapped->m_Buffer) < psOverlapped->m_Data_Size)
	{
		delete psOverlapped;
		return FALSE;
	}

	memcpy_s(psOverlapped->m_Buffer, sizeof(psOverlapped->m_Buffer), Send_Packet, psOverlapped->m_Data_Size);

	// WSABUF ����
	WSABUF wsaBuffer;
	wsaBuffer.buf = psOverlapped->m_Buffer;
	wsaBuffer.len = psOverlapped->m_Data_Size;

	// WSASend() �������� �ɱ�
	DWORD dwNumberOfBytesSent = 0;

	int iResult = WSASend(psOverlapped->m_Socket,
		&wsaBuffer,
		1,
		&dwNumberOfBytesSent,
		0,
		&psOverlapped->wsaOverlapped,
		nullptr);

	if ((SOCKET_ERROR == iResult) && (WSAGetLastError() != WSA_IO_PENDING))
	{
		char szBuffer[PACKET_BUFSIZE] = { 0, };
		sprintf_s(szBuffer, "[TCP Ŭ���̾�Ʈ] ���� �߻� -- WSASend() :");
		err_display(szBuffer);

		Safe_CloseSocket();
		delete psOverlapped;
	}

	return TRUE;
}

bool Client::Recv(Packet_Header** Recv_Packet, char* MsgBuff)
{
	/// ť�� ������� FALSE�� ��ȯ�Ѵ�.
	if (Recv_Data_Queue.empty())
		return FALSE;

	/// ť���� �����͸� ���´�.
	Packet_Header* cpcHeader;
	Recv_Data_Queue.try_pop(cpcHeader);

	/// �޼��� Ÿ������ Header ĳ����.
	S2C_Message* S2C_Msg = static_cast<S2C_Message*>(cpcHeader);

	switch (cpcHeader->Packet_Type)
	{
		/// break���� �Ϻη� ���� �ʾƼ� Message�� Data�� ���̵����� ��쵵 ����Ѵ�.
	case S2C_Packet_Type_Message:         // ä�� �޼���
	{
		/// ä�� �޼����� ������ MsgBuff�� �������ش�.
		memcpy_s(MsgBuff, PACKET_BUFSIZE, S2C_Msg->Message_Buffer, PACKET_BUFSIZE);
	}
	case S2C_Packet_Type_Data:         // ĳġ���ε� ������.
	{
		/// ���� ����..
	}
	break;
	}

	// ����.
	delete cpcHeader;

	/// ť�� ������� ������ TRUE ��ȯ.
	return TRUE;
}

bool Client::End()
{
	PostQueuedCompletionStatus(g_IOCP, 0, 0, nullptr);

	for (auto k : g_Work_Thread)
	{
		k->join();
	}

	g_Work_Thread.clear();

	// IOCP ����
	CloseHandle(g_IOCP);
	g_IOCP = nullptr;

	// ���� ����
	WSACleanup();

	printf_s("[TCP Ŭ���̾�Ʈ] ����\n");

	return true;
}

void Client::err_display(const char* const cpcMSG)
{
	LPVOID lpMsgBuf = nullptr;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr,
		WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<TCHAR*>(&lpMsgBuf),
		0,
		nullptr);

	printf_s("%s %s", cpcMSG, reinterpret_cast<TCHAR>(lpMsgBuf));

	LocalFree(lpMsgBuf);
}

void Client::CreateWorkThread()
{
	for (int i = 0; i < CLIENT_THREAD_COUNT; i++)
	{
		/// Ŭ���̾�Ʈ �۾��� �ϴ� WorkThread�� ����.
		std::thread* Client_Work = new std::thread(std::bind(&Client::WorkThread, this));

		/// ������ �����ڿ� �־��ְ�..
		g_Work_Thread.push_back(Client_Work);

		/// ���ض� �������!!
	}
}

void Client::WorkThread()
{
	assert(nullptr != g_IOCP);

	while (TRUE)
	{
		DWORD        dwNumberOfBytesTransferred = 0;
		Socket_Struct* psSocket = nullptr;
		Overlapped_Struct* psOverlapped = nullptr;

		// GetQueuedCompletionStatus() - GQCS ��� �θ�
		// WSARead(), WSAWrite() ���� Overlapped I/O ���� ó�� ����� �޾ƿ��� �Լ�
		// PostQueuedCompletionStatus() �� ���ؼ��� GQCS �� ���Ͻ�ų �� �ִ�.( �Ϲ������� ������ ���� ó�� )
		BOOL bSuccessed = GetQueuedCompletionStatus(g_IOCP,												// IOCP �ڵ�
			&dwNumberOfBytesTransferred,						    // I/O �� ���� �������� ũ��
			reinterpret_cast<PULONG_PTR>(&psSocket),           // ������ IOCP ��Ͻ� �Ѱ��� Ű ��
																   // ( connect() �� ��, CreateIoCompletionPort() �� )
			reinterpret_cast<LPOVERLAPPED*>(&psOverlapped),   // WSARead(), WSAWrite() � ���� WSAOVERLAPPED
			INFINITE);										    // ��ȣ�� �߻��� ������ ������ ���

		// Ű�� nullptr �� ��� ������ ���Ḧ �ǹ�
		if (nullptr == psSocket)
		{
			// �ٸ� WorkerThread() ���� ���Ḧ ���ؼ�
			PostQueuedCompletionStatus(g_IOCP, 0, 0, nullptr);
			break;
		}

		assert(nullptr != psOverlapped);

		// �������� ��� üũ
		if (!bSuccessed)
		{
			if (SOCKET_ERROR != shutdown(psOverlapped->m_Socket, SD_BOTH))
			{
				Safe_CloseSocket();
				printf_s("[TCP Ŭ���̾�Ʈ] �������� ����� üũ�ϴ� ���� ������ ������ ������ϴ�.\n");
			}

			delete psOverlapped;
			continue;
		}

		// ���� ����
		if (0 == dwNumberOfBytesTransferred)
		{
			if (SOCKET_ERROR != shutdown(psOverlapped->m_Socket, SD_BOTH))
			{
				Safe_CloseSocket();
			}

			delete psOverlapped;
			continue;
		}

		// Overlapped I/O ó��
		switch (psOverlapped->m_IOType)
		{
		case Overlapped_Struct::IOType::IOType_Recv: IOFunction_Recv(dwNumberOfBytesTransferred, psOverlapped, psSocket); break; // WSARecv() �� Overlapped I/O �Ϸῡ ���� ó��
		case Overlapped_Struct::IOType::IOType_Send: IOFunction_Send(dwNumberOfBytesTransferred, psOverlapped, psSocket); break; // WSASend() �� Overlapped I/O �Ϸῡ ���� ó��
		}
	}
}

void Client::ConnectThread()
{
	while (!g_Is_Exit)
	{
		/// ���� �̹� ������ ������ �Ǿ��ִٸ�
		if (g_Server_Socket->Is_Connected == true)
		{
			/// ���� ������� �� �ʿ䰡 �����ϱ� �ٸ� �����忡�� �������� �Ѱ��ְ� while ������ ����.
			Sleep(0);
			continue;
		}

		if (INVALID_SOCKET == g_Server_Socket->m_Socket)
		{
			/// WSASocket ����.
			g_Server_Socket->m_Socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
			assert(0 != g_Server_Socket->m_Socket);

			if (INVALID_SOCKET == g_Server_Socket->m_Socket)
			{
				char szBuffer[PACKET_BUFSIZE] = { 0, };
				sprintf_s(szBuffer, "[TCP Ŭ���̾�Ʈ] ���� �߻� -- WSASocket() :");
				err_display(szBuffer);
				break;
			}
		}

		printf_s("[TCP Ŭ���̾�Ʈ] ���� �õ�\n");

		/// connect �۾� �õ�.
		while (!g_Is_Exit)
		{
			/// ������ PORT�� IP�� ������ �õ��Ѵ�.
			SOCKADDR_IN serveraddr;
			ZeroMemory(&serveraddr, sizeof(serveraddr));
			serveraddr.sin_family = AF_INET;
			serveraddr.sin_port = htons(PORT);
			serveraddr.sin_addr.s_addr = inet_addr(IP.c_str());

			if (SOCKET_ERROR == connect(g_Server_Socket->m_Socket, reinterpret_cast<SOCKADDR*>(&serveraddr), sizeof(serveraddr)))
			{
				if (WSAGetLastError() == WSAECONNREFUSED)
				{
					printf_s("[TCP Ŭ���̾�Ʈ] ���� ���� ��õ�\n");
					continue;
				}

				Safe_CloseSocket();
			}

			break;
		}

		/// ����� �����ص� ���� üũ
		// 1. ���� �÷��װ� �����ְų�
		// 2. ������ �Ҵ�Ǿ����� �ʴٸ� IOCP�� ������� �ʴ´�.
		if ((g_Is_Exit) || (INVALID_SOCKET == g_Server_Socket->m_Socket))
		{
			continue;
		}

		/// ������ �Ǿ��ٴ� flag
		g_Server_Socket->Is_Connected = TRUE;
		printf_s("[TCP Ŭ���̾�Ʈ] ������ ���� �Ϸ�\n");

		/// ������ IOCP �� Ű ���� �Բ� ���
		if (CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_Server_Socket->m_Socket), g_IOCP, reinterpret_cast<ULONG_PTR>(g_Server_Socket.get()), 0) != g_IOCP)
		{
			/// ���� ��Ͽ� �����Ѵٸ� ���� ����.
			Safe_CloseSocket();
			continue;
		}

		/// WSARecv() �ɾ�α�.
		if (!Reserve_WSAReceive(g_Server_Socket->m_Socket))
		{
			Safe_CloseSocket();
			continue;
		}
	}
}

void Client::ExitThread()
{
	while (TRUE)
	{
		if (GetAsyncKeyState(VK_ESCAPE))
		{
			g_Is_Exit = true;
			End();
			break;
		}
	}
}

void Client::Safe_CloseSocket()
{
	/// ���� 2�� �̻��� �Լ��� CloseSocket �Լ��� ȣ���ϰ� �� ��찡 ���� ���� �����ϱ�..
	EnterCriticalSection(&g_CCS);

	while (!g_Server_Socket.unique())
	{
		/// �ٸ� ���� �����ǰ� �ִ� �κ��� �ִٸ�, ���ᰡ �� ������ ��ٸ���.
		Sleep(0);
	}

	if (g_Server_Socket.unique())
	{
		shutdown(g_Server_Socket->m_Socket, SD_BOTH);
		closesocket(g_Server_Socket->m_Socket);
		g_Server_Socket->m_Socket = INVALID_SOCKET;
		g_Server_Socket->Is_Connected = FALSE;

		printf_s("[TCP Ŭ���̾�Ʈ] ������ ���� ����\n");
	}

	LeaveCriticalSection(&g_CCS);
}

bool Client::Reserve_WSAReceive(SOCKET socket, Overlapped_Struct* psOverlapped /* = nullptr */)
{
	assert(INVALID_SOCKET != socket);

	BOOL bRecycleOverlapped = TRUE;

	// ����� �������带 ���� �ʾ����� ����
	if (nullptr == psOverlapped)
	{
		psOverlapped = new Overlapped_Struct;
		bRecycleOverlapped = FALSE;
	}

	// �������� ����
	psOverlapped->m_IOType = Overlapped_Struct::IOType::IOType_Recv;
	psOverlapped->m_Socket = socket;

	// WSABUF ����
	WSABUF wsaBuffer;
	wsaBuffer.buf = psOverlapped->m_Buffer + psOverlapped->m_Data_Size;
	wsaBuffer.len = sizeof(psOverlapped->m_Buffer) - psOverlapped->m_Data_Size;

	// WSARecv() �������� �ɱ�
	DWORD dwNumberOfBytesRecvd = 0, dwFlag = 0;

	int iResult = WSARecv(psOverlapped->m_Socket,
		&wsaBuffer,
		1,
		&dwNumberOfBytesRecvd,
		&dwFlag,
		&psOverlapped->wsaOverlapped,
		nullptr);

	if ((SOCKET_ERROR == iResult) && (WSAGetLastError() != WSA_IO_PENDING))
	{
		char szBuffer[PACKET_BUFSIZE] = { 0, };
		sprintf_s(szBuffer, "[TCP Ŭ���̾�Ʈ] ���� �߻� -- WSARecv() :");
		err_display(szBuffer);

		if (!bRecycleOverlapped)
		{
			delete psOverlapped;
		}
		return FALSE;
	}

	return TRUE;
}

void Client::IOFunction_Recv(DWORD dwNumberOfBytesTransferred, Overlapped_Struct* psOverlapped, Socket_Struct* psSocket)
{
	printf_s("[TCP Ŭ���̾�Ʈ] ��Ŷ ���� �Ϸ� <- %d ����Ʈ\n", dwNumberOfBytesTransferred);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// ��Ŷ ó��
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// ������ �����͵� ũ�⸦ ���� �����ش�.
	psOverlapped->m_Data_Size += dwNumberOfBytesTransferred;

	// ó���� �����Ͱ� ������ ó��
	while (psOverlapped->m_Data_Size > 0)
	{
		// header ũ��� 2 ����Ʈ( ���� )
		static const unsigned short cusHeaderSize = 2;

		// header �� �� ���� ���ߴ�. �̾ recv()
		if (cusHeaderSize > psOverlapped->m_Data_Size)
		{
			break;
		}

		// body �� ũ��� N ����Ʈ( ���� ), ��Ŷ�� �������
		unsigned short usBodySize = *reinterpret_cast<unsigned short*>(psOverlapped->m_Buffer);
		unsigned short usPacketSize = cusHeaderSize + usBodySize;

		// �ϳ��� ��Ŷ�� �� ���� ���ߴ�. �̾ recv()
		if (usPacketSize > psOverlapped->m_Data_Size)
		{
			break;
		}

		// Ŭ���̾�Ʈ�κ��� ������ ��Ŷ
		Packet_Header* cpcHeader = reinterpret_cast<Packet_Header*>(malloc(usPacketSize));
		memcpy_s(cpcHeader, usPacketSize, psOverlapped->m_Buffer, usPacketSize);

		// ������ ��Ŷ�� ť�� �־�ΰ� ���߿� ó���Ѵ�.
		Recv_Data_Queue.push(cpcHeader);

		// �����͵��� �̹��� ó���Ѹ�ŭ ����.
		memcpy_s(psOverlapped->m_Buffer, psOverlapped->m_Data_Size,
			psOverlapped->m_Buffer + usPacketSize, psOverlapped->m_Data_Size - usPacketSize);

		// ó���� ��Ŷ ũ�⸸ŭ ó���ҷ� ����
		psOverlapped->m_Data_Size -= usPacketSize;
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// ���� �ɱ�( �̹��� ����� �������带 �ٽ� ��� )
	if (!Reserve_WSAReceive(psOverlapped->m_Socket, psOverlapped))
	{
		if (SOCKET_ERROR != shutdown(psOverlapped->m_Socket, SD_BOTH))
		{
			Safe_CloseSocket();
		}

		delete psOverlapped;
		return;
	}
}

void Client::IOFunction_Send(DWORD dwNumberOfBytesTransferred, Overlapped_Struct* psOverlapped, Socket_Struct* psSocket)
{
	printf_s("[TCP Ŭ���̾�Ʈ] ��Ŷ �۽� �Ϸ� -> %d ����Ʈ\n", dwNumberOfBytesTransferred);

	delete psOverlapped;
}
