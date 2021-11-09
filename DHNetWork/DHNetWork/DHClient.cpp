#include "DHClient.h"

DHClient::DHClient()
{

}

DHClient::~DHClient()
{
	/// 클라이언트가 종료될 때 같이 CS도 해제.
	DeleteCriticalSection(&g_CCS);
	g_Server_Socket.reset();
}

bool DHClient::Start()
{
	/// CS 초기화.
	InitializeCriticalSection(&g_CCS);

	TCHAR Error_Buffer[MSG_BUFSIZE] = { 0, };

	/// 소켓의 구조체를 생성한다.
	g_Server_Socket = std::make_shared<Socket_Struct>();

	/// IOCP를 생성한다.
	g_IOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	assert(nullptr != g_IOCP);

	/// 클라이언트의 종료를 체크하기 위한 쓰레드 생성.
	//g_Exit_Check_Thread = new std::thread(std::bind(&DHClient::ExitThread, this));

	/// CLIENT_THREAD_COUNT 개수만큼 WorkThread를 생성한다.
	CreateWorkThread();

	printf_s("[TCP 클라이언트] 시작\n");

	return LOGIC_SUCCESS;
}

bool DHClient::Send(Packet_Header* Send_Packet)
{
	assert(INVALID_SOCKET != g_Server_Socket->m_Socket);
	assert(nullptr != Send_Packet);

	// 등록되지 않은 패킷은 전송할 수 없다.
	// [클라] -> [서버]
	if (C2S_Packet_Type_MAX <= Send_Packet->Packet_Type)
	{
		return FALSE;
	}

	// 오버랩드 셋팅
	Overlapped_Struct* psOverlapped = new Overlapped_Struct;
	psOverlapped->m_IOType = Overlapped_Struct::IOType::IOType_Send;
	psOverlapped->m_Socket = g_Server_Socket->m_Socket;

	// 패킷 복사
	psOverlapped->m_Data_Size = 2 + Send_Packet->Packet_Size;

	if (sizeof(psOverlapped->m_Buffer) < psOverlapped->m_Data_Size)
	{
		delete psOverlapped;
		return FALSE;
	}

	memcpy_s(psOverlapped->m_Buffer, sizeof(psOverlapped->m_Buffer), Send_Packet, psOverlapped->m_Data_Size);

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
		_stprintf_s(szBuffer, _countof(szBuffer), _T("[TCP 클라이언트] 에러 발생 -- WSASend() :"));
		err_display(szBuffer);

		Safe_CloseSocket();
		delete psOverlapped;
	}

	return TRUE;
}

bool DHClient::Recv(std::vector<Network_Message*>& _Message_Vec)
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

bool DHClient::Connect(unsigned short _Port, std::string _IP)
{
	if (g_Connect_Client_Thread != nullptr)
	{
		return Is_Server_Connect_Success;
	}

	/// 해당 포트와 IP 설정.
	PORT = _Port; IP = _IP;
	/// 클라이언트의 연결로직을 실행할 쓰레드 생성. (재접속 시도를 계속 하기위해서)
	g_Connect_Client_Thread = new std::thread(std::bind(&DHClient::ConnectThread, this));

	return false;
}

bool DHClient::End()
{
	PostQueuedCompletionStatus(g_IOCP, 0, 0, nullptr);
	g_Is_Exit = true;

	//	Connect 쓰레드 종료.
	g_Connect_Client_Thread->join();

	for (auto k : g_Work_Thread)
	{
		k->join();
	}

	g_Work_Thread.clear();

	// IOCP 종료
	CloseHandle(g_IOCP);
	g_IOCP = nullptr;

	// 윈속 종료
	Safe_CloseSocket();
	WSACleanup();

	printf_s("[TCP 클라이언트] 종료\n");

	return true;
}

void DHClient::CreateWorkThread()
{
	for (int i = 0; i < CLIENT_THREAD_COUNT; i++)
	{
		/// 클라이언트 작업을 하는 WorkThread들 생성.
		std::thread* Client_Work = new std::thread(std::bind(&DHClient::WorkThread, this));

		/// 쓰레드 관리자에 넣어주고..
		g_Work_Thread.push_back(Client_Work);

		/// 일해라 쓰레드들!!
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

		/// GetQueuedCompletionStatusEx 를 쓰게 되면서 새로 필요한 부분.
		OVERLAPPED_ENTRY Entry_Data[64];	// 담겨 올 엔트리의 데이터들. 최대 64개로 선언해둠.
		ULONG Entry_Count = sizeof(Entry_Data) / sizeof(OVERLAPPED_ENTRY);	// 가져올 데이터의 최대 개수 ( 64개 )
		ULONG Get_Entry_Count = 0;	// 실제로 몇 개의 엔트리를 가져 왔는지 기록하기 위한 변수.

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
			Entry_Count,
			&Get_Entry_Count,
			INFINITE,
			0);

		// 오버랩드 결과 체크
		if (!bSuccessed)
		{
			if (SOCKET_ERROR != shutdown(psOverlapped->m_Socket, SD_BOTH))
			{
				Safe_CloseSocket();
				printf_s("[TCP 클라이언트] 오버랩드 결과를 체크하는 도중 서버와 연결이 끊겼습니다.\n");
				Is_Server_Connect_Success = false;
			}

			delete psOverlapped;
			continue;
		}

		/// 실제로 받아온 Entry 데이터에 대하여
		for (ULONG i = 0; i < Get_Entry_Count; i++)
		{
			/// WSARead() / WSAWrite() 등에 사용된 Overlapped 데이터를 가져온다.
			psOverlapped = reinterpret_cast<Overlapped_Struct*>(Entry_Data[i].lpOverlapped);
			/// I/O에 사용된 데이터의 크기.
			dwNumberOfBytesTransferred = Entry_Data[i].dwNumberOfBytesTransferred;
			/// Key값으로 넘겨줬었던 Socket_Struct 데이터.
			psSocket = reinterpret_cast<Socket_Struct*>(Entry_Data[i].lpCompletionKey);

			// 키가 nullptr 일 경우 쓰레드 종료를 의미
			if (!psSocket)
			{
				// 다른 WorkerThread() 의 종료를 위해서 PostQueue를 호출해주고 
				// 현재의 Thread를 종료한다.
				PostQueuedCompletionStatus(g_IOCP, 0, 0, nullptr);
				return;
			}

			assert(nullptr != psOverlapped);

			// 연결 종료
			if (0 == dwNumberOfBytesTransferred)
			{
				if (SOCKET_ERROR != shutdown(psOverlapped->m_Socket, SD_BOTH))
				{
					Safe_CloseSocket();
				}

				delete psOverlapped;
				continue;
			}

			/// Overlapped 데이터가 있을 때
			if (psOverlapped)
			{
				// Overlapped I/O 처리
				switch (psOverlapped->m_IOType)
				{
				case Overlapped_Struct::IOType::IOType_Recv: IOFunction_Recv(dwNumberOfBytesTransferred, psOverlapped, psSocket); break; // WSARecv() 의 Overlapped I/O 완료에 대한 처리
				case Overlapped_Struct::IOType::IOType_Send: IOFunction_Send(dwNumberOfBytesTransferred, psOverlapped, psSocket); break; // WSASend() 의 Overlapped I/O 완료에 대한 처리
				}
			}
		}
	}
}

void DHClient::ConnectThread()
{
	while (!g_Is_Exit)
	{
		/// 만약 이미 서버가 연결이 되어있다면
		if (g_Server_Socket->Is_Connected == true)
		{
			/// 현재 쓰레드는 돌 필요가 없으니까 다른 쓰레드에게 점유권을 넘겨주고 while 루프를 돈다.
			Sleep(0);
			continue;
		}

		if (INVALID_SOCKET == g_Server_Socket->m_Socket)
		{
			/// WSASocket 설정.
			g_Server_Socket->m_Socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
			assert(0 != g_Server_Socket->m_Socket);

			if (INVALID_SOCKET == g_Server_Socket->m_Socket)
			{
				/// TCHAR을 통해 유니코드/멀티바이트의 가변적 상황에 제네릭하게 동작할 수 있도록 한다.
				TCHAR szBuffer[MSG_BUFSIZE] = { 0, };
				_stprintf_s(szBuffer, _countof(szBuffer), _T("[TCP 클라이언트] 에러 발생 -- WSASocket() :"));
				err_display(szBuffer);
				break;
			}
		}

		printf_s("[TCP 클라이언트] 연결 시도\n");

		/// connect 작업 시도.
		while (!g_Is_Exit)
		{
			/// 지정된 PORT와 IP로 접속을 시도한다.
			SOCKADDR_IN serveraddr;
			ZeroMemory(&serveraddr, sizeof(serveraddr));
			serveraddr.sin_family = AF_INET;
			serveraddr.sin_port = htons(PORT);
			/// IPv4 기반의 address 가져오기.
			inet_pton(AF_INET, IP.c_str(), &(serveraddr.sin_addr.s_addr));

			if (SOCKET_ERROR == connect(g_Server_Socket->m_Socket, reinterpret_cast<SOCKADDR*>(&serveraddr), sizeof(serveraddr)))
			{
				if (WSAGetLastError() == WSAECONNREFUSED)
				{
					printf_s("[TCP 클라이언트] 서버 연결 재시도\n");
					continue;
				}

				Safe_CloseSocket();
			}

			break;
		}

		/// 통신을 진행해도 될지 체크
		// 1. 종료 플래그가 켜져있거나
		// 2. 소켓이 할당되어있지 않다면 IOCP에 등록하지 않는다.
		if ((g_Is_Exit) || (INVALID_SOCKET == g_Server_Socket->m_Socket))
		{
			continue;
		}

		/// 소켓을 IOCP 에 키 값과 함께 등록
		if (CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_Server_Socket->m_Socket), g_IOCP, reinterpret_cast<ULONG_PTR>(g_Server_Socket.get()), 0) != g_IOCP)
		{
			/// 만약 등록에 실패한다면 소켓 종료.
			Safe_CloseSocket();
			continue;
		}

		/// WSARecv() 걸어두기.
		if (!Reserve_WSAReceive(g_Server_Socket->m_Socket))
		{
			Safe_CloseSocket();
			continue;
		}

		/// 연결이 되었다는 flag
		g_Server_Socket->Is_Connected = TRUE;
		/// 두개로 나눈이유는 Socket에 대한 포인터참조가 일어날 경우가 있을수도 있으니?! 확실한 안전빵으루다가.
		Is_Server_Connect_Success = true;
		printf_s("[TCP 클라이언트] 서버와 연결 완료\n");

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
	/// 만약 2개 이상의 함수가 CloseSocket 함수를 호출하게 될 경우가 생길 수도 있으니까..
	EnterCriticalSection(&g_CCS);

	while (!g_Server_Socket.unique())
	{
		/// 다른 곳에 참조되고 있는 부분이 있다면, 종료가 될 때까지 기다린다.
		Sleep(0);
	}

	if (g_Server_Socket.unique())
	{
		shutdown(g_Server_Socket->m_Socket, SD_BOTH);
		closesocket(g_Server_Socket->m_Socket);
		g_Server_Socket->m_Socket = INVALID_SOCKET;
		g_Server_Socket->Is_Connected = FALSE;

		printf_s("[TCP 클라이언트] 서버와 연결 종료\n");
	}

	LeaveCriticalSection(&g_CCS);
}

bool DHClient::Reserve_WSAReceive(SOCKET socket, Overlapped_Struct* psOverlapped /* = nullptr */)
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

	if ((SOCKET_ERROR == iResult) && (WSAGetLastError() != WSA_IO_PENDING))
	{
		/// TCHAR을 통해 유니코드/멀티바이트의 가변적 상황에 제네릭하게 동작할 수 있도록 한다.
		TCHAR szBuffer[MSG_BUFSIZE] = { 0, };
		_stprintf_s(szBuffer, _countof(szBuffer), _T("[TCP 클라이언트] 에러 발생 -- WSARecv() :"));
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
	printf_s("[TCP 클라이언트] 패킷 수신 완료 <- %d 바이트\n", dwNumberOfBytesTransferred);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 패킷 처리
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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

		// 클라이언트로부터 수신한 패킷
		Packet_Header* cpcHeader = reinterpret_cast<Packet_Header*>(malloc(usPacketSize));
		memcpy_s(cpcHeader, usPacketSize, psOverlapped->m_Buffer, usPacketSize);

		Network_Message* Net_Msg = new Network_Message;
		Net_Msg->Socket = psSocket->m_Socket;
		Net_Msg->Packet = cpcHeader;

		// 수신한 패킷을 큐에 넣어두고 나중에 처리한다.
		Recv_Data_Queue.push(Net_Msg);

		// 데이터들을 이번에 처리한만큼 당긴다.
		memcpy_s(psOverlapped->m_Buffer, psOverlapped->m_Data_Size,
			psOverlapped->m_Buffer + usPacketSize, psOverlapped->m_Data_Size - usPacketSize);

		// 처리한 패킷 크기만큼 처리할량 감소
		psOverlapped->m_Data_Size -= usPacketSize;
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// 수신 걸기( 이번에 사용한 오버랩드를 다시 사용 )
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
	printf_s("[TCP 클라이언트][SOCKET %d] 패킷 송신 완료 -> %d 바이트\n", (int)psOverlapped->m_Socket,dwNumberOfBytesTransferred);

	delete psOverlapped;
}
