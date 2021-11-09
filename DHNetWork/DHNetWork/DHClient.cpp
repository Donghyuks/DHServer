#include "DHClient.h"

DHClient::DHClient()
{

}

DHClient::~DHClient()
{
	/// Ŭ���̾�Ʈ�� ����� �� ���� CS�� ����.
	DeleteCriticalSection(&g_CCS);
	g_Server_Socket.reset();
}

bool DHClient::Start()
{
	/// CS �ʱ�ȭ.
	InitializeCriticalSection(&g_CCS);

	TCHAR Error_Buffer[MSG_BUFSIZE] = { 0, };

	/// ������ ����ü�� �����Ѵ�.
	g_Server_Socket = std::make_shared<Socket_Struct>();

	/// IOCP�� �����Ѵ�.
	g_IOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	assert(nullptr != g_IOCP);

	/// Ŭ���̾�Ʈ�� ���Ḧ üũ�ϱ� ���� ������ ����.
	//g_Exit_Check_Thread = new std::thread(std::bind(&DHClient::ExitThread, this));

	/// CLIENT_THREAD_COUNT ������ŭ WorkThread�� �����Ѵ�.
	CreateWorkThread();

	printf_s("[TCP Ŭ���̾�Ʈ] ����\n");

	return LOGIC_SUCCESS;
}

bool DHClient::Send(Packet_Header* Send_Packet)
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
		psOverlapped,
		nullptr);

	if ((SOCKET_ERROR == iResult) && (WSAGetLastError() != WSA_IO_PENDING))
	{
		/// TCHAR�� ���� �����ڵ�/��Ƽ����Ʈ�� ������ ��Ȳ�� ���׸��ϰ� ������ �� �ֵ��� �Ѵ�.
		TCHAR szBuffer[MSG_BUFSIZE] = { 0, };
		_stprintf_s(szBuffer, _countof(szBuffer), _T("[TCP Ŭ���̾�Ʈ] ���� �߻� -- WSASend() :"));
		err_display(szBuffer);

		Safe_CloseSocket();
		delete psOverlapped;
	}

	return TRUE;
}

bool DHClient::Recv(std::vector<Network_Message*>& _Message_Vec)
{
	/// ť�� ������� FALSE�� ��ȯ�Ѵ�.
	if (Recv_Data_Queue.empty())
		return FALSE;

	/// ť�� ����Ǿ��ִ� ��� �޼����� ��Ƽ� ��ȯ.
	while (!Recv_Data_Queue.empty())
	{
		/// ť���� �����͸� ���´�.
		Network_Message* _Net_Msg = nullptr;
		Recv_Data_Queue.try_pop(_Net_Msg);

		/// ���� �����͸� �־ ����.
		_Message_Vec.emplace_back(_Net_Msg);
		/// �޼��� Ÿ������ Header ĳ����.
		//C2S_Message* C2S_Msg = static_cast<C2S_Message*>(cpcHeader);

		//switch (cpcHeader->Packet_Type)
		//{
		//case C2S_Packet_Type_Message:         // ä�� �޼���
		//{
		//	/// ä�� �޼����� ������ MsgBuff�� �������ش�.
		//	memcpy_s(MsgBuff, MSG_BUFSIZE, C2S_Msg->Message_Buffer, MSG_BUFSIZE);
		//}
		//case C2S_Packet_Type_Data:
		//{
		//	/// ���� �ʿ�� ����..
		//}
		//break;
		//}

		// ����.
		//delete cpcHeader;
	}

	/// ť�� �����͸� �� ���� TRUE ��ȯ.
	return TRUE;
}

bool DHClient::Connect(unsigned short _Port, std::string _IP)
{
	if (g_Connect_Client_Thread != nullptr)
	{
		return Is_Server_Connect_Success;
	}

	/// �ش� ��Ʈ�� IP ����.
	PORT = _Port; IP = _IP;
	/// Ŭ���̾�Ʈ�� ��������� ������ ������ ����. (������ �õ��� ��� �ϱ����ؼ�)
	g_Connect_Client_Thread = new std::thread(std::bind(&DHClient::ConnectThread, this));

	return false;
}

bool DHClient::End()
{
	PostQueuedCompletionStatus(g_IOCP, 0, 0, nullptr);
	g_Is_Exit = true;

	//	Connect ������ ����.
	g_Connect_Client_Thread->join();

	for (auto k : g_Work_Thread)
	{
		k->join();
	}

	g_Work_Thread.clear();

	// IOCP ����
	CloseHandle(g_IOCP);
	g_IOCP = nullptr;

	// ���� ����
	Safe_CloseSocket();
	WSACleanup();

	printf_s("[TCP Ŭ���̾�Ʈ] ����\n");

	return true;
}

void DHClient::CreateWorkThread()
{
	for (int i = 0; i < CLIENT_THREAD_COUNT; i++)
	{
		/// Ŭ���̾�Ʈ �۾��� �ϴ� WorkThread�� ����.
		std::thread* Client_Work = new std::thread(std::bind(&DHClient::WorkThread, this));

		/// ������ �����ڿ� �־��ְ�..
		g_Work_Thread.push_back(Client_Work);

		/// ���ض� �������!!
	}
}

void DHClient::WorkThread()
{
	assert(nullptr != g_IOCP);

	while (true)
	{
		DWORD        dwNumberOfBytesTransferred = 0;
		Socket_Struct* psSocket = nullptr;
		Overlapped_Struct* psOverlapped = nullptr;

		/// GetQueuedCompletionStatusEx �� ���� �Ǹ鼭 ���� �ʿ��� �κ�.
		OVERLAPPED_ENTRY Entry_Data[64];	// ��� �� ��Ʈ���� �����͵�. �ִ� 64���� �����ص�.
		ULONG Entry_Count = sizeof(Entry_Data) / sizeof(OVERLAPPED_ENTRY);	// ������ �������� �ִ� ���� ( 64�� )
		ULONG Get_Entry_Count = 0;	// ������ �� ���� ��Ʈ���� ���� �Դ��� ����ϱ� ���� ����.

		// GetQueuedCompletionStatus() - GQCS ��� �θ�
		// WSARead(), WSAWrite() ���� Overlapped I/O ���� ó�� ����� �޾ƿ��� �Լ�
		// PostQueuedCompletionStatus() �� ���ؼ��� GQCS �� ���Ͻ�ų �� �ִ�.( �Ϲ������� ������ ���� ó�� )

		/// GetQueuedCompletionStatusEx �Լ� ����.
		/// 1. HANDLE hCompletionPort : � I/O ���ø��� ��Ʈ�� ����� �������� �����ϴ� �ڵ� ���� ����. ( I/O ���ø��� ť�� ���� ���� �׸��� ���ԵǾ� ���� ��, ���� ���� �׸���� �� ���� ������ �� �ִ�. )
		/// 2. LPOVERLAPPED_ENTRY pCompletionPortEntries : �� �׸���� pCompletionPortEntries �迭�� ���� ���� �ȴ�.
		/// 3. ULONG ulCount : �� ���� �׸��� pCompletionPortEntries�� ������ �� �������� �����ϴ� ���� �����ϸ� �ȴ�.
		/// 4. PULONG pulNumEntriesRemoved : long ������ I/O ���ø��� ť�κ��� �� ���� �׸���� ������ ������ �Դ����� �޾ƿ��� �ȴ�. bAlertable �� FALSE�� �����Ǿ��ٸ� ������ �ð���ŭ I/O ���ø��� ť�� �Ϸ� ������ ���Ե� ������ ����ϰ� �ȴ�.
		///										���� TRUE�� �����Ǿ� �ֵ��� I/O ���ø��� ť�� ��� �Ϸ� ������ �������� �ʴ� ��� �����带 Alertable ���·� ��ȯ�Ѵ�.
		/// 5. DWORD dwMilliseconds : ��ٸ� �ð�
		/// 6. BOOL bAlertable : Alertable ���¿� ���� ó���� 4 �׸��� ����.

		BOOL bSuccessed = GetQueuedCompletionStatusEx(g_IOCP,
			Entry_Data,
			Entry_Count,
			&Get_Entry_Count,
			INFINITE,
			0);

		// �������� ��� üũ
		if (!bSuccessed)
		{
			if (SOCKET_ERROR != shutdown(psOverlapped->m_Socket, SD_BOTH))
			{
				Safe_CloseSocket();
				printf_s("[TCP Ŭ���̾�Ʈ] �������� ����� üũ�ϴ� ���� ������ ������ ������ϴ�.\n");
				Is_Server_Connect_Success = false;
			}

			delete psOverlapped;
			continue;
		}

		/// ������ �޾ƿ� Entry �����Ϳ� ���Ͽ�
		for (ULONG i = 0; i < Get_Entry_Count; i++)
		{
			/// WSARead() / WSAWrite() � ���� Overlapped �����͸� �����´�.
			psOverlapped = reinterpret_cast<Overlapped_Struct*>(Entry_Data[i].lpOverlapped);
			/// I/O�� ���� �������� ũ��.
			dwNumberOfBytesTransferred = Entry_Data[i].dwNumberOfBytesTransferred;
			/// Key������ �Ѱ������ Socket_Struct ������.
			psSocket = reinterpret_cast<Socket_Struct*>(Entry_Data[i].lpCompletionKey);

			// Ű�� nullptr �� ��� ������ ���Ḧ �ǹ�
			if (!psSocket)
			{
				// �ٸ� WorkerThread() �� ���Ḧ ���ؼ� PostQueue�� ȣ�����ְ� 
				// ������ Thread�� �����Ѵ�.
				PostQueuedCompletionStatus(g_IOCP, 0, 0, nullptr);
				return;
			}

			assert(nullptr != psOverlapped);

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

			/// Overlapped �����Ͱ� ���� ��
			if (psOverlapped)
			{
				// Overlapped I/O ó��
				switch (psOverlapped->m_IOType)
				{
				case Overlapped_Struct::IOType::IOType_Recv: IOFunction_Recv(dwNumberOfBytesTransferred, psOverlapped, psSocket); break; // WSARecv() �� Overlapped I/O �Ϸῡ ���� ó��
				case Overlapped_Struct::IOType::IOType_Send: IOFunction_Send(dwNumberOfBytesTransferred, psOverlapped, psSocket); break; // WSASend() �� Overlapped I/O �Ϸῡ ���� ó��
				}
			}
		}
	}
}

void DHClient::ConnectThread()
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
				/// TCHAR�� ���� �����ڵ�/��Ƽ����Ʈ�� ������ ��Ȳ�� ���׸��ϰ� ������ �� �ֵ��� �Ѵ�.
				TCHAR szBuffer[MSG_BUFSIZE] = { 0, };
				_stprintf_s(szBuffer, _countof(szBuffer), _T("[TCP Ŭ���̾�Ʈ] ���� �߻� -- WSASocket() :"));
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
			/// IPv4 ����� address ��������.
			inet_pton(AF_INET, IP.c_str(), &(serveraddr.sin_addr.s_addr));

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

		/// ������ �Ǿ��ٴ� flag
		g_Server_Socket->Is_Connected = TRUE;
		/// �ΰ��� ���������� Socket�� ���� ������������ �Ͼ ��찡 �������� ������?! Ȯ���� ����������ٰ�.
		Is_Server_Connect_Success = true;
		printf_s("[TCP Ŭ���̾�Ʈ] ������ ���� �Ϸ�\n");

	}
}

void DHClient::ExitThread()
{
	while (TRUE)
	{
		if (GetAsyncKeyState(VK_ESCAPE))
		{
			g_Is_Exit = true;
			End();
			break;
		}

		Sleep(0);
	}
}

void DHClient::Safe_CloseSocket()
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

bool DHClient::Reserve_WSAReceive(SOCKET socket, Overlapped_Struct* psOverlapped /* = nullptr */)
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
		psOverlapped,
		nullptr);

	if ((SOCKET_ERROR == iResult) && (WSAGetLastError() != WSA_IO_PENDING))
	{
		/// TCHAR�� ���� �����ڵ�/��Ƽ����Ʈ�� ������ ��Ȳ�� ���׸��ϰ� ������ �� �ֵ��� �Ѵ�.
		TCHAR szBuffer[MSG_BUFSIZE] = { 0, };
		_stprintf_s(szBuffer, _countof(szBuffer), _T("[TCP Ŭ���̾�Ʈ] ���� �߻� -- WSARecv() :"));
		err_display(szBuffer);

		if (!bRecycleOverlapped)
		{
			delete psOverlapped;
		}
		return FALSE;
	}

	return TRUE;
}

void DHClient::IOFunction_Recv(DWORD dwNumberOfBytesTransferred, Overlapped_Struct* psOverlapped, Socket_Struct* psSocket)
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

		Network_Message* Net_Msg = new Network_Message;
		Net_Msg->Socket = psSocket->m_Socket;
		Net_Msg->Packet = cpcHeader;

		// ������ ��Ŷ�� ť�� �־�ΰ� ���߿� ó���Ѵ�.
		Recv_Data_Queue.push(Net_Msg);

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

void DHClient::IOFunction_Send(DWORD dwNumberOfBytesTransferred, Overlapped_Struct* psOverlapped, Socket_Struct* psSocket)
{
	printf_s("[TCP Ŭ���̾�Ʈ][SOCKET %d] ��Ŷ �۽� �Ϸ� -> %d ����Ʈ\n", (int)psOverlapped->m_Socket,dwNumberOfBytesTransferred);

	delete psOverlapped;
}
