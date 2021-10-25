#include "Server.h"

unsigned short Server::MAX_USER_COUNT = 0;

Server::Server()
{
	
}

Server::~Server()
{

}

bool Server::Start()
{
	/// ������ ����ϱ����� ���۸� ������ ��. (�޸�Ǯ ���)
	TCHAR* Error_Buffer = (TCHAR*)m_MemoryPool.GetMemory(MSG_BUFSIZE);

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
		_stprintf_s(Error_Buffer, MSG_BUFSIZE, _T("[TCP ����] ���� �߻� -- WSASocket() :"));
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
		_stprintf_s(Error_Buffer, MSG_BUFSIZE, _T("[TCP ����] ���� �߻� -- bind() :"));
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
		_stprintf_s(Error_Buffer, MSG_BUFSIZE, _T("[TCP ����] ���� �߻� -- listen() :"));
		err_display(Error_Buffer);

		closesocket(*g_Listen_Socket);
		*g_Listen_Socket = INVALID_SOCKET;

		WSACleanup();
		return LOGIC_FAIL;
	}

	/// Ŭ���̾�Ʈ�� ���Ḧ üũ�ϱ� ���� ������ ����.
	g_Exit_Check_Thread = new std::thread(std::bind(&Server::ExitThread, this));

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
			_stprintf_s(Error_Buffer, MSG_BUFSIZE, _T("[TCP ����] ���� �߻� -- Accept ���� ������ ���� �߻� :"));
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
			_stprintf_s(Error_Buffer, MSG_BUFSIZE, _T("[TCP ����] ���� �߻� -- AcceptEx �Լ�"));
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
			_stprintf_s(Error_Buffer, MSG_BUFSIZE, _T("[TCP ����] ���� �߻� -- CreateIoCompletionPort �Լ�"));
			err_display(Error_Buffer);
			assert(false);
			// ���� �ÿ� ���� ������ ������ �ʱ�ȭ.
			delete psSocket;
		}

		// ������ ���� ����ü�� 1:1�� �ڵ�� ���εǾ� ���α׷��� ������ ���� �������� �ʰ� ��Ȱ��.
		g_Socket_Struct_Pool.insert({ psSocket->m_Socket , psSocket });
	}

	// ����� �޸� ��ȯ.
	m_MemoryPool.ResetMemory(Error_Buffer, MSG_BUFSIZE);

	return LOGIC_SUCCESS;
}

bool Server::Send(Packet_Header* Send_Packet)
{
	/// ��� Ŭ���̾�Ʈ���� �޼����� ����.

	assert(nullptr != Send_Packet);

	// �Ӱ� ����
	{
		// ��ü Ŭ���̾�Ʈ ���Ͽ� ��Ŷ �۽�
		for (auto it : g_Connected_Client)
		{
			// ���� Map ����ü�� ���� �ش� ���� lock�� �ɷ����� ����.
			Client_Map::accessor m_Accessor;

			// �ش� �ڷῡ���� lock. (�ٸ����� ������̶�� ��ٸ�)
			while (!g_Connected_Client.find(m_Accessor, it.first)) {};

			// ��Ŷ �۽�
			SendTargetSocket(m_Accessor->second->m_Socket, Send_Packet);

			// �ش� �ڷῡ ���� lock ����.
			m_Accessor.release();
		}
	}

	return true;
}


bool Server::Recv(std::vector<Network_Message*>& _Message_Vec)
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

	// ���� ����
	WSACleanup();

	printf_s("[TCP ����] ����\n");

	return true;
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
			printf_s("[TCP ����] �������� ����� üũ�ϴ� ���� ������ ������ ������ϴ�.\n");
			delete psSocket;
			delete psOverlapped;
			continue;
		}

		/// ������ �޾ƿ� Entry �����Ϳ� ���Ͽ�
		for (ULONG i = 0; i < *Get_Entry_Count; i++)
		{
			/// WSARead() / WSAWrite() � ���� Overlapped �����͸� �����´�.
			psOverlapped = reinterpret_cast<Overlapped_Struct*>(Entry_Data[i].lpOverlapped);

			if (!psOverlapped)
			{
				// �ٸ� WorkerThread() ���� ���Ḧ ���ؼ�
				PostQueuedCompletionStatus(g_IOCP, 0, 0, nullptr);
				break;
			}

			/// ���� Disconnect �޼����� �Դٸ�.
			switch (psOverlapped->m_IOType)
			{
			case Overlapped_Struct::IOType::IOType_Disconnect:
			{
				IOFunction_Disconnect(psOverlapped);
				continue;	// Disconnect() �� Overlapped I/O �Ϸῡ ���� ó��
			}
			case Overlapped_Struct::IOType::IOType_Accept: IOFunction_Accept(psOverlapped); continue; // Accept() �� Overlapped I/O �Ϸῡ ���� ó��

			}
			/// I/O�� ���� �������� ũ��.
			*dwNumberOfBytesTransferred = Entry_Data[i].dwNumberOfBytesTransferred;
			/// Key������ �Ѱ������ Socket_Struct ������.
			psSocket = reinterpret_cast<Socket_Struct*>(Entry_Data[i].lpCompletionKey);

			// Ű�� nullptr �� ��� ������ ���Ḧ �ǹ�
			if (!psSocket)
			{
				// �ٸ� WorkerThread() ���� ���Ḧ ���ؼ�
				PostQueuedCompletionStatus(g_IOCP, 0, 0, nullptr);
				break;
			}

			assert(nullptr != psOverlapped);

			// ���� ����
			if (0 == *dwNumberOfBytesTransferred)
			{
				SOCKET _Socket_Data = psSocket->m_Socket;
				Delete_in_Socket_List(psSocket);
				Reuse_Socket(_Socket_Data);
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

bool Server::AddClientSocket(Socket_Struct* psSocket)
{
	assert(nullptr != psSocket);

	// �Ӱ� ����
	{
		// Ŭ���̾�Ʈ �ߺ� �˻�
		for (auto it : g_Connected_Client)
		{
			{
				// const_accessor�� ���ʿ��� ���� ����. (������ �Ҷ�)
				Client_Map::const_accessor m_Accessor;

				while (!g_Connected_Client.find(m_Accessor, it.first)) {};

				if (m_Accessor->second == psSocket)
				{
					m_Accessor.release();
					return FALSE;
				}

				m_Accessor.release();
			}
		}

		// �ߺ��� �ƴ϶�� ����� Ŭ���̾�Ʈ ����Ʈ�� ���.
		g_Connected_Client.insert({ psSocket->m_Socket, psSocket });
	}


	return TRUE;
}

void Server::Reuse_Socket(SOCKET _Socket)
{
	Client_Map::accessor m_accessor;

	// �ش� ���Ϲ�ȣ�� �����ϴ� ���� ����ü�� �����´�.
	while (!g_Socket_Struct_Pool.find(m_accessor, _Socket)) {};

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

void Server::Delete_in_Socket_List(Socket_Struct* psSocket)
{
	std::string Socket_IP(psSocket->IP);
	auto Socket_PORT = psSocket->PORT;
	int Socket_Number = psSocket->m_Socket;

	for (auto it : g_Connected_Client)
	{
		// ���� Map ����ü�� ���� �ش� ���� lock�� �ɷ����� ����.
		Client_Map::accessor m_Accessor;

		// �ش� �ڷῡ���� lock. (�ٸ����� ������̶�� ��ٸ�)
		while (!g_Connected_Client.find(m_Accessor, it.first)) {};

		/// �ش��ϴ� �����͸� ã�´�.
		if (m_Accessor->second == psSocket)
		{
			/// �ش��ϴ� Ŭ���̾�Ʈ ������ ����Ʈ���� ����.
			g_Connected_Client.erase(m_Accessor);

			break;
		}
	}

	printf_s("[TCP ����] [%15s:%5d] [Socket_Number:%d] Ŭ���̾�Ʈ�� ����\n", Socket_IP.c_str(), Socket_PORT, Socket_Number);

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
		psOverlapped,
		nullptr);

	int wResult = WSAGetLastError();

	if ((SOCKET_ERROR == iResult) && (WSAGetLastError() != WSA_IO_PENDING))
	{
		/// TCHAR�� ���� �����ڵ�/��Ƽ����Ʈ�� ������ ��Ȳ�� ���׸��ϰ� ������ �� �ֵ��� �Ѵ�.
		TCHAR szBuffer[MSG_BUFSIZE] = { 0, };
		_stprintf_s(szBuffer, _countof(szBuffer), _T("[TCP ����] ���� �߻� -- WSARecv() :"));
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
		psOverlapped,
		nullptr);

	if ((SOCKET_ERROR == iResult) && (WSAGetLastError() != WSA_IO_PENDING))
	{
		/// TCHAR�� ���� �����ڵ�/��Ƽ����Ʈ�� ������ ��Ȳ�� ���׸��ϰ� ������ �� �ֵ��� �Ѵ�.
		TCHAR szBuffer[MSG_BUFSIZE] = { 0, };
		_stprintf_s(szBuffer, _countof(szBuffer), _T("[TCP ����] ���� �߻� -- WSASend() :"));
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
			SOCKET _Socket_Data = psSocket->m_Socket;
			Delete_in_Socket_List(psSocket);
			Reuse_Socket(_Socket_Data);
			Available_Overlapped->ResetObject(psOverlapped);
			return;
		}

		// ������ �����͸� ���ϰ� �Բ� �������ش�.
		Network_Message* _Net_Msg = new Network_Message;
		_Net_Msg->Socket = psSocket->m_Socket;
		_Net_Msg->Packet = cpcHeader;

		// ������ ��Ŷ�� ť�� �־�ΰ� ���߿� ó���Ѵ�.
		Recv_Data_Queue.push(_Net_Msg);

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
		SOCKET _Socket_Data = psSocket->m_Socket;
		Delete_in_Socket_List(psSocket);
		Reuse_Socket(_Socket_Data);
		Available_Overlapped->ResetObject(psOverlapped);
		return;
	}
}

void Server::IOFunction_Send(DWORD dwNumberOfBytesTransferred, Overlapped_Struct* psOverlapped, Socket_Struct* psSocket)
{
	printf_s("[TCP ����] [%15s:%5d] ��Ŷ �۽� �Ϸ� -> %d ����Ʈ\n", psSocket->IP.c_str()
		, psSocket->PORT, dwNumberOfBytesTransferred);

	delete psOverlapped;
}

void Server::IOFunction_Accept(Overlapped_Struct* psOverlapped)
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

	TCHAR szBuffer[STRUCT_BUFSIZE] = { 0, };
	TCHAR Error_Buffer[MSG_BUFSIZE] = { 0, };

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

	printf_s("[TCP ����] [%15s:%5d] [Socket_Number:%d] Ŭ���̾�Ʈ�� ����\n", psSocket->IP.c_str(), psSocket->PORT, psSocket->m_Socket);

	/// WSARecv�� �ɾ�־� ������ ���� �� �ְ���?!
	if (!Reserve_WSAReceive(psSocket->m_Socket))
	{
		// Ŭ���̾�Ʈ ����
		SOCKET _Socket_Data = psSocket->m_Socket;
		Delete_in_Socket_List(psSocket);
		Reuse_Socket(_Socket_Data);
		Available_Overlapped->ResetObject(psOverlapped);
	}
}

void Server::IOFunction_Disconnect(Overlapped_Struct* psOverlapped)
{
	/// ������ ����ϱ����� ���۸� ������ ��.
	TCHAR Error_Buffer[MSG_BUFSIZE] = { 0, };

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
