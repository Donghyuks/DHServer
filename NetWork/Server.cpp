#include "Server.h"

#include <assert.h>
#include <functional>

unsigned short Server::MAX_USER_COUNT = 0;

Server::Server()
{

}

Server::~Server()
{
	/// Ŭ���̾�Ʈ�� ����� �� ���� CS�� ����.
	DeleteCriticalSection(&g_SCS);
	DeleteCriticalSection(&g_Aceept_CS);
}

bool Server::Start()
{
	/// ������ �ʱ�ȭ.
	WSADATA wsa;
	/// CS �ʱ�ȭ.
	InitializeCriticalSection(&g_SCS);
	InitializeCriticalSection(&g_Aceept_CS);

	char Error_Buffer[PACKET_BUFSIZE] = { 0, };

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		sprintf_s(Error_Buffer, "[TCP ����] ���� �߻� -- WSAStartup() :");
		err_display(Error_Buffer);

		return LOGIC_FAIL;
	}

	/// ������ ����ü�� �����Ѵ�.
	g_Listen_Socket = std::make_shared<SOCKET>();

	/// Socket ����.
	*g_Listen_Socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);

	if (INVALID_SOCKET == *g_Listen_Socket)
	{
		sprintf_s(Error_Buffer, "[TCP ����] ���� �߻� -- WSASocket() :");
		err_display(Error_Buffer);

		WSACleanup();
		return LOGIC_FAIL;
	}

	/// bind
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(PORT);
	if (SOCKET_ERROR == bind(*g_Listen_Socket, reinterpret_cast<SOCKADDR*>(&serveraddr), sizeof(serveraddr)))
	{
		sprintf_s(Error_Buffer, "[TCP ����] ���� �߻� -- bind() :");
		err_display(Error_Buffer);

		closesocket(*g_Listen_Socket);
		*g_Listen_Socket = INVALID_SOCKET;

		WSACleanup();
		return LOGIC_FAIL;
	}

	/// listen
	if (SOCKET_ERROR == listen(*g_Listen_Socket, 5))
	{
		sprintf_s(Error_Buffer, "[TCP ����] ���� �߻� -- listen() :");
		err_display(Error_Buffer);

		closesocket(*g_Listen_Socket);
		*g_Listen_Socket = INVALID_SOCKET;

		WSACleanup();
		return LOGIC_FAIL;
	}


	/// IOCP�� �����Ѵ�.
	g_IOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	assert(nullptr != g_IOCP);

	/// Worker Thread���� ���� �ھ�� ~ �ھ��*2 �� ������ �����Ѵٰ� �Ѵ�.
	/// �ý����� ������ �޾ƿͼ� �ھ���� *2����ŭ WorkThread�� �����Ѵ�.
	CreateWorkThread();

	/// Ŭ���̾�Ʈ�� ���Ḧ üũ�ϱ� ���� ������ ����.
	g_Exit_Check_Thread = new std::thread(std::bind(&Server::ExitThread, this));

	printf_s("[TCP ����] ����\n");

	/// Ŭ���̾�Ʈ�� ��������� ������ ������ ����. (������ �õ��� ��� �ϱ����ؼ�)
	g_Accept_Client_Thread = new std::thread(std::bind(&Server::AcceptThread, this));

	return LOGIC_SUCCESS;
}

bool Server::Send(Packet_Header* Send_Packet)
{
	/// ��� Ŭ���̾�Ʈ���� �޼����� ����.

	assert(nullptr != Send_Packet);

	// �Ӱ� ����
	{
		EnterCriticalSection(&g_SCS);

		// ��ü Ŭ���̾�Ʈ ���Ͽ� ��Ŷ �۽�
		for (auto it : g_Client_Socket_List)
		{
			// ��Ŷ �۽�
			SendTargetSocket(it->m_Socket, Send_Packet);
		}

		LeaveCriticalSection(&g_SCS);
	}

	return true;
}


bool Server::Recv(Packet_Header** Recv_Packet, char* MsgBuff)
{
	/// ť�� ������� FALSE�� ��ȯ�Ѵ�.
	if (Recv_Data_Queue.empty())
		return FALSE;

	/// ť���� �����͸� ���´�.
	Packet_Header* cpcHeader = nullptr;
	Recv_Data_Queue.try_pop(cpcHeader);

	/// �޼��� Ÿ������ Header ĳ����.
	C2S_Message* C2S_Msg = static_cast<C2S_Message*>(cpcHeader);

	switch (cpcHeader->Packet_Type)
	{
	case C2S_Packet_Type_Message:         // ä�� �޼���
	{
		/// ä�� �޼����� ������ MsgBuff�� �������ش�.
		//MsgBuff.resize(PACKET_BUFSIZE);
		//strcpy_s(sPacket.Message_Buffer, Msg_Buff.c_str());
		memcpy_s(MsgBuff, PACKET_BUFSIZE, C2S_Msg->Message_Buffer, PACKET_BUFSIZE);
	}
	case C2S_Packet_Type_Data:
	{
		/// ���� �ʿ�� ����..
	}
	break;
	}

	// ����.
	delete cpcHeader;

	/// ť�� ������� ������ TRUE ��ȯ.
	return TRUE;
}

bool Server::End()
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

	for (auto k : g_Client_Socket_List)
	{
		/// Ȥ�ö� �ٸ������� ���̰� ���� ���� �����ϱ�� ����
		EnterCriticalSection(&g_SCS);
		Safe_CloseSocket(k);
		LeaveCriticalSection(&g_SCS);
	}

	// ���� ����
	WSACleanup();

	printf_s("[TCP ����] ����\n");

	return true;
}

void Server::err_display(const char* const cpcMSG)
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

void Server::CreateWorkThread()
{
	SYSTEM_INFO SystemInfo;
	GetSystemInfo(&SystemInfo);
	int iThreadCount = SystemInfo.dwNumberOfProcessors * 2;

	for (int i = 0; i < iThreadCount; i++)
	{
		/// Ŭ���̾�Ʈ �۾��� �ϴ� WorkThread�� ����.
		std::thread* Client_Work = new std::thread(std::bind(&Server::WorkThread, this));

		/// ������ �����ڿ� �־��ְ�..
		g_Work_Thread.push_back(Client_Work);

		/// ���ض� �������!!
	}
}

void Server::WorkThread()
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
			printf_s("[TCP ����] �������� ����� üũ�ϴ� ���� ������ ������ ������ϴ�.\n");
			EnterCriticalSection(&g_SCS);
			Safe_CloseSocket(psSocket);
			LeaveCriticalSection(&g_SCS);

			delete psOverlapped;
			continue;
		}

		// ���� ����
		if (0 == dwNumberOfBytesTransferred)
		{
			EnterCriticalSection(&g_SCS);
			Safe_CloseSocket(psSocket);
			LeaveCriticalSection(&g_SCS);
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

int CALLBACK Server::AcceptFunction(LPWSABUF lpCallerId, LPWSABUF lpCallerData, LPQOS lpSQOS, LPQOS lpGQOS, LPWSABUF lpCalleeId, LPWSABUF lpCalleeData, GROUP FAR* g, DWORD_PTR dwCallbackData)
{
	/// ���� MAX_USER_COUNT �� 0�̸� �ִ� �������� ������ ����.
	if (MAX_USER_COUNT == 0)
	{
		// ���� ����.
		return CF_ACCEPT;
	}

	/// WSAAccept���� �޾ƿ��� ���ڰ�. (�츮�� list * �� �Ѱ����� �Ȱ��� ĳ���� ���༭ ���� �����ͷ� �����Ѵ�. )
	const std::list< std::shared_ptr<Socket_Struct> >* const cpclistClients = reinterpret_cast<std::list< std::shared_ptr<Socket_Struct> >*>(dwCallbackData);

	/// list�� ����� üũ�ϸ鼭 ���� Accept�� �޾Ƶ� �Ǵ��� �ȵǴ��� üũ�Ѵ�.
	/// �ִ� ������ ���� Ȯ���ϱ� ����.
	if (cpclistClients->size() >= MAX_USER_COUNT)
	{
		// ���� ����.
		return CF_REJECT;
	}

	// ���� ����.
	return CF_ACCEPT;
}

bool Server::AddClientSocket(std::shared_ptr<Socket_Struct> psSocket)
{
	assert(nullptr != psSocket);

	// �Ӱ� ����
	{
		// Ŭ���̾�Ʈ ������ list �� �߰�

		EnterCriticalSection(&g_SCS);

		// Ŭ���̾�Ʈ �ߺ� �˻�
		for (auto it : g_Client_Socket_List)
		{
			if (it == psSocket)
			{
				LeaveCriticalSection(&g_SCS);
				return FALSE;
			}
		}

		// Ŭ���̾�Ʈ ���
		g_Client_Socket_List.push_back(psSocket);

		LeaveCriticalSection(&g_SCS);
	}

	return TRUE;
}

bool Server::RemoveClientSocket(std::shared_ptr<Socket_Struct> psSocket)
{
	assert(INVALID_SOCKET != psSocket->m_Socket);

	// �Ӱ� ����
	{
		EnterCriticalSection(&g_SCS);

		// Ŭ���̾�Ʈ ���� �� ����
		for (auto it = g_Client_Socket_List.begin(); it != g_Client_Socket_List.end(); ++it)
		{
			if (*it == psSocket)
			{
				Safe_CloseSocket(psSocket);
				LeaveCriticalSection(&g_SCS);
				return TRUE;
			}
		}

		LeaveCriticalSection(&g_SCS);
	}

	return FALSE;
}

void Server::AcceptThread()
{
	char szBuffer[STRUCT_BUFSIZE] = { 0, };

	while (!g_Is_Exit)
	{
		/// Accept
		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);
		ZeroMemory(&clientaddr, addrlen);
		SOCKET socket = WSAAccept(*g_Listen_Socket, reinterpret_cast<SOCKADDR*>(&clientaddr), &addrlen, AcceptFunction, reinterpret_cast<DWORD_PTR>(&g_Client_Socket_List));

		if (INVALID_SOCKET == socket)
		{
			// �������� ���� �ź�( AcceptCondition() )�� �� ���� ���� ����� ���ϰ� ��
			if (WSAGetLastError() != WSAECONNREFUSED)
			{
				sprintf_s(szBuffer, "[TCP ����] ���� �߻� -- WSAAccept() :");
				err_display(szBuffer);
			}

			continue;
		}

		// Ŭ���̾�Ʈ ����ü ���� �� ���
		std::shared_ptr<Socket_Struct> psSocket = std::make_shared<Socket_Struct>();
		psSocket->m_Socket = socket;
		psSocket->IP = inet_ntoa(clientaddr.sin_addr);
		psSocket->PORT = ntohs(clientaddr.sin_port);

		/// IOCP�� ������ �ڵ�� ���� ����Ѵ�.
		if (CreateIoCompletionPort(reinterpret_cast<HANDLE>(psSocket->m_Socket), g_IOCP, reinterpret_cast<ULONG_PTR>(psSocket.get()), 0) != g_IOCP)
		{
			// ���� �ÿ� ���� ������ ������ �����Ǿ�� �Ѵ�.
			psSocket.reset();
			continue;
		}

		// ����� Ŭ���̾�Ʈ ������ ���
		if (!AddClientSocket(psSocket))
		{
			psSocket.reset();
			continue;
		}

		printf_s("[TCP ����] [%15s:%5d] Ŭ���̾�Ʈ�� ����\n", psSocket->IP.c_str(), psSocket->PORT);

		/// WSARecv�� �ɾ�־� ������ ���� �� �ְ���?!
		if (!Reserve_WSAReceive(psSocket->m_Socket))
		{
			// Ŭ���̾�Ʈ ����
			RemoveClientSocket(psSocket);
			continue;
		}
	}

}

void Server::ExitThread()
{
	while (TRUE)
	{
		if (GetAsyncKeyState(VK_ESCAPE))
		{
			g_Is_Exit = true;

			/// ���������� ���ǰ� ���� �ʴٸ�..
			if (INVALID_SOCKET != *g_Listen_Socket)
			{
				/// �����غ��ϱ� ��� ��� �����ϰ� �ִ� Accept���� ���������� ���ѰŴϱ�..
				/// unique�ϋ� ���� �ʿ䰡 ���ڳ�?
				shutdown(*g_Listen_Socket, SD_BOTH);
				closesocket(*g_Listen_Socket);
				g_Listen_Socket.reset();
			}

			break;
		}
	}
}

void Server::Safe_CloseSocket(std::shared_ptr<Socket_Struct> psSocket)
{
	/// Critical Section�� Ŭ���̾�Ʈ ������ ���ﶩ �Լ��� ȣ���ϱ����� �̹� �ɷ��ִ� ���°� �ƴұ�?
	auto it = g_Client_Socket_List.begin();

	for (it; it != g_Client_Socket_List.end(); it++)
	{
		/// �ش��ϴ� �����͸� ����ִ� shared_ptr�� ã�´�.
		if (*it == psSocket)
		{
			break;
		}
	}

	while (!psSocket.unique())
	{
		/// �ٸ� ���� �����ǰ� �ִ� �κ��� �ִٸ�, ���ᰡ �� ������ ��ٸ���.
		Sleep(0);
	}

	if (psSocket.unique())
	{
		shutdown(psSocket->m_Socket, SD_BOTH);
		closesocket(psSocket->m_Socket);
		psSocket->m_Socket = INVALID_SOCKET;
		psSocket->Is_Connected = FALSE;

		printf_s("[TCP ����] [%15s:%5d] Ŭ���̾�Ʈ�� ����\n", psSocket->IP.c_str(), psSocket->PORT);
	}

	/// �ش��ϴ� Ŭ���̾�Ʈ ������ ����Ʈ���� ����.
	g_Client_Socket_List.erase(it);
}

void Server::Safe_CloseSocket(Socket_Struct* psSocket)
{
	auto it = g_Client_Socket_List.begin();

	for (it; it != g_Client_Socket_List.end(); it++)
	{
		/// �ش��ϴ� �����͸� ����ִ� shared_ptr�� ã�´�.
		if (it->get() == psSocket)
		{
			break;
		}
	}

	while (!it->unique())
	{
		/// �ٸ� ���� �����ǰ� �ִ� �κ��� �ִٸ�, ���ᰡ �� ������ ��ٸ���.
		Sleep(0);
	}

	if (it->unique())
	{
		shutdown(psSocket->m_Socket, SD_BOTH);
		closesocket(psSocket->m_Socket);
		psSocket->m_Socket = INVALID_SOCKET;
		psSocket->Is_Connected = FALSE;

		printf_s("[TCP ����] [%15s:%5d] Ŭ���̾�Ʈ�� ����\n", psSocket->IP.c_str(), psSocket->PORT);
	}

	/// �ش��ϴ� Ŭ���̾�Ʈ ������ ����Ʈ���� ����.
	g_Client_Socket_List.erase(it);
}

bool Server::Reserve_WSAReceive(SOCKET socket, Overlapped_Struct* psOverlapped /* = nullptr */)
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
		sprintf_s(szBuffer, "[TCP ����] ���� �߻� -- WSARecv() :");
		err_display(szBuffer);

		if (!bRecycleOverlapped)
		{
			delete psOverlapped;
		}
		return FALSE;
	}

	return TRUE;
}

bool Server::SendTargetSocket(SOCKET socket, Packet_Header* psPacket)
{
	assert(INVALID_SOCKET != socket);
	assert(nullptr != psPacket);

	// ��ϵ��� ���� ��Ŷ�� ������ �� ����.
	// [����] -> [Ŭ��]
	if (S2C_Packet_Type_MAX <= psPacket->Packet_Type)
	{
		return FALSE;
	}

	// �������� ����
	Overlapped_Struct* psOverlapped = new Overlapped_Struct;
	psOverlapped->m_IOType = Overlapped_Struct::IOType::IOType_Send;
	psOverlapped->m_Socket = socket;

	// ��Ŷ ����
	psOverlapped->m_Data_Size = 2 + psPacket->Packet_Size;

	if (sizeof(psOverlapped->m_Buffer) < psOverlapped->m_Data_Size)
	{
		delete psOverlapped;
		return FALSE;
	}

	memcpy_s(psOverlapped->m_Buffer, sizeof(psOverlapped->m_Buffer), psPacket, psOverlapped->m_Data_Size);

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
		char szBuffer[STRUCT_BUFSIZE] = { 0, };
		sprintf_s(szBuffer, "[TCP ����] ���� �߻� -- WSASend() :");
		err_display(szBuffer);

		delete psOverlapped;
		return FALSE;
	}

	return TRUE;
}

void Server::IOFunction_Recv(DWORD dwNumberOfBytesTransferred, Overlapped_Struct* psOverlapped, Socket_Struct* psSocket)
{
	printf_s("[TCP ����] [%15s:%5d] ��Ŷ ���� �Ϸ� <- %d ����Ʈ\n", psSocket->IP.c_str(), psSocket->PORT, dwNumberOfBytesTransferred);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	 // ��Ŷ ó��
	 /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// �ϼ��� ��Ŷ�� ó��
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Ŭ���̾�Ʈ�κ��� ������ ��Ŷ
		Packet_Header* cpcHeader = reinterpret_cast<Packet_Header*>(malloc(usPacketSize));
		memcpy_s(cpcHeader, usPacketSize, psOverlapped->m_Buffer, usPacketSize);

		// �߸��� ��Ŷ
		if (C2S_Packet_Type_MAX <= cpcHeader->Packet_Type)
		{
			// Ŭ���̾�Ʈ ����
			EnterCriticalSection(&g_SCS);
			Safe_CloseSocket(psSocket);
			LeaveCriticalSection(&g_SCS);
			delete psOverlapped;
			return;
		}

		Recv_Data_Queue.push(cpcHeader);

		// �����͵��� �̹��� ó���Ѹ�ŭ ����.
		memcpy_s(psOverlapped->m_Buffer, psOverlapped->m_Data_Size,
			psOverlapped->m_Buffer + usPacketSize, psOverlapped->m_Data_Size - usPacketSize);

		// ó���� ��Ŷ ũ�⸸ŭ ó���ҷ� ����
		psOverlapped->m_Data_Size -= usPacketSize;
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// ���� �ɱ�( �̹��� ����� �������带 �ٽ� ��� )
	if (!Reserve_WSAReceive(psOverlapped->m_Socket, psOverlapped))
	{
		// Ŭ���̾�Ʈ ����
		EnterCriticalSection(&g_SCS);
		Safe_CloseSocket(psSocket);
		LeaveCriticalSection(&g_SCS);
		delete psOverlapped;
		return;
	}
}

void Server::IOFunction_Send(DWORD dwNumberOfBytesTransferred, Overlapped_Struct* psOverlapped, Socket_Struct* psSocket)
{
	printf_s("[TCP ����] [%15s:%5d] ��Ŷ �۽� �Ϸ� -> %d ����Ʈ\n", psSocket->IP.c_str()
		, psSocket->PORT, dwNumberOfBytesTransferred);

	delete psOverlapped;
}
