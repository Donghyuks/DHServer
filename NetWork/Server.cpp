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
	/// 에러를 출력하기위한 버퍼를 생성해 둠. (메모리풀 사용)
	TCHAR* Error_Buffer = (TCHAR*)m_MemoryPool.GetMemory(MSG_BUFSIZE);

	/// IOCP를 생성한다.
	g_IOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	assert(nullptr != g_IOCP);

	/// 사용가능한 Overlapped를 생성해둔다. (2000개로 생성해둠)
	Available_Overlapped = new ObjectPool<Overlapped_Struct>(TOTAL_OVERLAPPED_COUNT);

	/// Worker Thread들은 보통 코어수 ~ 코어수*2 개 정도로 생성한다고 한다.
	/// 시스템의 정보를 받아와서 코어수의 *2개만큼 WorkThread를 생성한다.
	CreateWorkThread();

	/// 소켓의 구조체를 생성한다.
	g_Listen_Socket = std::make_shared<SOCKET>();

	/// Socket 생성.
	*g_Listen_Socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);

	if (INVALID_SOCKET == *g_Listen_Socket)
	{
		/// TCHAR을 통해 유니코드/멀티바이트의 가변적 상황에 제네릭하게 동작할 수 있도록 한다.
		_stprintf_s(Error_Buffer, MSG_BUFSIZE, _T("[TCP 서버] 에러 발생 -- WSASocket() :"));
		err_display(Error_Buffer);

		WSACleanup();
		return LOGIC_FAIL;
	}

	/// Listen 소켓을 IOCP에 등록을 해놔야 AcceptEx를 쓸수 있겠지!!!!!!!!!!!
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(*g_Listen_Socket), g_IOCP, 0, 0);

	/// bind
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(PORT);
	if (SOCKET_ERROR == bind(*g_Listen_Socket, reinterpret_cast<SOCKADDR*>(&serveraddr), sizeof(serveraddr)))
	{
		/// TCHAR을 통해 유니코드/멀티바이트의 가변적 상황에 제네릭하게 동작할 수 있도록 한다.
		_stprintf_s(Error_Buffer, MSG_BUFSIZE, _T("[TCP 서버] 에러 발생 -- bind() :"));
		err_display(Error_Buffer);

		closesocket(*g_Listen_Socket);
		*g_Listen_Socket = INVALID_SOCKET;

		WSACleanup();
		return LOGIC_FAIL;
	}

	/// listen ( 여기에 5개를 설정한다해서 5명만 받는게 아니다. 버퍼의 수임 )
	if (SOCKET_ERROR == listen(*g_Listen_Socket, 5))
	{
		/// TCHAR을 통해 유니코드/멀티바이트의 가변적 상황에 제네릭하게 동작할 수 있도록 한다.
		_stprintf_s(Error_Buffer, MSG_BUFSIZE, _T("[TCP 서버] 에러 발생 -- listen() :"));
		err_display(Error_Buffer);

		closesocket(*g_Listen_Socket);
		*g_Listen_Socket = INVALID_SOCKET;

		WSACleanup();
		return LOGIC_FAIL;
	}

	/// 클라이언트의 종료를 체크하기 위한 쓰레드 생성.
	g_Exit_Check_Thread = new std::thread(std::bind(&Server::ExitThread, this));

	printf_s("[TCP 서버] 시작\n");

	DWORD dwByte;	/// 처리 Byte

	/// 10개정도의 소켓을 추가로 만들어서 유저가 MAX_USER수를 초과한 경우를 처리한다.
	for (unsigned short i = 0; i < MAX_USER_COUNT + 10; i++)
	{
		/// 오버랩드 생성
		auto psOverlapped = Available_Overlapped->GetObject();

		psOverlapped->m_IOType = Overlapped_Struct::IOType::IOType_Accept;
		/// Socket 생성.
		psOverlapped->m_Socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);

		if (INVALID_SOCKET == psOverlapped->m_Socket)
		{
			/// TCHAR을 통해 유니코드/멀티바이트의 가변적 상황에 제네릭하게 동작할 수 있도록 한다.
			_stprintf_s(Error_Buffer, MSG_BUFSIZE, _T("[TCP 서버] 에러 발생 -- Accept 소켓 생성중 오류 발생 :"));
			err_display(Error_Buffer);

			WSACleanup();
			return LOGIC_FAIL;
		}

		/// AcceptEx 함수을 사용하여 소켓을 Queue에 등록.
		BOOL Return_Value = AcceptEx(*g_Listen_Socket,				// Listen 상태에 있는 소켓, 해당 소켓에 오는 연결시도를 기다리게됨.
			psOverlapped->m_Socket,									// 들어오는 연결을 받는 소켓, 연결되거나 bound 되지 않아야 한다.
			&psOverlapped->m_Buffer[0],								// 새로 들어온 연결에서 받은 첫번째 데이터 블록의 버퍼, 서버의 로컬주소, 클라이언트의 리모트 주소
			0,														// 실제 데이터의 길이
			sizeof(sockaddr_in) + IP_SIZE,							// 로컬 주소 정보의 바이트 수
			sizeof(sockaddr_in) + IP_SIZE,							// 리모트 주소 정보의 바이트 수
			&dwByte,												// 받은 바이트의 개수
			psOverlapped							// Overlapped 구조체
		);

		if (ERROR_IO_PENDING != WSAGetLastError())
		{
			wprintf(L"Create accept socket failed with error: %u\n", WSAGetLastError());
			_stprintf_s(Error_Buffer, MSG_BUFSIZE, _T("[TCP 서버] 에러 발생 -- AcceptEx 함수"));
			err_display(Error_Buffer);
			WSACleanup();
			return LOGIC_FAIL;
		}

		// 클라이언트 구조체 생성 및 등록
		Socket_Struct* psSocket = new Socket_Struct;
		psSocket->m_Socket = psOverlapped->m_Socket;

		// IOCP에 소켓의 핸들과 같이 등록한다.
		if (CreateIoCompletionPort(reinterpret_cast<HANDLE>(psOverlapped->m_Socket), g_IOCP, reinterpret_cast<ULONG_PTR>(psSocket), 0) != g_IOCP)
		{
			wprintf(L"CreateIoCompletionPort Function error: %u\n", WSAGetLastError());
			_stprintf_s(Error_Buffer, MSG_BUFSIZE, _T("[TCP 서버] 에러 발생 -- CreateIoCompletionPort 함수"));
			err_display(Error_Buffer);
			assert(false);
			// 실패 시에 현재 생성한 소켓은 초기화.
			delete psSocket;
		}

		// 생성된 소켓 구조체는 1:1로 핸들과 맵핑되어 프로그램이 끝날때 까지 없어지지 않고 재활용.
		g_Socket_Struct_Pool.insert({ psSocket->m_Socket , psSocket });
	}

	// 사용한 메모리 반환.
	m_MemoryPool.ResetMemory(Error_Buffer, MSG_BUFSIZE);

	return LOGIC_SUCCESS;
}

bool Server::Send(Packet_Header* Send_Packet)
{
	/// 모든 클라이언트에게 메세지를 보냄.

	assert(nullptr != Send_Packet);

	// 임계 영역
	{
		// 전체 클라이언트 소켓에 패킷 송신
		for (auto it : g_Connected_Client)
		{
			// 아직 Map 구조체에 대해 해당 값에 lock이 걸려있지 않음.
			Client_Map::accessor m_Accessor;

			// 해당 자료에대한 lock. (다른데서 사용중이라면 기다림)
			while (!g_Connected_Client.find(m_Accessor, it.first)) {};

			// 패킷 송신
			SendTargetSocket(m_Accessor->second->m_Socket, Send_Packet);

			// 해당 자료에 대한 lock 해제.
			m_Accessor.release();
		}
	}

	return true;
}


bool Server::Recv(std::vector<Network_Message*>& _Message_Vec)
{
	/// 큐가 비었으면 FALSE를 반환한다.
	if (Recv_Data_Queue.empty())
		return FALSE;

	/// 큐에 저장되어있는 모든 메세지를 담아서 반환.
	while (!Recv_Data_Queue.empty())
	{
		/// 큐에서 데이터를 빼온다.
		Network_Message* _Net_Msg = nullptr;
		Recv_Data_Queue.try_pop(_Net_Msg);

		/// 빼온 데이터를 넣어서 보냄.
		_Message_Vec.emplace_back(_Net_Msg);
		/// 메세지 타입으로 Header 캐스팅.
		//C2S_Message* C2S_Msg = static_cast<C2S_Message*>(cpcHeader);

		//switch (cpcHeader->Packet_Type)
		//{
		//case C2S_Packet_Type_Message:         // 채팅 메세지
		//{
		//	/// 채팅 메세지가 있으면 MsgBuff에 저장해준다.
		//	memcpy_s(MsgBuff, MSG_BUFSIZE, C2S_Msg->Message_Buffer, MSG_BUFSIZE);
		//}
		//case C2S_Packet_Type_Data:
		//{
		//	/// 추후 필요시 구현..
		//}
		//break;
		//}

		// 해제.
		//delete cpcHeader;
	}

	/// 큐에 데이터를 다 빼면 TRUE 반환.
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

	// IOCP 종료
	CloseHandle(g_IOCP);
	g_IOCP = nullptr;

	// 생성된 전체 소켓에 대한 제거 작업.
	for (auto it : g_Socket_Struct_Pool)
	{
		// 아직 Map 구조체에 대해 해당 값에 lock이 걸려있지 않음.
		Client_Map::accessor m_Accessor;

		// 해당 자료에대한 lock. (다른데서 사용중이라면 기다림)
		while (!g_Socket_Struct_Pool.find(m_Accessor, it.first)) {};

		shutdown(m_Accessor->second->m_Socket, SD_BOTH);
		closesocket(m_Accessor->second->m_Socket);

		m_Accessor.release();
	}

	// 윈속 종료
	WSACleanup();

	printf_s("[TCP 서버] 종료\n");

	return true;
}

void Server::CreateWorkThread()
{
	SYSTEM_INFO SystemInfo;
	GetSystemInfo(&SystemInfo);
	int iThreadCount = SystemInfo.dwNumberOfProcessors * 2;

	for (int i = 0; i < iThreadCount; i++)
	{
		/// 클라이언트 작업을 하는 WorkThread들 생성.
		std::thread* Client_Work = new std::thread(std::bind(&Server::WorkThread, this));

		/// 쓰레드 관리자에 넣어주고..
		g_Work_Thread.push_back(Client_Work);

		/// 일해라 쓰레드들!!
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

		/// GetQueuedCompletionStatusEx 를 쓰게 되면서 새로 필요한 부분.
		OVERLAPPED_ENTRY Entry_Data[64];	// 담겨 올 엔트리의 데이터들. 최대 64개로 선언해둠.
		*Entry_Count = sizeof(Entry_Data) / sizeof(OVERLAPPED_ENTRY);	// 가져올 데이터의 최대 개수 ( 64개 )
		*Get_Entry_Count = 0;	// 실제로 몇 개의 엔트리를 가져 왔는지 기록하기 위한 변수.

		// GetQueuedCompletionStatus() - GQCS 라고 부름
		// WSARead(), WSAWrite() 등의 Overlapped I/O 관련 처리 결과를 받아오는 함수
		// PostQueuedCompletionStatus() 를 통해서도 GQCS 를 리턴시킬 수 있다.( 일반적으로 쓰레드 종료 처리 )

		/// GetQueuedCompletionStatusEx 함수 정리.
		/// 1. HANDLE hCompletionPort : 어떤 I/O 컴플리션 포트를 대기할 것인지를 결정하는 핸들 값을 전달. ( I/O 컴플리션 큐에 여러 개의 항목이 삽입되어 있을 때, 여러 개의 항목들을 한 번에 가져올 수 있다. )
		/// 2. LPOVERLAPPED_ENTRY pCompletionPortEntries : 각 항목들은 pCompletionPortEntries 배열을 통해 전달 된다.
		/// 3. ULONG ulCount : 몇 개의 항목을 pCompletionPortEntries로 복사해 올 것인지를 지정하는 값을 전달하면 된다.
		/// 4. PULONG pulNumEntriesRemoved : long 값으로 I/O 컴플리션 큐로부터 몇 개의 항목들을 실제로 가지고 왔는지를 받아오게 된다. bAlertable 이 FALSE로 설정되었다면 지정된 시간만큼 I/O 컴플리션 큐로 완료 통지가 삽입될 때까지 대기하게 된다.
		///										만약 TRUE로 설정되어 있따면 I/O 컴플리션 큐에 어떠한 완료 통지도 존재하지 않는 경우 쓰레드를 Alertable 상태로 전환한다.
		/// 5. DWORD dwMilliseconds : 기다릴 시간
		/// 6. BOOL bAlertable : Alertable 상태에 따른 처리는 4 항목을 참조.

		BOOL bSuccessed = GetQueuedCompletionStatusEx(g_IOCP,
			Entry_Data,
			*Entry_Count,
			Get_Entry_Count,
			INFINITE,
			0);

		// 오버랩드 결과 체크
		if (!bSuccessed)
		{
			printf_s("[TCP 서버] 오버랩드 결과를 체크하는 도중 서버와 연결이 끊겼습니다.\n");
			delete psSocket;
			delete psOverlapped;
			continue;
		}

		/// 실제로 받아온 Entry 데이터에 대하여
		for (ULONG i = 0; i < *Get_Entry_Count; i++)
		{
			/// WSARead() / WSAWrite() 등에 사용된 Overlapped 데이터를 가져온다.
			psOverlapped = reinterpret_cast<Overlapped_Struct*>(Entry_Data[i].lpOverlapped);

			if (!psOverlapped)
			{
				// 다른 WorkerThread() 들의 종료를 위해서
				PostQueuedCompletionStatus(g_IOCP, 0, 0, nullptr);
				break;
			}

			/// 만약 Disconnect 메세지가 왔다면.
			switch (psOverlapped->m_IOType)
			{
			case Overlapped_Struct::IOType::IOType_Disconnect:
			{
				IOFunction_Disconnect(psOverlapped);
				continue;	// Disconnect() 의 Overlapped I/O 완료에 대한 처리
			}
			case Overlapped_Struct::IOType::IOType_Accept: IOFunction_Accept(psOverlapped); continue; // Accept() 의 Overlapped I/O 완료에 대한 처리

			}
			/// I/O에 사용된 데이터의 크기.
			*dwNumberOfBytesTransferred = Entry_Data[i].dwNumberOfBytesTransferred;
			/// Key값으로 넘겨줬었던 Socket_Struct 데이터.
			psSocket = reinterpret_cast<Socket_Struct*>(Entry_Data[i].lpCompletionKey);

			// 키가 nullptr 일 경우 쓰레드 종료를 의미
			if (!psSocket)
			{
				// 다른 WorkerThread() 들의 종료를 위해서
				PostQueuedCompletionStatus(g_IOCP, 0, 0, nullptr);
				break;
			}

			assert(nullptr != psOverlapped);

			// 연결 종료
			if (0 == *dwNumberOfBytesTransferred)
			{
				SOCKET _Socket_Data = psSocket->m_Socket;
				Delete_in_Socket_List(psSocket);
				Reuse_Socket(_Socket_Data);
				Available_Overlapped->ResetObject(psOverlapped);
				continue;
			}

			// Overlapped I/O 처리
			switch (psOverlapped->m_IOType)
			{
			case Overlapped_Struct::IOType::IOType_Recv: IOFunction_Recv(*dwNumberOfBytesTransferred, psOverlapped, psSocket); break; // WSARecv() 의 Overlapped I/O 완료에 대한 처리
			case Overlapped_Struct::IOType::IOType_Send: IOFunction_Send(*dwNumberOfBytesTransferred, psOverlapped, psSocket); break; // WSASend() 의 Overlapped I/O 완료에 대한 처리
			}
		}
	}

	// 사용한 메모리 영역 반환.
	m_MemoryPool.ResetMemory(dwNumberOfBytesTransferred, sizeof(DWORD));
	m_MemoryPool.ResetMemory(Entry_Count, sizeof(ULONG));
	m_MemoryPool.ResetMemory(Get_Entry_Count, sizeof(ULONG));

	return;
}

bool Server::AddClientSocket(Socket_Struct* psSocket)
{
	assert(nullptr != psSocket);

	// 임계 영역
	{
		// 클라이언트 중복 검사
		for (auto it : g_Connected_Client)
		{
			{
				// const_accessor로 불필요한 락은 막음. (참조만 할때)
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

		// 중복이 아니라면 연결된 클라이언트 리스트로 등록.
		g_Connected_Client.insert({ psSocket->m_Socket, psSocket });
	}


	return TRUE;
}

void Server::Reuse_Socket(SOCKET _Socket)
{
	Client_Map::accessor m_accessor;

	// 해당 소켓번호에 대응하는 소켓 구조체를 가져온다.
	while (!g_Socket_Struct_Pool.find(m_accessor, _Socket)) {};

	// 소멸자를 호출하여 초기화.
	m_accessor->second = new (m_accessor->second) Socket_Struct;
	// 소켓 번호는 제활용. 어차피 key 값으로 소켓과 매칭되어 있으므로.
	m_accessor->second->m_Socket = _Socket;

	shutdown(_Socket, SD_BOTH);

	auto Disconnect_Overlapped = Available_Overlapped->GetObject();
	Disconnect_Overlapped->m_IOType = Overlapped_Struct::IOType::IOType_Disconnect;
	Disconnect_Overlapped->m_Socket = _Socket;
	Disconnect_EX(_Socket, Disconnect_Overlapped, TF_REUSE_SOCKET, 0);

	// 명시적 호출.
	m_accessor.release();
}

void Server::Delete_in_Socket_List(Socket_Struct* psSocket)
{
	std::string Socket_IP(psSocket->IP);
	auto Socket_PORT = psSocket->PORT;
	int Socket_Number = psSocket->m_Socket;

	for (auto it : g_Connected_Client)
	{
		// 아직 Map 구조체에 대해 해당 값에 lock이 걸려있지 않음.
		Client_Map::accessor m_Accessor;

		// 해당 자료에대한 lock. (다른데서 사용중이라면 기다림)
		while (!g_Connected_Client.find(m_Accessor, it.first)) {};

		/// 해당하는 포인터를 찾는다.
		if (m_Accessor->second == psSocket)
		{
			/// 해당하는 클라이언트 소켓을 리스트에서 삭제.
			g_Connected_Client.erase(m_Accessor);

			break;
		}
	}

	printf_s("[TCP 서버] [%15s:%5d] [Socket_Number:%d] 클라이언트와 종료\n", Socket_IP.c_str(), Socket_PORT, Socket_Number);

}

void Server::ExitThread()
{
	while (TRUE)
	{
		if (GetAsyncKeyState(VK_ESCAPE))
		{
			g_Is_Exit = true;

			/// 리슨소켓이 사용되고 있지 않다면..
			if (INVALID_SOCKET != *g_Listen_Socket)
			{
				/// 생각해보니까 얘는 몇개를 참조하고 있던 Accept에서 빠져나오기 위한거니까..
				/// unique일떄 지울 필요가 없자나?
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

	// 사용할 오버랩드를 받지 않았으면 생성
	if (nullptr == psOverlapped)
	{
		psOverlapped = new Overlapped_Struct;
		bRecycleOverlapped = FALSE;
	}

	// 오버랩드 셋팅
	psOverlapped->m_IOType = Overlapped_Struct::IOType::IOType_Recv;
	psOverlapped->m_Socket = socket;

	// WSABUF 셋팅
	WSABUF wsaBuffer;
	wsaBuffer.buf = psOverlapped->m_Buffer + psOverlapped->m_Data_Size;
	wsaBuffer.len = sizeof(psOverlapped->m_Buffer) - psOverlapped->m_Data_Size;

	// WSARecv() 오버랩드 걸기
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
		/// TCHAR을 통해 유니코드/멀티바이트의 가변적 상황에 제네릭하게 동작할 수 있도록 한다.
		TCHAR szBuffer[MSG_BUFSIZE] = { 0, };
		_stprintf_s(szBuffer, _countof(szBuffer), _T("[TCP 서버] 에러 발생 -- WSARecv() :"));
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

	// 등록되지 않은 패킷은 전송할 수 없다.
	// [서버] -> [클라]
	if (S2C_Packet_Type_MAX <= psPacket->Packet_Type)
	{
		return FALSE;
	}

	// 오버랩드 셋팅
	Overlapped_Struct* psOverlapped = new Overlapped_Struct;
	psOverlapped->m_IOType = Overlapped_Struct::IOType::IOType_Send;
	psOverlapped->m_Socket = socket;

	// 패킷 복사
	psOverlapped->m_Data_Size = 2 + psPacket->Packet_Size;

	if (sizeof(psOverlapped->m_Buffer) < psOverlapped->m_Data_Size)
	{
		delete psOverlapped;
		return FALSE;
	}

	memcpy_s(psOverlapped->m_Buffer, sizeof(psOverlapped->m_Buffer), psPacket, psOverlapped->m_Data_Size);

	// WSABUF 셋팅
	WSABUF wsaBuffer;
	wsaBuffer.buf = psOverlapped->m_Buffer;
	wsaBuffer.len = psOverlapped->m_Data_Size;

	// WSASend() 오버랩드 걸기
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
		/// TCHAR을 통해 유니코드/멀티바이트의 가변적 상황에 제네릭하게 동작할 수 있도록 한다.
		TCHAR szBuffer[MSG_BUFSIZE] = { 0, };
		_stprintf_s(szBuffer, _countof(szBuffer), _T("[TCP 서버] 에러 발생 -- WSASend() :"));
		err_display(szBuffer);

		delete psOverlapped;
		return FALSE;
	}

	return TRUE;
}

void Server::IOFunction_Recv(DWORD dwNumberOfBytesTransferred, Overlapped_Struct* psOverlapped, Socket_Struct* psSocket)
{
	printf_s("[TCP 서버] [%15s:%5d] 패킷 수신 완료 <- %d 바이트\n", psSocket->IP.c_str(), psSocket->PORT, dwNumberOfBytesTransferred);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	 // 패킷 처리
	 /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	 // 수신한 데이터들 크기를 누적 시켜준다.
	psOverlapped->m_Data_Size += dwNumberOfBytesTransferred;

	// 처리할 데이터가 있으면 처리
	while (psOverlapped->m_Data_Size > 0)
	{
		// header 크기는 2 바이트( 고정 )
		static const unsigned short cusHeaderSize = 2;

		// header 를 다 받지 못했다. 이어서 recv()
		if (cusHeaderSize > psOverlapped->m_Data_Size)
		{
			break;
		}

		// body 의 크기는 N 바이트( 가변 ), 패킷에 담겨있음
		unsigned short usBodySize = *reinterpret_cast<unsigned short*>(psOverlapped->m_Buffer);
		unsigned short usPacketSize = cusHeaderSize + usBodySize;

		// 하나의 패킷을 다 받지 못했다. 이어서 recv()
		if (usPacketSize > psOverlapped->m_Data_Size)
		{
			break;
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// 완성된 패킷을 처리
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// 클라이언트로부터 수신한 패킷
		Packet_Header* cpcHeader = reinterpret_cast<Packet_Header*>(malloc(usPacketSize));
		memcpy_s(cpcHeader, usPacketSize, psOverlapped->m_Buffer, usPacketSize);

		// 잘못된 패킷
		if (C2S_Packet_Type_MAX <= cpcHeader->Packet_Type)
		{
			// 클라이언트 종료
			SOCKET _Socket_Data = psSocket->m_Socket;
			Delete_in_Socket_List(psSocket);
			Reuse_Socket(_Socket_Data);
			Available_Overlapped->ResetObject(psOverlapped);
			return;
		}

		// 수신한 데이터를 소켓과 함께 저장해준다.
		Network_Message* _Net_Msg = new Network_Message;
		_Net_Msg->Socket = psSocket->m_Socket;
		_Net_Msg->Packet = cpcHeader;

		// 수신한 패킷을 큐에 넣어두고 나중에 처리한다.
		Recv_Data_Queue.push(_Net_Msg);

		// 데이터들을 이번에 처리한만큼 당긴다.
		memcpy_s(psOverlapped->m_Buffer, psOverlapped->m_Data_Size,
			psOverlapped->m_Buffer + usPacketSize, psOverlapped->m_Data_Size - usPacketSize);

		// 처리한 패킷 크기만큼 처리할량 감소
		psOverlapped->m_Data_Size -= usPacketSize;
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// 수신 걸기( 이번에 사용한 오버랩드를 다시 사용 )
	if (!Reserve_WSAReceive(psOverlapped->m_Socket, psOverlapped))
	{
		// 클라이언트 종료
		SOCKET _Socket_Data = psSocket->m_Socket;
		Delete_in_Socket_List(psSocket);
		Reuse_Socket(_Socket_Data);
		Available_Overlapped->ResetObject(psOverlapped);
		return;
	}
}

void Server::IOFunction_Send(DWORD dwNumberOfBytesTransferred, Overlapped_Struct* psOverlapped, Socket_Struct* psSocket)
{
	printf_s("[TCP 서버] [%15s:%5d] 패킷 송신 완료 -> %d 바이트\n", psSocket->IP.c_str()
		, psSocket->PORT, dwNumberOfBytesTransferred);

	delete psOverlapped;
}

void Server::IOFunction_Accept(Overlapped_Struct* psOverlapped)
{
	/// 만약 최대 유저수에 도달했다면 해당 소켓을 끊어줘야 한다.
	if (g_Connected_Client.size() == MAX_USER_COUNT)
	{
		int k = g_Connected_Client.size();
		// 클라이언트 종료
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


	/// 데이터의 변경이 생겨 락이 필요함. (해당 자료에 대한)
	Client_Map::accessor m_accessor;

	// 해당 소켓에 대한 소켓 구조체를 가져온다.
	while (!g_Socket_Struct_Pool.find(m_accessor, psOverlapped->m_Socket)) {};
	Socket_Struct* psSocket = m_accessor->second;

	/// IPv4 기반의 IP 등록하기.
	inet_ntop(AF_INET, &Remoteaddr->sin_addr, const_cast<PSTR>(psSocket->IP.c_str()), sizeof(psSocket->IP));
	psSocket->PORT = ntohs(Remoteaddr->sin_port);

	m_accessor.release();

	// 연결된 클라이언트 정보를 등록
	if (!AddClientSocket(psSocket))
	{
		printf_s("[TCP 서버] 클라이언트 정보 리스트에 등록 실패.\n");
		assert(false);
		return;
	}

	printf_s("[TCP 서버] [%15s:%5d] [Socket_Number:%d] 클라이언트가 접속\n", psSocket->IP.c_str(), psSocket->PORT, psSocket->m_Socket);

	/// WSARecv를 걸어둬야 정보를 받을 수 있겠죠?!
	if (!Reserve_WSAReceive(psSocket->m_Socket))
	{
		// 클라이언트 종료
		SOCKET _Socket_Data = psSocket->m_Socket;
		Delete_in_Socket_List(psSocket);
		Reuse_Socket(_Socket_Data);
		Available_Overlapped->ResetObject(psOverlapped);
	}
}

void Server::IOFunction_Disconnect(Overlapped_Struct* psOverlapped)
{
	/// 에러를 출력하기위한 버퍼를 생성해 둠.
	TCHAR Error_Buffer[MSG_BUFSIZE] = { 0, };

	/// 이미 Disconnect가 들어올때 Overlapped에 소켓이 등록되어있음.
	psOverlapped->m_IOType = Overlapped_Struct::IOType::IOType_Accept;

	DWORD dwByte;
	/// AcceptEx 함수을 사용하여 소켓을 Queue에 등록.
	BOOL Return_Value = AcceptEx(*g_Listen_Socket,				// Listen 상태에 있는 소켓, 해당 소켓에 오는 연결시도를 기다리게됨.
		psOverlapped->m_Socket,									// 들어오는 연결을 받는 소켓, 연결되거나 bound 되지 않아야 한다.
		&psOverlapped->m_Buffer[0],								// 새로 들어온 연결에서 받은 첫번째 데이터 블록의 버퍼, 서버의 로컬주소, 클라이언트의 리모트 주소
		0,														// 실제 데이터의 길이
		sizeof(sockaddr_in) + IP_SIZE,							// 로컬 주소 정보의 바이트 수
		sizeof(sockaddr_in) + IP_SIZE,							// 리모트 주소 정보의 바이트 수
		&dwByte,												// 받은 바이트의 개수
		psOverlapped							// Overlapped 구조체
	);

	if (ERROR_IO_PENDING != WSAGetLastError())
	{
		wprintf(L"Create accept socket failed with error: %u\n", WSAGetLastError());
		_stprintf_s(Error_Buffer, _countof(Error_Buffer), _T("[TCP 서버] 에러 발생 -- DisconnectEX 함수"));
		err_display(Error_Buffer);
		WSACleanup();
		assert(false);
		return;
	}
}
