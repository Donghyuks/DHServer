#include "DHClient.h"

DHClient::DHClient()
{
	/// CS 초기화.
	InitializeCriticalSection(&g_CCS);

	TCHAR Error_Buffer[ERROR_MSG_BUFIZE] = { 0, };

	/// 소켓의 구조체를 생성한다.
	g_Server_Socket = std::make_shared<Socket_Struct>();

	/// IOCP를 생성한다.
	g_IOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, CLIENT_IOCP_THREAD_COUNT);
	assert(nullptr != g_IOCP);

	/// 사용가능한 Overlapped를 생성해둔다. (100개로 생성해둠)
	Available_Overlapped = new ObjectPool<Overlapped_Struct>(CLIENT_OVERLAPPED_COUNT);

	/// CLIENT_THREAD_COUNT 개수만큼 WorkThread를 생성한다.
	CreateWorkThread();
}

DHClient::~DHClient()
{
	if (g_IOCP != nullptr)
	{
		End();
	}
}

void DHClient::SetDebug(unsigned short _Debug_Option)
{
	g_Debug_Option = _Debug_Option;
}

BOOL DHClient::Send(Packet_Header* Send_Packet, int _SendType, SOCKET _Socket /*= INVALID_SOCKET*/)
{
	assert(nullptr != Send_Packet);
	if (INVALID_SOCKET == g_Server_Socket->m_Socket)
	{
		return LOGIC_FAIL;
	}

	// 등록되지 않은 패킷은 전송할 수 없다.
	// [클라] -> [서버]
	if (PACKET_TYPE_NONE == (unsigned short)Send_Packet->Packet_Type)
	{
		return LOGIC_FAIL;
	}

	// 받은 패킷을 복사해서 Send 큐에 넣음.
	size_t Send_Packet_Total_Size = PACKET_HEADER_SIZE + Send_Packet->Packet_Size;
	Packet_Header* Copy_Packet = reinterpret_cast<Packet_Header*>(malloc(Send_Packet_Total_Size));
	memcpy_s(Copy_Packet, Send_Packet_Total_Size, Send_Packet, Send_Packet_Total_Size);

	// 보낼 메세지를 큐에 넣어서 한번에 보냄 ( 최대한 효율적으로 자원을 사용하기 위해 )
	Send_Data_Queue.push(Copy_Packet);

	return LOGIC_SUCCESS;
}

BOOL DHClient::Recv(std::vector<Network_Message>& _Message_Vec)
{
	// 큐가 비었으면 FALSE를 반환한다.
	if (Recv_Data_Queue.empty())
		return LOGIC_FAIL;

	// 큐에서 데이터를 빼온다.
	Network_Message* _Net_Msg = nullptr;

	// 큐에 저장되어있는 모든 메세지를 담아서 반환.
	while (!Recv_Data_Queue.empty())
	{
		Recv_Data_Queue.try_pop(_Net_Msg);
		// 빼온 데이터를 넣어서 보냄.
		_Message_Vec.push_back(*_Net_Msg);

		delete _Net_Msg;
		_Net_Msg = nullptr;
	}

	// 큐에 데이터를 다 빼면 TRUE 반환.
	return LOGIC_SUCCESS;
}

BOOL DHClient::Connect(unsigned short _Port, std::string _IP)
{
	if (g_Connect_Send_Client_Thread != nullptr)
	{
		return Is_Server_Connect_Success;
	}

	/// 해당 포트와 IP 설정.
	PORT = _Port; IP = _IP;
	/// 클라이언트의 연결로직을 실행할 쓰레드 생성. (재접속 시도를 계속 하기위해서)
	g_Connect_Send_Client_Thread = new std::thread(std::bind(&DHClient::ConnectSendThread, this));

	return false;
}

BOOL DHClient::End()
{
	PostQueuedCompletionStatus(g_IOCP, 0, 0, nullptr);
	g_Is_Exit = true;

	// Send & Connect 쓰레드 종료.
	g_Connect_Send_Client_Thread->join();
	delete g_Connect_Send_Client_Thread;

	for (auto k : g_Work_Thread)
	{
		k->join();
		delete k;
	}

	g_Work_Thread.clear();

	// Send/Recv 큐 초기화.
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

	// IOCP 종료
	CloseHandle(g_IOCP);
	g_IOCP = nullptr;

	// 윈속 종료
	Safe_CloseSocket();
	WSACleanup();

	// 오버랩드 종료
	delete Available_Overlapped;

	/// 클라이언트가 종료될 때 같이 CS도 해제.
	DeleteCriticalSection(&g_CCS);
	g_Server_Socket.reset();

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
			Safe_CloseSocket();
			printf_s("[TCP 클라이언트] 오버랩드 결과를 체크하는 도중 서버와 연결이 끊겼습니다.\n");
			Is_Server_Connect_Success = false;
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

				Available_Overlapped->ResetObject(psOverlapped);
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

void DHClient::ConnectSendThread()
{
	while (!g_Is_Exit)
	{
		/// 만약 이미 서버가 연결이 되어있다면
		if (Is_Server_Connect_Success)
		{
			/// Connect가 된 쓰레드를 Send하는 쓰레드로써 재활용한다. (불필요한 쓰레드 낭비 줄이기!)
			SendFunction();
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
				TCHAR szBuffer[ERROR_MSG_BUFIZE] = { 0, };
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
				int Last_Error = WSAGetLastError();

				if (Last_Error == WSAECONNREFUSED)
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

		/// 두개로 나눈이유는 Socket에 대한 포인터참조가 일어날 경우가 있을수도 있으니?! 확실한 안전빵으루다가.
		Is_Server_Connect_Success = true;

		printf_s("[TCP 클라이언트] [%15s:%d] [SOCKET : %d] 서버 접속 성공\n", IP.c_str(), PORT, (int)g_Server_Socket->m_Socket);

	}
}

void DHClient::SendFunction()
{
	// 보낼 패킷의 총 사이즈 (헤더 + 실제 버퍼에 들어있는 패킷 사이즈)
	size_t Total_Packet_Size = 0;
	// Buffer를 얼만큼 위치에서 잘라야하는지 나타내기위함.
	size_t Buff_Offset = 0;
	// 데이터를 Queue 에서 빼오기 위한 변수.
	Packet_Header* Send_Packet = nullptr;

	// 보낼 메세지에 있는 큐에서 데이터가 있는지 검사
	while (!Send_Data_Queue.empty())
	{
		if (g_Server_Socket->m_Socket == INVALID_SOCKET)
		{
			break;
		}

		// 큐에서 보낼 데이터를 빼온다.
		Send_Data_Queue.try_pop(Send_Packet);

		// 초기 셋팅.
		Buff_Offset = 0;
		Total_Packet_Size = PACKET_HEADER_SIZE + Send_Packet->Packet_Size;

		// 만약 패킷의 사이즈가 준비된 버퍼보다 크다면 잘라서 여러개로 보내준다.
		while (Total_Packet_Size > 0)
		{
			// 오버랩드 셋팅
			Overlapped_Struct* psOverlapped = Available_Overlapped->GetObject();
			psOverlapped->m_IOType = Overlapped_Struct::IOType::IOType_Send;
			psOverlapped->m_Socket = g_Server_Socket->m_Socket;

			// 패킷 복사
			// 패킷이 아직 오버랩드 버퍼보다 사이즈가 큰 경우
			if (Total_Packet_Size >= OVERLAPPED_BUFIZE)
			{
				psOverlapped->m_Data_Size = OVERLAPPED_BUFIZE;
				Total_Packet_Size -= OVERLAPPED_BUFIZE;
				memcpy_s(psOverlapped->m_Buffer, OVERLAPPED_BUFIZE, (char*)Send_Packet + (Buff_Offset * OVERLAPPED_BUFIZE), OVERLAPPED_BUFIZE);
			}
			// 오버랩드 버퍼보다 사이즈가 작은 경우
			else
			{
				psOverlapped->m_Data_Size = Total_Packet_Size;
				memcpy_s(psOverlapped->m_Buffer, OVERLAPPED_BUFIZE, (char*)Send_Packet + (Buff_Offset * OVERLAPPED_BUFIZE), Total_Packet_Size);
				Total_Packet_Size = 0;
			}


			Buff_Offset++;

			// WSABUF 셋팅
			psOverlapped->m_WSABUF.len = (ULONG)psOverlapped->m_Data_Size;

			// WSASend() 오버랩드 걸기
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
				/// TCHAR을 통해 유니코드/멀티바이트의 가변적 상황에 제네릭하게 동작할 수 있도록 한다.
				TCHAR szBuffer[ERROR_MSG_BUFIZE] = { 0, };
				_stprintf_s(szBuffer, _countof(szBuffer), _T("[TCP 클라이언트] 에러 발생 -- WSASend() :"));
				err_display(szBuffer);
				Safe_CloseSocket();
				Available_Overlapped->ResetObject(psOverlapped);
				break;
			}

		}
		// 다 처리한 데이터 해제.
		delete Send_Packet;
	}

	// 보낼 메세지가 없을때는 다른 쓰레드에게 점유권을 넘겨줌.
	Sleep(0);
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

	int Exit_Socket_Num = (int)g_Server_Socket->m_Socket;

	shutdown(g_Server_Socket->m_Socket, SD_BOTH);
	closesocket(g_Server_Socket->m_Socket);
	g_Server_Socket->m_Socket = INVALID_SOCKET;
		
	LeaveCriticalSection(&g_CCS);
	
	printf_s("[TCP 클라이언트] [%15s:%d] [SOCKET : %d] 서버 접속 종료\n", IP.c_str(), PORT, Exit_Socket_Num);

	Is_Server_Connect_Success = false;
}

bool DHClient::BackUp_Overlapped(Overlapped_Struct* psOverlapped)
{
	if ((psOverlapped->m_Processed_Packet_Size + psOverlapped->m_Data_Size) > MAX_PACKET_SIZE)
	{
		printf_s("[TCP 클라이언트] 서버로부터 [%d]Byte 를 초과하는 패킷 처리가 들어왔습니다. \n", MAX_PACKET_SIZE);
		assert(LOGIC_FAIL);

		return false;
	}

	// 지금 들어온 데이터에 대한 백업을 해놓는다.
	memcpy_s(psOverlapped->m_Processing_Packet_Buffer + psOverlapped->m_Processed_Packet_Size, MAX_PACKET_SIZE - psOverlapped->m_Processed_Packet_Size,
		psOverlapped->m_Buffer, psOverlapped->m_Data_Size);

	// 수신한 데이터들 크기를 누적 시켜준다.
	psOverlapped->m_Processed_Packet_Size += psOverlapped->m_Data_Size;

	return true;
}

bool DHClient::Reserve_WSAReceive(SOCKET socket, Overlapped_Struct* psOverlapped /* = nullptr */)
{
	assert(INVALID_SOCKET != socket);

	BOOL bRecycleOverlapped = TRUE;

	// 사용할 오버랩드를 받지 않았으면 생성
	if (nullptr == psOverlapped)
	{
		psOverlapped = Available_Overlapped->GetObject();
		bRecycleOverlapped = FALSE;
	}

	// 오버랩드 셋팅
	psOverlapped->m_IOType = Overlapped_Struct::IOType::IOType_Recv;
	psOverlapped->m_Socket = socket;

	// WSABUF 셋팅
	psOverlapped->m_WSABUF.len = sizeof(psOverlapped->m_Buffer);

	// WSARecv() 오버랩드 걸기
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
		/// TCHAR을 통해 유니코드/멀티바이트의 가변적 상황에 제네릭하게 동작할 수 있도록 한다.
		TCHAR szBuffer[ERROR_MSG_BUFIZE] = { 0, };
		_stprintf_s(szBuffer, _countof(szBuffer), _T("[TCP 클라이언트] 에러 발생 -- WSARecv() :"));
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
	// 패킷에 아무런 설정을 하지않고 보냈다면 잘못 된 패킷이다.
	if (PACKET_TYPE_NONE == (unsigned short)_Data_Packet->Packet_Type)
	{
		// 받은 데이터 삭제.
		delete _Data_Packet;
		_Data_Packet = nullptr;
		// 클라이언트 종료
		SOCKET _Socket_Data = _Socket_Struct->m_Socket;
		Safe_CloseSocket();
		Available_Overlapped->ResetObject(_Overlapped_Struct);
		return LOGIC_FAIL;
	}

	// 수신한 데이터를 소켓과 함께 저장해준다.
	Network_Message* _Net_Msg = new Network_Message;
	_Net_Msg->Socket = _Socket_Struct->m_Socket;
	_Net_Msg->Packet = _Data_Packet;

	// 수신한 패킷을 큐에 넣어두고 나중에 처리한다.
	Recv_Data_Queue.push(_Net_Msg);

	// 데이터들을 이번에 처리한만큼 당긴다.
	memcpy_s(_Overlapped_Struct->m_Processing_Packet_Buffer, MAX_PACKET_SIZE,
		_Overlapped_Struct->m_Processing_Packet_Buffer + _Pull_Size, MAX_PACKET_SIZE - _Pull_Size);

	// 처리한 패킷 크기만큼 처리할량 감소
	_Overlapped_Struct->m_Processed_Packet_Size -= _Pull_Size;

	return LOGIC_SUCCESS;
}

void DHClient::IOFunction_Recv(DWORD dwNumberOfBytesTransferred, Overlapped_Struct* psOverlapped, Socket_Struct* psSocket)
{
	// 아무 데이터도 받지 못했으면 Overlapped 재사용.
	if (dwNumberOfBytesTransferred <= 0)
	{
		// 수신 걸기( 이번에 사용한 오버랩드를 다시 사용 )
		if (!Reserve_WSAReceive(psOverlapped->m_Socket, psOverlapped))
		{
			// 클라이언트 종료
			Safe_CloseSocket();
			Available_Overlapped->ResetObject(psOverlapped);
		}
		return;
	}

	if (g_Debug_Option == DHDEBUG_DETAIL)
	{
		printf_s("[TCP 클라이언트] [%d Byte] 패킷 수신 완료\n", dwNumberOfBytesTransferred);
	}

	// 이번에 받은 데이터 양
	psOverlapped->m_Data_Size = dwNumberOfBytesTransferred;

	// 이번에 받아온 버퍼 데이터만큼 처리할 버퍼로 옮긴다.
	if (BackUp_Overlapped(psOverlapped) == false)
	{
		// 클라이언트 종료
		Safe_CloseSocket();
		Available_Overlapped->ResetObject(psOverlapped);
		return;
	}

	// 처리할 데이터가 있으면 처리
	while (psOverlapped->m_Processed_Packet_Size > 0)
	{
		// header 를 다 받지 못했다. 이어서 recv()
		if (PACKET_HEADER_SIZE > psOverlapped->m_Processed_Packet_Size)
		{
			break;
		}

		// 패킷 헤더에 대한 포인터
		Packet_Header* Packet_header = nullptr;

		// 버퍼에 들어온 데이터는 가변길이 사이즈이다. (size_t 로 치환하는 이유는, 패킷의 사이즈를 size_t 값으로 저장해서 들어오기 때문이다.)
		size_t Packet_Body_Size = *reinterpret_cast<size_t*>(psOverlapped->m_Processing_Packet_Buffer);

		// 총 패킷 사이즈.
		size_t Packet_Total_Size = PACKET_HEADER_SIZE + Packet_Body_Size;

		// 하나의 패킷을 다 받지 못했다. 이어서 recv()
		if (Packet_Total_Size > psOverlapped->m_Processed_Packet_Size)
		{
			break;
		}

		// 서버로부터 수신한 패킷(완성된 패킷)
		Packet_header = reinterpret_cast<Packet_Header*>(malloc(Packet_Total_Size));
		memcpy_s(Packet_header, Packet_Total_Size, psOverlapped->m_Processing_Packet_Buffer, Packet_Total_Size);

		if (Push_RecvData(Packet_header, psSocket, psOverlapped, Packet_Total_Size) == false)
		{
			return;
		}
	}

	// 수신 걸기( 이번에 사용한 오버랩드를 다시 사용 )
	if (!Reserve_WSAReceive(psOverlapped->m_Socket, psOverlapped))
	{
		// 클라이언트 종료
		SOCKET _Socket_Data = psSocket->m_Socket;
		Safe_CloseSocket();
		Available_Overlapped->ResetObject(psOverlapped);
		return;
	}
}

void DHClient::IOFunction_Send(DWORD dwNumberOfBytesTransferred, Overlapped_Struct* psOverlapped, Socket_Struct* psSocket)
{
	if (g_Debug_Option == DHDEBUG_DETAIL)
	{
		printf_s("[TCP 클라이언트] [%15s:%d] [SOCKET : %d] [%d Byte] 패킷 송신 완료\n", IP.c_str(), PORT, (int)psOverlapped->m_Socket, dwNumberOfBytesTransferred);
	}

	Available_Overlapped->ResetObject(psOverlapped);
}
