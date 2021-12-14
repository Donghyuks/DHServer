#include "DHServer.h"

unsigned short DHServer::MAX_USER_COUNT = 0;

DHServer::DHServer()
{

}

DHServer::~DHServer()
{
	End();
}

BOOL DHServer::Accept(unsigned short _Port, unsigned short _Max_User_Count)
{
	/// ��Ʈ�� �ִ��ο��� ����.
	PORT = _Port; MAX_USER_COUNT = _Max_User_Count;

	/// ������ ����ϱ����� ���۸� ������ ��. (�޸�Ǯ ���)
	TCHAR* Error_Buffer = (TCHAR*)m_MemoryPool.GetMemory(ERROR_MSG_BUFIZE);

	/// IOCP�� �����Ѵ�.
	g_IOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	assert(nullptr != g_IOCP);

	/// ��밡���� Overlapped�� �����صд�. (2000���� �����ص�)
	Available_Overlapped = new ObjectPool<Overlapped_Struct>(TOTAL_OVERLAPPED_COUNT);

	/// Worker Thread���� ���� �ھ�� ~ �ھ��*2 �� ������ �����Ѵٰ� �Ѵ�.
	/// �ý����� ������ �޾ƿͼ� �ھ���� *2����ŭ WorkThread�� �����Ѵ�.
	CreateWorkThread();

	/// ������ ����ü�� �����Ѵ�.
	g_Listen_Socket = std::make_shared<SOCKET>();

	/// Socket ����.
	*g_Listen_Socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);

	if (INVALID_SOCKET == *g_Listen_Socket)
	{
		/// TCHAR�� ���� �����ڵ�/��Ƽ����Ʈ�� ������ ��Ȳ�� ���׸��ϰ� ������ �� �ֵ��� �Ѵ�.
		_stprintf_s(Error_Buffer, ERROR_MSG_BUFIZE, _T("[TCP ����] ���� �߻� -- WSASocket() :"));
		err_display(Error_Buffer);

		WSACleanup();
		return LOGIC_FAIL;
	}

	/// Listen ������ IOCP�� ����� �س��� AcceptEx�� ���� �ְ���!!!!!!!!!!!
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(*g_Listen_Socket), g_IOCP, 0, 0);

	/// bind
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(PORT);
	if (SOCKET_ERROR == bind(*g_Listen_Socket, reinterpret_cast<SOCKADDR*>(&serveraddr), sizeof(serveraddr)))
	{
		/// TCHAR�� ���� �����ڵ�/��Ƽ����Ʈ�� ������ ��Ȳ�� ���׸��ϰ� ������ �� �ֵ��� �Ѵ�.
		_stprintf_s(Error_Buffer, ERROR_MSG_BUFIZE, _T("[TCP ����] ���� �߻� -- bind() :"));
		err_display(Error_Buffer);

		closesocket(*g_Listen_Socket);
		*g_Listen_Socket = INVALID_SOCKET;

		WSACleanup();
		return LOGIC_FAIL;
	}

	/// listen ( ���⿡ 5���� �����Ѵ��ؼ� 5�� �޴°� �ƴϴ�. ������ ���� )
	if (SOCKET_ERROR == listen(*g_Listen_Socket, 5))
	{
		/// TCHAR�� ���� �����ڵ�/��Ƽ����Ʈ�� ������ ��Ȳ�� ���׸��ϰ� ������ �� �ֵ��� �Ѵ�.
		_stprintf_s(Error_Buffer, ERROR_MSG_BUFIZE, _T("[TCP ����] ���� �߻� -- listen() :"));
		err_display(Error_Buffer);

		closesocket(*g_Listen_Socket);
		*g_Listen_Socket = INVALID_SOCKET;

		WSACleanup();
		return LOGIC_FAIL;
	}

	printf_s("[TCP ����] ����\n");

	DWORD dwByte;	/// ó�� Byte

	/// 10�������� ������ �߰��� ���� ������ MAX_USER���� �ʰ��� ��츦 ó���Ѵ�.
	for (unsigned short i = 0; i < MAX_USER_COUNT + 10; i++)
	{
		/// �������� ����
		auto psOverlapped = Available_Overlapped->GetObject();

		psOverlapped->m_IOType = Overlapped_Struct::IOType::IOType_Accept;
		/// Socket ����.
		psOverlapped->m_Socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);

		if (INVALID_SOCKET == psOverlapped->m_Socket)
		{
			/// TCHAR�� ���� �����ڵ�/��Ƽ����Ʈ�� ������ ��Ȳ�� ���׸��ϰ� ������ �� �ֵ��� �Ѵ�.
			_stprintf_s(Error_Buffer, ERROR_MSG_BUFIZE, _T("[TCP ����] ���� �߻� -- Accept ���� ������ ���� �߻� :"));
			err_display(Error_Buffer);

			WSACleanup();
			return LOGIC_FAIL;
		}

		/// AcceptEx �Լ��� ����Ͽ� ������ Queue�� ���.
		BOOL Return_Value = AcceptEx(*g_Listen_Socket,				// Listen ���¿� �ִ� ����, �ش� ���Ͽ� ���� ����õ��� ��ٸ��Ե�.
			psOverlapped->m_Socket,									// ������ ������ �޴� ����, ����ǰų� bound ���� �ʾƾ� �Ѵ�.
			&psOverlapped->m_Buffer[0],								// ���� ���� ���ῡ�� ���� ù��° ������ ����� ����, ������ �����ּ�, Ŭ���̾�Ʈ�� ����Ʈ �ּ�
			0,														// ���� �������� ����
			sizeof(sockaddr_in) + IP_SIZE,							// ���� �ּ� ������ ����Ʈ ��
			sizeof(sockaddr_in) + IP_SIZE,							// ����Ʈ �ּ� ������ ����Ʈ ��
			&dwByte,												// ���� ����Ʈ�� ����
			psOverlapped							// Overlapped ����ü
		);

		if (ERROR_IO_PENDING != WSAGetLastError())
		{
			wprintf(L"Create accept socket failed with error: %u\n", WSAGetLastError());
			_stprintf_s(Error_Buffer, ERROR_MSG_BUFIZE, _T("[TCP ����] ���� �߻� -- AcceptEx �Լ�"));
			err_display(Error_Buffer);
			WSACleanup();
			return LOGIC_FAIL;
		}

		// Ŭ���̾�Ʈ ����ü ���� �� ���
		Socket_Struct* psSocket = new Socket_Struct;
		psSocket->m_Socket = psOverlapped->m_Socket;

		// IOCP�� ������ �ڵ�� ���� ����Ѵ�.
		if (CreateIoCompletionPort(reinterpret_cast<HANDLE>(psOverlapped->m_Socket), g_IOCP, reinterpret_cast<ULONG_PTR>(psSocket), 0) != g_IOCP)
		{
			wprintf(L"CreateIoCompletionPort Function error: %u\n", WSAGetLastError());
			_stprintf_s(Error_Buffer, ERROR_MSG_BUFIZE, _T("[TCP ����] ���� �߻� -- CreateIoCompletionPort �Լ�"));
			err_display(Error_Buffer);
			assert(false);
			// ���� �ÿ� ���� ������ ������ �ʱ�ȭ.
			delete psSocket;
		}

		// ������ ���� ����ü�� 1:1�� �ڵ�� ���εǾ� ���α׷��� ������ ���� �������� �ʰ� ��Ȱ��.
		g_Socket_Struct_Pool.insert({ psSocket->m_Socket , psSocket });
	}

	// Send ó���� ���� ������ ����
	g_Send_Thread = new std::thread(std::bind(&DHServer::SendThread, this));

	// ����� �޸� ��ȯ.
	m_MemoryPool.ResetMemory(Error_Buffer, ERROR_MSG_BUFIZE);

	return LOGIC_SUCCESS;
}

BOOL DHServer::Send(Packet_Header* Send_Packet, SOCKET _Socket /*= INVALID_SOCKET*/)
{
	/// ��� Ŭ���̾�Ʈ���� �޼����� ����.
	// �ش� ������ �������� �ʰų� ��Ŷ�� null �̸� ����.
	assert(nullptr != Send_Packet);

	// ��ϵ��� ���� ��Ŷ�� ������ �� ����.
	if (S2C_Packet_Type_None <= Send_Packet->Packet_Type)
	{
		return LOGIC_FAIL;
	}

	// ���� ������ �������� �ʾҴٸ� ��ο��� �޼����� ����.
	if (_Socket == INVALID_SOCKET)
	{
		return BroadCastMessage(Send_Packet);
	}
	else
	{
		// ���� ��Ŷ�� �����ؼ� Send ť�� ����.
		size_t Send_Packet_Total_Size = PACKET_HEADER_SIZE + Send_Packet->Packet_Size;
		Packet_Header* Copy_Packet = reinterpret_cast<Packet_Header*>(malloc(Send_Packet_Total_Size));
		memcpy_s(Copy_Packet, Send_Packet_Total_Size, Send_Packet, Send_Packet_Total_Size);

		// ���� Map ����ü�� ���� �ش� ���� lock�� �ɷ����� ����.
		Client_Map::const_accessor m_Accessor;

		// �ش� ���Ͽ� �ش��ϴ� Ŭ���̾�Ʈ�� ������ �ִ°�?
		if (g_Connected_Client.find(m_Accessor, _Socket) == false)
		{
			m_Accessor.release();
			return LOGIC_FAIL;
		}

		Send_Data_Queue.push({ m_Accessor->second->m_Socket , Copy_Packet });

		// �ش� �ڷῡ ���� lock ����.
		m_Accessor.release();
	}

	return LOGIC_SUCCESS;
}


BOOL DHServer::Recv(std::vector<Network_Message>& _Message_Vec)
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

BOOL DHServer::Disconnect(SOCKET _Socket)
{
	// �ش� ������ �����ϴ��� ���� �˻�.
	assert(_Socket != INVALID_SOCKET);
	assert(FindSocketOnClient(_Socket));

	// Ŭ���̾�Ʈ ����
	Delete_in_Socket_List(_Socket);
	// �ش� ������ �����ϰ� ������.
	Reuse_Socket(_Socket);

	return LOGIC_SUCCESS;
}

BOOL DHServer::End()
{
	/// �̹� �ܺο��� �ʱ�ȭ�� ȣ�������� �ִٸ� �׳� ����. ( End()ȣ���� ��������� �Ҹ��ڿ��� �ڵ����� ���ֵ��� �ϱ�����. )
	if (g_IOCP == nullptr)
		return LOGIC_FAIL;

	g_Is_Exit = true;

	PostQueuedCompletionStatus(g_IOCP, 0, 0, nullptr);

	for (auto k : g_Work_Thread)
	{
		k->join();
	}

	g_Work_Thread.clear();

	// Send/Recv ť �ʱ�ȭ.
	Recv_Data_Queue.clear();
	Send_Data_Queue.clear();

	// IOCP ����
	CloseHandle(g_IOCP);
	g_IOCP = nullptr;

	// ������ ��ü ���Ͽ� ���� ���� �۾�.
	for (auto it : g_Socket_Struct_Pool)
	{
		// ���� Map ����ü�� ���� �ش� ���� lock�� �ɷ����� ����.
		Client_Map::accessor m_Accessor;

		// �ش� �ڷῡ���� lock. (�ٸ����� ������̶�� ��ٸ�)
		while (!g_Socket_Struct_Pool.find(m_Accessor, it.first)) {};

		shutdown(m_Accessor->second->m_Socket, SD_BOTH);
		closesocket(m_Accessor->second->m_Socket);

		m_Accessor.release();
	}

	// ����� Ǯ ����
	delete Available_Overlapped;

	// ���� ����
	WSACleanup();

	printf_s("[TCP ����] ����\n");

	return LOGIC_SUCCESS;
}

void DHServer::CreateWorkThread()
{
	SYSTEM_INFO SystemInfo;
	GetSystemInfo(&SystemInfo);
	// Send Thread �� ���� �־��ϹǷ� CPU �ھ� �ϳ��� ���ܵд�.
	int iThreadCount = (SystemInfo.dwNumberOfProcessors - 1) * 2;

	for (int i = 0; i < iThreadCount; i++)
	{
		/// Ŭ���̾�Ʈ �۾��� �ϴ� WorkThread�� ����.
		std::thread* Client_Work = new std::thread(std::bind(&DHServer::WorkThread, this));

		/// ������ �����ڿ� �־��ְ�..
		g_Work_Thread.push_back(Client_Work);

		/// ���ض� �������!!
	}
}

void DHServer::WorkThread()
{
	assert(nullptr != g_IOCP);

	DWORD* dwNumberOfBytesTransferred = (DWORD*)m_MemoryPool.GetMemory(sizeof(DWORD));
	ULONG* Entry_Count = (ULONG*)m_MemoryPool.GetMemory(sizeof(ULONG));
	ULONG* Get_Entry_Count = (ULONG*)m_MemoryPool.GetMemory(sizeof(ULONG));

	while (TRUE)
	{
		*dwNumberOfBytesTransferred = 0;
		Socket_Struct* psSocket = nullptr;
		Overlapped_Struct* psOverlapped = nullptr;

		/// GetQueuedCompletionStatusEx �� ���� �Ǹ鼭 ���� �ʿ��� �κ�.
		OVERLAPPED_ENTRY Entry_Data[64];	// ��� �� ��Ʈ���� �����͵�. �ִ� 64���� �����ص�.
		*Entry_Count = sizeof(Entry_Data) / sizeof(OVERLAPPED_ENTRY);	// ������ �������� �ִ� ���� ( 64�� )
		*Get_Entry_Count = 0;	// ������ �� ���� ��Ʈ���� ���� �Դ��� ����ϱ� ���� ����.

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
			*Entry_Count,
			Get_Entry_Count,
			INFINITE,
			0);

		// �������� ��� üũ
		if (!bSuccessed)
		{
			printf_s("[TCP ����] [WorkThread] �������� ����� üũ�ϴ� ���� ������ ������ ������ϴ�.\n");
			continue;
		}

		/// ������ �޾ƿ� Entry �����Ϳ� ���Ͽ�
		for (ULONG i = 0; i < *Get_Entry_Count; i++)
		{
			/// WSARead() / WSAWrite() � ���� Overlapped �����͸� �����´�.
			psOverlapped = reinterpret_cast<Overlapped_Struct*>(Entry_Data[i].lpOverlapped);
			/// Key������ �Ѱ������ Socket_Struct ������.
			psSocket = reinterpret_cast<Socket_Struct*>(Entry_Data[i].lpCompletionKey);
			/// I/O�� ���� �������� ũ��.
			*dwNumberOfBytesTransferred = Entry_Data[i].dwNumberOfBytesTransferred;

			if (!psOverlapped)
			{
				// Overlapped �� 0 �̶�� ���� ������ ���Ḧ �˸���.(Ȥ�� �𸣴� �޼����� ���� ����..)
				printf_s("[TCP ����] [WorkThread] Overlapped �� NULL �Դϴ�.\n");
				PostQueuedCompletionStatus(g_IOCP, 0, 0, nullptr);
				return;
			}

			/// ���� Disconnect �޼����� �Դٸ�.
			switch (psOverlapped->m_IOType)
			{
			// Disconnect() �� Overlapped I/O �Ϸῡ ���� ó��
			case Overlapped_Struct::IOType::IOType_Disconnect:
			{
				// Overlapped �� ���ο��� AcceptEx�� �ɶ� �����ϰԵȴ�.
				IOFunction_Disconnect(psOverlapped);
				// ���ϱ���ü�� ���� ��밡���� ���·� �ٲ��ش�.
				psSocket->Is_Available = true;
				continue;	
			}
			// Accept() �� Overlapped I/O �Ϸῡ ���� ó��
			case Overlapped_Struct::IOType::IOType_Accept: 
			{
				IOFunction_Accept(psOverlapped);
				continue; 
			}
			}

			// Ű�� nullptr �� ���..
			if (!psSocket)
			{
				assert(false);
				printf_s("[TCP ����] [WorkThread] Key(Socket) �� NULL �Դϴ�.\n");
				continue;
			}

			// Ŭ���̾�Ʈ ���� ����
			if (0 == *dwNumberOfBytesTransferred)
			{
				SOCKET _Socket_Data = psSocket->m_Socket;
				Delete_in_Socket_List(_Socket_Data);
				Reuse_Socket(_Socket_Data);
				Available_Overlapped->ResetObject(psOverlapped);
				continue;
			}

			// ���� ���Ͽ� ���� ������ �������ΰ�� �ش� ���������� �����͸� ����Ѵ�.
			if (!psSocket->Is_Available)
			{
				Available_Overlapped->ResetObject(psOverlapped);
				continue;
			}

			// Overlapped I/O ó��
			switch (psOverlapped->m_IOType)
			{
			case Overlapped_Struct::IOType::IOType_Recv: IOFunction_Recv(*dwNumberOfBytesTransferred, psOverlapped, psSocket); break; // WSARecv() �� Overlapped I/O �Ϸῡ ���� ó��
			case Overlapped_Struct::IOType::IOType_Send: IOFunction_Send(*dwNumberOfBytesTransferred, psOverlapped, psSocket); break; // WSASend() �� Overlapped I/O �Ϸῡ ���� ó��
			}
		}
	}

	// ����� �޸� ���� ��ȯ.
	m_MemoryPool.ResetMemory(dwNumberOfBytesTransferred, sizeof(DWORD));
	m_MemoryPool.ResetMemory(Entry_Count, sizeof(ULONG));
	m_MemoryPool.ResetMemory(Get_Entry_Count, sizeof(ULONG));

	return;
}

void DHServer::SendThread()
{
	// ���� ��Ŷ�� �� ������ (��� + ���� ���ۿ� ����ִ� ��Ŷ ������)
	size_t Total_Packet_Size = 0;
	// Buffer�� ��ŭ ��ġ���� �߶���ϴ��� ��Ÿ��������.
	size_t Buff_Offset = 0;
	// �����͸� Queue ���� ������ ���� ����.
	std::pair<SOCKET, Packet_Header*> Send_Packet;

	while (!g_Is_Exit)
	{
		// ���� �޼����� �ִ� ť���� �����Ͱ� �ִ��� �˻�
		while (!Send_Data_Queue.empty())
		{
			// ť���� ���� �����͸� ���´�.
			Send_Data_Queue.try_pop(Send_Packet);
			
			{
				// ���� Map ����ü�� ���� �ش� ���� lock�� �ɷ����� ����.
				Client_Map::accessor m_Accessor;

				// ���� �������ƴ� accessor�� �ҷ����� false �� ������ �����Ǿ��ų� �����Ǵ� ������ ��Ÿ����.
				if (g_Connected_Client.find(m_Accessor, Send_Packet.first) == false)
				{
					// �ش� ��Ŷ�� ����.
					delete Send_Packet.second;
					m_Accessor.release();
					continue;
				}

				// �ʱ� ����.
				Buff_Offset = 0;
				Total_Packet_Size = PACKET_HEADER_SIZE + Send_Packet.second->Packet_Size;

				// ���� ��Ŷ�� ����� �غ�� ���ۺ��� ũ�ٸ� �߶� �������� �����ش�.
				while (Total_Packet_Size > 0)
				{
					// �������� ����
					Overlapped_Struct* psOverlapped = Available_Overlapped->GetObject();
					psOverlapped->m_IOType = Overlapped_Struct::IOType::IOType_Send;
					psOverlapped->m_Socket = Send_Packet.first;

					// ��Ŷ ����
					// ��Ŷ�� ���� �������� ���ۺ��� ����� ū ���
					if (Total_Packet_Size >= OVERLAPPED_BUFIZE)
					{
						psOverlapped->m_Data_Size = OVERLAPPED_BUFIZE;
						Total_Packet_Size -= OVERLAPPED_BUFIZE;
						memcpy_s(psOverlapped->m_Buffer, OVERLAPPED_BUFIZE, (char*)Send_Packet.second + (Buff_Offset * OVERLAPPED_BUFIZE), OVERLAPPED_BUFIZE);
					}
					// �������� ���ۺ��� ����� ���� ���
					else
					{
						psOverlapped->m_Data_Size = Total_Packet_Size;
						memcpy_s(psOverlapped->m_Buffer, OVERLAPPED_BUFIZE, (char*)Send_Packet.second + (Buff_Offset * OVERLAPPED_BUFIZE), Total_Packet_Size);
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

					// ��ȿ���� ���� ������ ���� ��� ( �ַ� 10053 ������ ����. => ������ �ٸ� �����忡�� ������ ��� )
					if ((SOCKET_ERROR == iResult) && (WSAGetLastError() != WSA_IO_PENDING))
					{
						/// TCHAR�� ���� �����ڵ�/��Ƽ����Ʈ�� ������ ��Ȳ�� ���׸��ϰ� ������ �� �ֵ��� �Ѵ�.
						TCHAR szBuffer[ERROR_MSG_BUFIZE] = { 0, };
						_stprintf_s(szBuffer, _countof(szBuffer), _T("[TCP ����] [SendThread] ���� �߻� -- WSASend() :"));
						err_display(szBuffer);

						// ������ �������带 ���� �Ѵ�.
						Available_Overlapped->ResetObject(psOverlapped);
						break;
					}

				}
				// �� ó���� ������ ����.
				delete Send_Packet.second;
				// �ش� �ڷῡ ���� lock ����.
				m_Accessor.release();
			}
		}
		// ���� �޼����� �������� �ٸ� �����忡�� �������� �Ѱ���.
		Sleep(0);
	}
}

bool DHServer::AddClientSocket(Socket_Struct* psSocket)
{
	assert(nullptr != psSocket);

	// Ŭ���̾�Ʈ �ߺ� �˻�

	if (!FindSocketOnClient(psSocket->m_Socket))
	{
		// �ߺ��� �ƴ϶�� ����� Ŭ���̾�Ʈ ����Ʈ�� ���.
		g_Connected_Client.insert({ psSocket->m_Socket, psSocket });
		return LOGIC_SUCCESS;
	}
	else
	{
		return LOGIC_FAIL;
	}
}

void DHServer::Reuse_Socket(SOCKET _Socket)
{
	Client_Map::accessor m_accessor;

	// �ش� ���Ϲ�ȣ�� �����ϴ� ���� ����ü�� �����´�.
	g_Socket_Struct_Pool.find(m_accessor, _Socket);

	// �Ҹ��ڸ� ȣ���Ͽ� �ʱ�ȭ.
	m_accessor->second = new (m_accessor->second) Socket_Struct;
	// ���� ��ȣ�� ��Ȱ��. ������ key ������ ���ϰ� ��Ī�Ǿ� �����Ƿ�.
	m_accessor->second->m_Socket = _Socket;

	shutdown(_Socket, SD_BOTH);

	auto Disconnect_Overlapped = Available_Overlapped->GetObject();
	Disconnect_Overlapped->m_IOType = Overlapped_Struct::IOType::IOType_Disconnect;
	Disconnect_Overlapped->m_Socket = _Socket;
	Disconnect_EX(_Socket, Disconnect_Overlapped, TF_REUSE_SOCKET, 0);

	// ����� ȣ��.
	m_accessor.release();
}

void DHServer::Delete_in_Socket_List(SOCKET _Socket)
{
	Client_Map::const_accessor m_Socket_Struct_Acc;

	// �ش� ���Ϲ�ȣ�� �����ϴ� ���� ����ü�� �����´�.
	if (g_Socket_Struct_Pool.find(m_Socket_Struct_Acc, _Socket) == false)
	{
		printf_s("[TCP ����] [Delete Socket List] ��ȿ���� ���� Socket ��ȣ�Դϴ�.\n");
		assert(false);
		return;
	}

	// ���� Map ����ü�� ���� �ش� ���� lock�� �ɷ����� ����.
	Client_Map::accessor m_Connect_Client_Acc;

	// �ش� �ڷῡ���� lock. (�ٸ����� ������̶�� ��ٸ�)
	while (!g_Connected_Client.find(m_Connect_Client_Acc, _Socket)) {};

	// �ش��ϴ� Ŭ���̾�Ʈ ������ ����Ʈ���� ����.
	g_Connected_Client.erase(m_Connect_Client_Acc);
	m_Connect_Client_Acc.release();

	std::string Socket_IP(m_Socket_Struct_Acc->second->IP);
	auto Socket_PORT = m_Socket_Struct_Acc->second->PORT;
	int Socket_Number = m_Socket_Struct_Acc->second->m_Socket;
	m_Socket_Struct_Acc->second->Is_Available = false;

	m_Socket_Struct_Acc.release();

	printf_s("[TCP ����] [%15s:%5d] [Socket_Number:%d] Ŭ���̾�Ʈ�� ����\n", Socket_IP.c_str(), Socket_PORT, Socket_Number);

}

bool DHServer::Reserve_WSAReceive(SOCKET socket, Overlapped_Struct* psOverlapped /* = nullptr */)
{
	assert(INVALID_SOCKET != socket);

	// ����� �������带 ���� �ʾ����� ����
	if (nullptr == psOverlapped)
	{
		psOverlapped = Available_Overlapped->GetObjectW();
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

	int wResult = WSAGetLastError();

	if ((SOCKET_ERROR == iResult) && (WSAGetLastError() != WSA_IO_PENDING))
	{
		/// TCHAR�� ���� �����ڵ�/��Ƽ����Ʈ�� ������ ��Ȳ�� ���׸��ϰ� ������ �� �ֵ��� �Ѵ�.
		TCHAR szBuffer[ERROR_MSG_BUFIZE] = { 0, };
		_stprintf_s(szBuffer, _countof(szBuffer), _T("[TCP ����] ���� �߻� -- WSARecv() :"));
		err_display(szBuffer);

		return FALSE;
	}

	return TRUE;
}

bool DHServer::BroadCastMessage(Packet_Header* Send_Packet)
{
	size_t Send_Packet_Total_Size = PACKET_HEADER_SIZE + Send_Packet->Packet_Size;

	// ��ü Ŭ���̾�Ʈ ���Ͽ� ��Ŷ �۽�
	for (auto itr : g_Connected_Client)
	{
		// ���� ��Ŷ�� �����ؼ� Send ť�� ����.
		Packet_Header* Copy_Packet = reinterpret_cast<Packet_Header*>(malloc(Send_Packet_Total_Size));
		memcpy_s(Copy_Packet, Send_Packet_Total_Size, Send_Packet, Send_Packet_Total_Size);

		// ���� Map ����ü�� ���� �ش� ���� lock�� �ɷ����� ����.
		Client_Map::const_accessor m_Accessor;

		// �ش� ���Ͽ� �ش��ϴ� Ŭ���̾�Ʈ�� ������ �ִ°�?
		if (g_Connected_Client.find(m_Accessor, itr.first) == false)
		{
			m_Accessor.release();
			return LOGIC_FAIL;
		}

		Send_Data_Queue.push({ m_Accessor->first , Copy_Packet });

		// �ش� �ڷῡ ���� lock ����.
		m_Accessor.release();
	}

	return LOGIC_SUCCESS;
}

bool DHServer::BackUp_Overlapped(Overlapped_Struct* psOverlapped)
{
	if ((psOverlapped->m_Processed_Packet_Size + psOverlapped->m_Data_Size) > MAX_PACKET_SIZE)
	{
		printf_s("[TCP ����] Ŭ���̾�Ʈ�� [%d]Byte �� �ʰ��ϴ� ��Ŷ ó���� ���Խ��ϴ�. \n", MAX_PACKET_SIZE);
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

bool DHServer::Push_RecvData(Packet_Header* _Data_Packet, Socket_Struct* _Socket_Struct, Overlapped_Struct* _Overlapped_Struct, size_t _Pull_Size)
{
	// ��Ŷ�� �ƹ��� ������ �����ʰ� ���´ٸ� �߸� �� ��Ŷ�̴�.
	if (C2S_Packet_Type_None <= (unsigned int)_Data_Packet->Packet_Type)
	{
		// ���� ������ ����.
		delete _Data_Packet;
		_Data_Packet = nullptr;
		// Ŭ���̾�Ʈ ����
		SOCKET _Socket_Data = _Socket_Struct->m_Socket;
		Delete_in_Socket_List(_Socket_Data);
		Reuse_Socket(_Socket_Data);
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

bool DHServer::FindSocketOnClient(SOCKET _Target)
{
	bool _Find_Result = false;
	// ���� Map ����ü�� ���� �ش� ���� lock�� �ɷ����� ����.
	Client_Map::const_accessor m_Accessor;

	// �ش��ϴ� ������ �����ϴ��� �˻�
	_Find_Result = g_Connected_Client.find(m_Accessor, _Target);

	// �ش� �ڷῡ ���� lock ����.
	m_Accessor.release();

	return _Find_Result;
}

void DHServer::IOFunction_Recv(DWORD dwNumberOfBytesTransferred, Overlapped_Struct* psOverlapped, Socket_Struct* psSocket)
{
	// �ƹ� �����͵� ���� �������� Overlapped ����.
	if (dwNumberOfBytesTransferred <= 0)
	{
		// ���� �ɱ�( �̹��� ����� �������带 �ٽ� ��� )
		if (!Reserve_WSAReceive(psOverlapped->m_Socket, psOverlapped))
		{
			// Ŭ���̾�Ʈ ����
			SOCKET _Socket_Data = psSocket->m_Socket;
			Delete_in_Socket_List(_Socket_Data);
			Reuse_Socket(_Socket_Data);
			Available_Overlapped->ResetObject(psOverlapped);
		}
		return;
	}

	printf_s("[TCP ����] [%15s:%5d] [SOCKET : %d] [%d Byte] ��Ŷ ���� �Ϸ�\n", psSocket->IP.c_str(), psSocket->PORT, (int)psSocket->m_Socket, dwNumberOfBytesTransferred);

	// �̹��� ���� ������ ��
	psOverlapped->m_Data_Size = dwNumberOfBytesTransferred;

	// �̹��� �޾ƿ� ���� �����͸�ŭ ó���� ���۷� �ű��.
	if (BackUp_Overlapped(psOverlapped) == false)
	{
		// Ŭ���̾�Ʈ ����
		SOCKET _Socket_Data = psSocket->m_Socket;
		Delete_in_Socket_List(_Socket_Data);
		Reuse_Socket(_Socket_Data);
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

		// Ŭ���̾�Ʈ�κ��� ������ ��Ŷ(�ϼ��� ��Ŷ)
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
		Delete_in_Socket_List(_Socket_Data);
		Reuse_Socket(_Socket_Data);
		Available_Overlapped->ResetObject(psOverlapped);
		return;
	}
}

void DHServer::IOFunction_Send(DWORD dwNumberOfBytesTransferred, Overlapped_Struct* psOverlapped, Socket_Struct* psSocket)
{
	printf_s("[TCP ����] [%15s:%5d] [SOCKET : %d] [%d Byte] ��Ŷ �۽� �Ϸ�\n", psSocket->IP.c_str(), psSocket->PORT, (int)psSocket->m_Socket, dwNumberOfBytesTransferred);

	Available_Overlapped->ResetObject(psOverlapped);
}

void DHServer::IOFunction_Accept(Overlapped_Struct* psOverlapped)
{
	/// ���� �ִ� �������� �����ߴٸ� �ش� ������ ������� �Ѵ�.
	if (g_Connected_Client.size() == MAX_USER_COUNT)
	{
		int k = g_Connected_Client.size();
		// Ŭ���̾�Ʈ ����
		SOCKET _Socket_Data = psOverlapped->m_Socket;
		Reuse_Socket(_Socket_Data);
		Available_Overlapped->ResetObject(psOverlapped);
		return;
	}

	TCHAR Error_Buffer[ERROR_MSG_BUFIZE] = { 0, };

	/// Accept
	SOCKADDR_IN* Localaddr = NULL;
	SOCKADDR_IN* Remoteaddr = NULL;
	int addrlen = sizeof(SOCKADDR_IN);
	//ZeroMemory(&Localaddr, addrlen);
	//ZeroMemory(&Remoteaddr, addrlen);

	GetAcceptExSockaddrs(
		&psOverlapped->m_Buffer[0],
		0,
		sizeof(SOCKADDR_IN) + IP_SIZE,
		sizeof(SOCKADDR_IN) + IP_SIZE,
		(SOCKADDR**)&Localaddr, &addrlen,
		(SOCKADDR**)&Remoteaddr, &addrlen
	);


	/// �������� ������ ���� ���� �ʿ���. (�ش� �ڷῡ ����)
	Client_Map::accessor m_accessor;

	// �ش� ���Ͽ� ���� ���� ����ü�� �����´�.
	while (!g_Socket_Struct_Pool.find(m_accessor, psOverlapped->m_Socket)) {};
	Socket_Struct* psSocket = m_accessor->second;

	/// IPv4 ����� IP ����ϱ�.
	inet_ntop(AF_INET, &Remoteaddr->sin_addr, const_cast<PSTR>(psSocket->IP.c_str()), sizeof(psSocket->IP));
	psSocket->PORT = ntohs(Remoteaddr->sin_port);

	m_accessor.release();

	// ����� Ŭ���̾�Ʈ ������ ���
	if (!AddClientSocket(psSocket))
	{
		printf_s("[TCP ����] Ŭ���̾�Ʈ ���� ����Ʈ�� ��� ����.\n");
		assert(false);
		return;
	}

	printf_s("[TCP ����] [%15s:%5d] [Socket : %d] Ŭ���̾�Ʈ ����\n", psSocket->IP.c_str(), psSocket->PORT, (int)psSocket->m_Socket);

	/// WSARecv�� �ɾ�־� ������ ���� �� �ְ���?!
	if (!Reserve_WSAReceive(psSocket->m_Socket))
	{
		// Ŭ���̾�Ʈ ����
		SOCKET _Socket_Data = psSocket->m_Socket;
		Delete_in_Socket_List(_Socket_Data);
		Reuse_Socket(_Socket_Data);
		Available_Overlapped->ResetObject(psOverlapped);
	}

	// ���ϱ���ü�� ���� ��밡���� ���·� �ٲ��ش�.
	psSocket->Is_Available = true;
}

void DHServer::IOFunction_Disconnect(Overlapped_Struct* psOverlapped)
{
	/// ������ ����ϱ����� ���۸� ������ ��.
	TCHAR Error_Buffer[ERROR_MSG_BUFIZE] = { 0, };

	/// �̹� Disconnect�� ���ö� Overlapped�� ������ ��ϵǾ�����.
	psOverlapped->m_IOType = Overlapped_Struct::IOType::IOType_Accept;

	DWORD dwByte;
	/// AcceptEx �Լ��� ����Ͽ� ������ Queue�� ���.
	BOOL Return_Value = AcceptEx(*g_Listen_Socket,				// Listen ���¿� �ִ� ����, �ش� ���Ͽ� ���� ����õ��� ��ٸ��Ե�.
		psOverlapped->m_Socket,									// ������ ������ �޴� ����, ����ǰų� bound ���� �ʾƾ� �Ѵ�.
		&psOverlapped->m_Buffer[0],								// ���� ���� ���ῡ�� ���� ù��° ������ ����� ����, ������ �����ּ�, Ŭ���̾�Ʈ�� ����Ʈ �ּ�
		0,														// ���� �������� ����
		sizeof(sockaddr_in) + IP_SIZE,							// ���� �ּ� ������ ����Ʈ ��
		sizeof(sockaddr_in) + IP_SIZE,							// ����Ʈ �ּ� ������ ����Ʈ ��
		&dwByte,												// ���� ����Ʈ�� ����
		psOverlapped							// Overlapped ����ü
	);

	if (ERROR_IO_PENDING != WSAGetLastError())
	{
		wprintf(L"Create accept socket failed with error: %u\n", WSAGetLastError());
		_stprintf_s(Error_Buffer, _countof(Error_Buffer), _T("[TCP ����] ���� �߻� -- DisconnectEX �Լ�"));
		err_display(Error_Buffer);
		WSACleanup();
		assert(false);
		return;
	}
}
