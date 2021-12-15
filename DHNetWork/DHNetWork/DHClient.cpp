#include "DHClient.h"

DHClient::DHClient()
{
	/// CS �ʱ�ȭ.
	InitializeCriticalSection(&g_CCS);

	TCHAR Error_Buffer[ERROR_MSG_BUFIZE] = { 0, };

	/// ������ ����ü�� �����Ѵ�.
	g_Server_Socket = std::make_shared<Socket_Struct>();

	/// IOCP�� �����Ѵ�.
	g_IOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, CLIENT_IOCP_THREAD_COUNT);
	assert(nullptr != g_IOCP);

	/// ��밡���� Overlapped�� �����صд�. (100���� �����ص�)
	Available_Overlapped = new ObjectPool<Overlapped_Struct>(CLIENT_OVERLAPPED_COUNT);

	/// CLIENT_THREAD_COUNT ������ŭ WorkThread�� �����Ѵ�.
	CreateWorkThread();
}

DHClient::~DHClient()
{
	if (g_IOCP != nullptr)
	{
		End();
	}
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

	// ���� ��Ŷ�� �����ؼ� Send ť�� ����.
	size_t Send_Packet_Total_Size = PACKET_HEADER_SIZE + Send_Packet->Packet_Size;
	Packet_Header* Copy_Packet = reinterpret_cast<Packet_Header*>(malloc(Send_Packet_Total_Size));
	memcpy_s(Copy_Packet, Send_Packet_Total_Size, Send_Packet, Send_Packet_Total_Size);

	// ���� �޼����� ť�� �־ �ѹ��� ���� ( �ִ��� ȿ�������� �ڿ��� ����ϱ� ���� )
	Send_Data_Queue.push(Copy_Packet);

	return LOGIC_SUCCESS;
}

BOOL DHClient::Recv(std::vector<Network_Message>& _Message_Vec)
{
	// ť�� ������� FALSE�� ��ȯ�Ѵ�.
	if (Recv_Data_Queue.empty())
		return LOGIC_FAIL;

	// ť���� �����͸� ���´�.
	Network_Message* _Net_Msg = nullptr;

	// ť�� ����Ǿ��ִ� ��� �޼����� ��Ƽ� ��ȯ.
	while (!Recv_Data_Queue.empty())
	{
		Recv_Data_Queue.try_pop(_Net_Msg);
		// ���� �����͸� �־ ����.
		_Message_Vec.push_back(*_Net_Msg);

		// ����.
		delete _Net_Msg->Packet;
		delete _Net_Msg;
	}

	// ť�� �����͸� �� ���� TRUE ��ȯ.
	return LOGIC_SUCCESS;
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
	delete g_Connect_Send_Client_Thread;

	for (auto k : g_Work_Thread)
	{
		k->join();
		delete k;
	}

	g_Work_Thread.clear();

	// Send/Recv ť �ʱ�ȭ.
	while (!Recv_Data_Queue.empty())
	{
		Network_Message* _Msg = nullptr;
		Recv_Data_Queue.try_pop(_Msg);
		delete _Msg->Packet;
		delete _Msg;
	}
	while (!Send_Data_Queue.empty())
	{
		Packet_Header* _Packet_Data = nullptr;
		Send_Data_Queue.try_pop(_Packet_Data);
		delete _Packet_Data;
	}

	// IOCP ����
	CloseHandle(g_IOCP);
	g_IOCP = nullptr;

	// ���� ����
	Safe_CloseSocket();
	WSACleanup();

	// �������� ����
	delete Available_Overlapped;

	/// Ŭ���̾�Ʈ�� ����� �� ���� CS�� ����.
	DeleteCriticalSection(&g_CCS);
	g_Server_Socket.reset();

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

		/// �ΰ��� ���������� Socket�� ���� ������������ �Ͼ ��찡 �������� ������?! Ȯ���� ����������ٰ�.
		Is_Server_Connect_Success = true;

		printf_s("[TCP Ŭ���̾�Ʈ] [%15s:%d] [SOCKET : %d] ���� ���� ����\n", IP.c_str(), PORT, (int)g_Server_Socket->m_Socket);

	}
}

void DHClient::SendFunction()
{
	// ���� ��Ŷ�� �� ������ (��� + ���� ���ۿ� ����ִ� ��Ŷ ������)
	size_t Total_Packet_Size = 0;
	// Buffer�� ��ŭ ��ġ���� �߶���ϴ��� ��Ÿ��������.
	size_t Buff_Offset = 0;
	// �����͸� Queue ���� ������ ���� ����.
	Packet_Header* Send_Packet = nullptr;

	// ���� �޼����� �ִ� ť���� �����Ͱ� �ִ��� �˻�
	while (!Send_Data_Queue.empty())
	{
		if (g_Server_Socket->m_Socket == INVALID_SOCKET)
		{
			break;
		}

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
			psOverlapped->m_WSABUF.len = psOverlapped->m_Data_Size;

			// WSASend() �������� �ɱ�
			DWORD dwNumberOfBytesSent = 0;

			int iResult = WSASend(psOverlapped->m_Socket,
				&psOverlapped->m_WSABUF,
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
		// �� ó���� ������ ����.
		delete Send_Packet;
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

	int Exit_Socket_Num = (int)g_Server_Socket->m_Socket;

	shutdown(g_Server_Socket->m_Socket, SD_BOTH);
	closesocket(g_Server_Socket->m_Socket);
	g_Server_Socket->m_Socket = INVALID_SOCKET;
		
	LeaveCriticalSection(&g_CCS);
	
	printf_s("[TCP Ŭ���̾�Ʈ] [%15s:%d] [SOCKET : %d] ���� ���� ����\n", IP.c_str(), PORT, Exit_Socket_Num);

	Is_Server_Connect_Success = false;
}

bool DHClient::BackUp_Overlapped(Overlapped_Struct* psOverlapped)
{
	if ((psOverlapped->m_Processed_Packet_Size + psOverlapped->m_Data_Size) > MAX_PACKET_SIZE)
	{
		printf_s("[TCP Ŭ���̾�Ʈ] �����κ��� [%d]Byte �� �ʰ��ϴ� ��Ŷ ó���� ���Խ��ϴ�. \n", MAX_PACKET_SIZE);
		assert(LOGIC_FAIL);

		return false;
	}

	// ���� ���� �����Ϳ� ���� ����� �س��´�.
	memcpy_s(psOverlapped->m_Processing_Packet_Buffer + psOverlapped->m_Processed_Packet_Size, MAX_PACKET_SIZE - psOverlapped->m_Processed_Packet_Size,
		psOverlapped->m_Buffer, psOverlapped->m_Data_Size);

	// ������ �����͵� ũ�⸦ ���� �����ش�.
	psOverlapped->m_Processed_Packet_Size += psOverlapped->m_Data_Size;

	return true;
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
	psOverlapped->m_WSABUF.len = sizeof(psOverlapped->m_Buffer);

	// WSARecv() �������� �ɱ�
	DWORD dwNumberOfBytesRecvd = 0, dwFlag = 0;

	int iResult = WSARecv(psOverlapped->m_Socket,
		&psOverlapped->m_WSABUF,
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

bool DHClient::Push_RecvData(Packet_Header* _Data_Packet, Socket_Struct* _Socket_Struct, Overlapped_Struct* _Overlapped_Struct, size_t _Pull_Size)
{
	// ��Ŷ�� �ƹ��� ������ �����ʰ� ���´ٸ� �߸� �� ��Ŷ�̴�.
	if (S2C_Packet_Type_None <= (unsigned int)_Data_Packet->Packet_Type)
	{
		// ���� ������ ����.
		delete _Data_Packet;
		_Data_Packet = nullptr;
		// Ŭ���̾�Ʈ ����
		SOCKET _Socket_Data = _Socket_Struct->m_Socket;
		Safe_CloseSocket();
		Available_Overlapped->ResetObject(_Overlapped_Struct);
		return LOGIC_FAIL;
	}

	// ������ �����͸� ���ϰ� �Բ� �������ش�.
	Network_Message* _Net_Msg = new Network_Message;
	_Net_Msg->Socket = _Socket_Struct->m_Socket;
	_Net_Msg->Packet = _Data_Packet;

	// ������ ��Ŷ�� ť�� �־�ΰ� ���߿� ó���Ѵ�.
	Recv_Data_Queue.push(_Net_Msg);

	// �����͵��� �̹��� ó���Ѹ�ŭ ����.
	memcpy_s(_Overlapped_Struct->m_Processing_Packet_Buffer, MAX_PACKET_SIZE,
		_Overlapped_Struct->m_Processing_Packet_Buffer + _Pull_Size, MAX_PACKET_SIZE - _Pull_Size);

	// ó���� ��Ŷ ũ�⸸ŭ ó���ҷ� ����
	_Overlapped_Struct->m_Processed_Packet_Size -= _Pull_Size;

	return LOGIC_SUCCESS;
}

void DHClient::IOFunction_Recv(DWORD dwNumberOfBytesTransferred, Overlapped_Struct* psOverlapped, Socket_Struct* psSocket)
{
	// �ƹ� �����͵� ���� �������� Overlapped ����.
	if (dwNumberOfBytesTransferred <= 0)
	{
		// ���� �ɱ�( �̹��� ����� �������带 �ٽ� ��� )
		if (!Reserve_WSAReceive(psOverlapped->m_Socket, psOverlapped))
		{
			// Ŭ���̾�Ʈ ����
			Safe_CloseSocket();
			Available_Overlapped->ResetObject(psOverlapped);
		}
		return;
	}

	printf_s("[TCP Ŭ���̾�Ʈ] [%d Byte] ��Ŷ ���� �Ϸ�\n", dwNumberOfBytesTransferred);

	// �̹��� ���� ������ ��
	psOverlapped->m_Data_Size = dwNumberOfBytesTransferred;

	// �̹��� �޾ƿ� ���� �����͸�ŭ ó���� ���۷� �ű��.
	if (BackUp_Overlapped(psOverlapped) == false)
	{
		// Ŭ���̾�Ʈ ����
		Safe_CloseSocket();
		Available_Overlapped->ResetObject(psOverlapped);
		return;
	}

	// ó���� �����Ͱ� ������ ó��
	while (psOverlapped->m_Processed_Packet_Size > 0)
	{
		// header �� �� ���� ���ߴ�. �̾ recv()
		if (PACKET_HEADER_SIZE > psOverlapped->m_Processed_Packet_Size)
		{
			break;
		}

		// ��Ŷ ����� ���� ������
		Packet_Header* Packet_header = nullptr;

		// ���ۿ� ���� �����ʹ� �������� �������̴�. (size_t �� ġȯ�ϴ� ������, ��Ŷ�� ����� size_t ������ �����ؼ� ������ �����̴�.)
		size_t Packet_Body_Size = *reinterpret_cast<size_t*>(psOverlapped->m_Processing_Packet_Buffer);

		// �� ��Ŷ ������.
		size_t Packet_Total_Size = PACKET_HEADER_SIZE + Packet_Body_Size;

		// �ϳ��� ��Ŷ�� �� ���� ���ߴ�. �̾ recv()
		if (Packet_Total_Size > psOverlapped->m_Processed_Packet_Size)
		{
			break;
		}

		// �����κ��� ������ ��Ŷ(�ϼ��� ��Ŷ)
		Packet_header = reinterpret_cast<Packet_Header*>(malloc(Packet_Total_Size));
		memcpy_s(Packet_header, Packet_Total_Size, psOverlapped->m_Processing_Packet_Buffer, Packet_Total_Size);

		if (Push_RecvData(Packet_header, psSocket, psOverlapped, Packet_Total_Size) == false)
		{
			return;
		}
	}

	// ���� �ɱ�( �̹��� ����� �������带 �ٽ� ��� )
	if (!Reserve_WSAReceive(psOverlapped->m_Socket, psOverlapped))
	{
		// Ŭ���̾�Ʈ ����
		SOCKET _Socket_Data = psSocket->m_Socket;
		Safe_CloseSocket();
		Available_Overlapped->ResetObject(psOverlapped);
		return;
	}
}

void DHClient::IOFunction_Send(DWORD dwNumberOfBytesTransferred, Overlapped_Struct* psOverlapped, Socket_Struct* psSocket)
{
	printf_s("[TCP Ŭ���̾�Ʈ] [%15s:%d] [SOCKET : %d] [%d Byte] ��Ŷ �۽� �Ϸ�\n", IP.c_str(), PORT, (int)psOverlapped->m_Socket, dwNumberOfBytesTransferred);

	Available_Overlapped->ResetObject(psOverlapped);
}
