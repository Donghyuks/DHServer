#include "DHClient.h"

DHClient::DHClient()
{
	/// CS �ʱ�ȭ.
	InitializeCriticalSection(&g_CCS);

	TCHAR Error_Buffer[ERROR_MSG_BUFIZE] = { 0, };

	/// ������ ����ü�� �����Ѵ�.
	g_Server_Socket = std::make_shared<Socket_Struct>();

	/// IOCP�� �����Ѵ�.
	g_IOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	assert(nullptr != g_IOCP);

	/// ��밡���� Overlapped�� �����صд�. (100���� �����ص�)
	Available_Overlapped = new ObjectPool<Overlapped_Struct>(CLIENT_OVERLAPPED_COUNT);

	/// CLIENT_THREAD_COUNT ������ŭ WorkThread�� �����Ѵ�.
	CreateWorkThread();
}

DHClient::~DHClient()
{
	/// Ŭ���̾�Ʈ�� ����� �� ���� CS�� ����.
	DeleteCriticalSection(&g_CCS);
	g_Server_Socket.reset();
}

BOOL DHClient::Send(Packet_Header* Send_Packet, SOCKET _Socket /*= INVALID_SOCKET*/)
{
	assert(nullptr != Send_Packet);
	if (INVALID_SOCKET == g_Server_Socket->m_Socket)
	{
		return LOGIC_FAIL;
	}

	// ��ϵ��� ���� ��Ŷ�� ������ �� ����.
	// [Ŭ��] -> [����]
	if (C2S_Packet_Type_None <= Send_Packet->Packet_Type)
	{
		return LOGIC_FAIL;
	}

	// ���� �޼����� ť�� �־ �ѹ��� ���� ( �ִ��� ȿ�������� �ڿ��� ����ϱ� ���� )
	Send_Data_Queue.push(Send_Packet);

	return LOGIC_SUCCESS;
}

BOOL DHClient::Recv(std::vector<Network_Message>& _Message_Vec)
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
		_Message_Vec.push_back(*_Net_Msg);

		// ����.
		delete _Net_Msg;
	}

	/// ť�� �����͸� �� ���� TRUE ��ȯ.
	return TRUE;
}

BOOL DHClient::Connect(unsigned short _Port, std::string _IP)
{
	if (g_Connect_Send_Client_Thread != nullptr)
	{
		return Is_Server_Connect_Success;
	}

	/// �ش� ��Ʈ�� IP ����.
	PORT = _Port; IP = _IP;
	/// Ŭ���̾�Ʈ�� ��������� ������ ������ ����. (������ �õ��� ��� �ϱ����ؼ�)
	g_Connect_Send_Client_Thread = new std::thread(std::bind(&DHClient::ConnectSendThread, this));

	return false;
}

BOOL DHClient::End()
{
	PostQueuedCompletionStatus(g_IOCP, 0, 0, nullptr);
	g_Is_Exit = true;

	// Send & Connect ������ ����.
	g_Connect_Send_Client_Thread->join();

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

	// �������� ����
	delete Available_Overlapped;

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
			Safe_CloseSocket();
			printf_s("[TCP Ŭ���̾�Ʈ] �������� ����� üũ�ϴ� ���� ������ ������ ������ϴ�.\n");
			Is_Server_Connect_Success = false;
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

				Available_Overlapped->ResetObject(psOverlapped);
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

void DHClient::ConnectSendThread()
{
	while (!g_Is_Exit)
	{
		/// ���� �̹� ������ ������ �Ǿ��ִٸ�
		if (Is_Server_Connect_Success)
		{
			/// Connect�� �� �����带 Send�ϴ� ������ν� ��Ȱ���Ѵ�. (���ʿ��� ������ ���� ���̱�!)
			SendFunction();
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
				TCHAR szBuffer[ERROR_MSG_BUFIZE] = { 0, };
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
		if (CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_Server_Socket->m_Socket), g_IOCP, reinterpret_cast<ULONG_PTR>(g_Server_Socket.get()), CLIENT_IOCP_THREAD_COUNT) != g_IOCP)
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

		/// �ΰ��� ���������� Socket�� ���� ������������ �Ͼ ��찡 �������� ������?! Ȯ���� ����������ٰ�.
		Is_Server_Connect_Success = true;

		printf_s("[TCP Ŭ���̾�Ʈ] [%15s:%d] [SOCKET : %d] ���� ���� ����\n", IP.c_str(), PORT, g_Server_Socket->m_Socket);

	}
}

void DHClient::SendFunction()
{
	// ���� ��Ŷ�� �� ������ (��� + ���� ���ۿ� ����ִ� ��Ŷ ������)
	size_t Total_Packet_Size = 0;
	// Buffer�� ��ŭ ��ġ���� �߶���ϴ��� ��Ÿ��������.
	size_t Buff_Offset = 0;

	// ���� �޼����� �ִ� ť���� �����Ͱ� �ִ��� �˻�
	while (!Send_Data_Queue.empty())
	{
		if (g_Server_Socket->m_Socket == INVALID_SOCKET)
		{
			break;
		}

		Packet_Header* Send_Packet = nullptr;

		// ť���� ���� �����͸� ���´�.
		Send_Data_Queue.try_pop(Send_Packet);

		// �ʱ� ����.
		Buff_Offset = 0;
		Total_Packet_Size = PACKET_HEADER_SIZE + Send_Packet->Packet_Size;

		// ���� ��Ŷ�� ����� �غ�� ���ۺ��� ũ�ٸ� �߶� �������� �����ش�.
		while (Total_Packet_Size > 0)
		{
			// �������� ����
			Overlapped_Struct* psOverlapped = Available_Overlapped->GetObject();
			psOverlapped->m_IOType = Overlapped_Struct::IOType::IOType_Send;
			psOverlapped->m_Socket = g_Server_Socket->m_Socket;

			// ��Ŷ ����
			// ��Ŷ�� ���� �������� ���ۺ��� ����� ū ���
			if (Total_Packet_Size >= OVERLAPPED_BUFIZE)
			{
				psOverlapped->m_Data_Size = OVERLAPPED_BUFIZE;
				Total_Packet_Size -= OVERLAPPED_BUFIZE;
				memcpy_s(psOverlapped->m_Buffer, OVERLAPPED_BUFIZE, (char*)Send_Packet + (Buff_Offset * OVERLAPPED_BUFIZE), OVERLAPPED_BUFIZE);
			}
			// �������� ���ۺ��� ����� ���� ���
			else
			{
				psOverlapped->m_Data_Size = Total_Packet_Size;
				memcpy_s(psOverlapped->m_Buffer, OVERLAPPED_BUFIZE, (char*)Send_Packet + (Buff_Offset * OVERLAPPED_BUFIZE), Total_Packet_Size);
				Total_Packet_Size = 0;
			}


			Buff_Offset++;

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
				TCHAR szBuffer[ERROR_MSG_BUFIZE] = { 0, };
				_stprintf_s(szBuffer, _countof(szBuffer), _T("[TCP Ŭ���̾�Ʈ] ���� �߻� -- WSASend() :"));
				err_display(szBuffer);
				Safe_CloseSocket();
				Available_Overlapped->ResetObject(psOverlapped);
				break;
			}

		}
	}

	// ���� �޼����� �������� �ٸ� �����忡�� �������� �Ѱ���.
	Sleep(0);
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

	int Exit_Socket_Num = g_Server_Socket->m_Socket;

	shutdown(g_Server_Socket->m_Socket, SD_BOTH);
	closesocket(g_Server_Socket->m_Socket);
	g_Server_Socket->m_Socket = INVALID_SOCKET;
		
	LeaveCriticalSection(&g_CCS);

	printf_s("[TCP Ŭ���̾�Ʈ] [%15s:%d] [SOCKET : %d] ���� ���� ����\n", IP.c_str(), PORT, Exit_Socket_Num);

	Is_Server_Connect_Success = false;
}

bool DHClient::Reserve_WSAReceive(SOCKET socket, Overlapped_Struct* psOverlapped /* = nullptr */)
{
	assert(INVALID_SOCKET != socket);

	BOOL bRecycleOverlapped = TRUE;

	// ����� �������带 ���� �ʾ����� ����
	if (nullptr == psOverlapped)
	{
		psOverlapped = Available_Overlapped->GetObject();
		bRecycleOverlapped = FALSE;
	}

	// �������� ����
	psOverlapped->m_IOType = Overlapped_Struct::IOType::IOType_Recv;
	psOverlapped->m_Socket = socket;

	// WSABUF ����
	WSABUF wsaBuffer;
	wsaBuffer.buf = psOverlapped->m_Buffer;
	wsaBuffer.len = sizeof(psOverlapped->m_Buffer);

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
		TCHAR szBuffer[ERROR_MSG_BUFIZE] = { 0, };
		_stprintf_s(szBuffer, _countof(szBuffer), _T("[TCP Ŭ���̾�Ʈ] ���� �߻� -- WSARecv() :"));
		err_display(szBuffer);

		if (!bRecycleOverlapped)
		{
			Available_Overlapped->ResetObject(psOverlapped);
		}
		return FALSE;
	}

	return TRUE;
}

void DHClient::IOFunction_Recv(DWORD dwNumberOfBytesTransferred, Overlapped_Struct* psOverlapped, Socket_Struct* psSocket)
{
	printf_s("[TCP Ŭ���̾�Ʈ] [%d Byte] ��Ŷ ���� �Ϸ�\n", dwNumberOfBytesTransferred);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// ��Ŷ ó��
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// ������ �����͵� ũ�⸦ ���� �����ش�.
	psOverlapped->m_Data_Size += dwNumberOfBytesTransferred;

	// ó���� �����Ͱ� ������ ó��
	while (psOverlapped->m_Data_Size > 0)
	{
		// header ũ��� 2 ����Ʈ( ���� )
		static const unsigned short cusHeaderSize = PACKET_HEADER_SIZE;

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

		Available_Overlapped->ResetObject(psOverlapped);
		return;
	}
}

void DHClient::IOFunction_Send(DWORD dwNumberOfBytesTransferred, Overlapped_Struct* psOverlapped, Socket_Struct* psSocket)
{
	printf_s("[TCP Ŭ���̾�Ʈ] [%15s:%d] [SOCKET : %d] [%d Byte] ��Ŷ �۽� �Ϸ�\n", IP.c_str(), PORT, (int)psOverlapped->m_Socket, dwNumberOfBytesTransferred);

	Available_Overlapped->ResetObject(psOverlapped);
}
