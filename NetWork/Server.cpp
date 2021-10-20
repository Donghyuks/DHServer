#include "Server.h"

#include <assert.h>
#include <functional>

unsigned short Server::MAX_USER_COUNT = 0;

Server::Server()
{

}

Server::~Server()
{
	/// 클라이언트가 종료될 때 같이 CS도 해제.
	DeleteCriticalSection(&g_SCS);
	DeleteCriticalSection(&g_Aceept_CS);
}

bool Server::Start()
{
	/// 윈소켓 초기화.
	WSADATA wsa;
	/// CS 초기화.
	InitializeCriticalSection(&g_SCS);
	InitializeCriticalSection(&g_Aceept_CS);

	char Error_Buffer[PACKET_BUFSIZE] = { 0, };

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		sprintf_s(Error_Buffer, "[TCP 서버] 에러 발생 -- WSAStartup() :");
		err_display(Error_Buffer);

		return LOGIC_FAIL;
	}

	/// 소켓의 구조체를 생성한다.
	g_Listen_Socket = std::make_shared<SOCKET>();

	/// Socket 생성.
	*g_Listen_Socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);

	if (INVALID_SOCKET == *g_Listen_Socket)
	{
		sprintf_s(Error_Buffer, "[TCP 서버] 에러 발생 -- WSASocket() :");
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
		sprintf_s(Error_Buffer, "[TCP 서버] 에러 발생 -- bind() :");
		err_display(Error_Buffer);

		closesocket(*g_Listen_Socket);
		*g_Listen_Socket = INVALID_SOCKET;

		WSACleanup();
		return LOGIC_FAIL;
	}

	/// listen
	if (SOCKET_ERROR == listen(*g_Listen_Socket, 5))
	{
		sprintf_s(Error_Buffer, "[TCP 서버] 에러 발생 -- listen() :");
		err_display(Error_Buffer);

		closesocket(*g_Listen_Socket);
		*g_Listen_Socket = INVALID_SOCKET;

		WSACleanup();
		return LOGIC_FAIL;
	}


	/// IOCP를 생성한다.
	g_IOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	assert(nullptr != g_IOCP);

	/// Worker Thread들은 보통 코어수 ~ 코어수*2 개 정도로 생성한다고 한다.
	/// 시스템의 정보를 받아와서 코어수의 *2개만큼 WorkThread를 생성한다.
	CreateWorkThread();

	/// 클라이언트의 종료를 체크하기 위한 쓰레드 생성.
	g_Exit_Check_Thread = new std::thread(std::bind(&Server::ExitThread, this));

	printf_s("[TCP 서버] 시작\n");

	/// 클라이언트의 연결로직을 실행할 쓰레드 생성. (재접속 시도를 계속 하기위해서)
	g_Accept_Client_Thread = new std::thread(std::bind(&Server::AcceptThread, this));

	return LOGIC_SUCCESS;
}

bool Server::Send(Packet_Header* Send_Packet)
{
	/// 모든 클라이언트에게 메세지를 보냄.

	assert(nullptr != Send_Packet);

	// 임계 영역
	{
		EnterCriticalSection(&g_SCS);

		// 전체 클라이언트 소켓에 패킷 송신
		for (auto it : g_Client_Socket_List)
		{
			// 패킷 송신
			SendTargetSocket(it->m_Socket, Send_Packet);
		}

		LeaveCriticalSection(&g_SCS);
	}

	return true;
}


bool Server::Recv(Packet_Header** Recv_Packet, char* MsgBuff)
{
	/// 큐가 비었으면 FALSE를 반환한다.
	if (Recv_Data_Queue.empty())
		return FALSE;

	/// 큐에서 데이터를 빼온다.
	Packet_Header* cpcHeader = nullptr;
	Recv_Data_Queue.try_pop(cpcHeader);

	/// 메세지 타입으로 Header 캐스팅.
	C2S_Message* C2S_Msg = static_cast<C2S_Message*>(cpcHeader);

	switch (cpcHeader->Packet_Type)
	{
	case C2S_Packet_Type_Message:         // 채팅 메세지
	{
		/// 채팅 메세지가 있으면 MsgBuff에 저장해준다.
		//MsgBuff.resize(PACKET_BUFSIZE);
		//strcpy_s(sPacket.Message_Buffer, Msg_Buff.c_str());
		memcpy_s(MsgBuff, PACKET_BUFSIZE, C2S_Msg->Message_Buffer, PACKET_BUFSIZE);
	}
	case C2S_Packet_Type_Data:
	{
		/// 추후 필요시 구현..
	}
	break;
	}

	// 해제.
	delete cpcHeader;

	/// 큐가 비어있지 않으면 TRUE 반환.
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

	for (auto k : g_Client_Socket_List)
	{
		/// 혹시라도 다른데에서 쓰이고 있을 수도 있으니까요 ㅎㅎ
		EnterCriticalSection(&g_SCS);
		Safe_CloseSocket(k);
		LeaveCriticalSection(&g_SCS);
	}

	// 윈속 종료
	WSACleanup();

	printf_s("[TCP 서버] 종료\n");

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

	while (TRUE)
	{
		DWORD        dwNumberOfBytesTransferred = 0;
		Socket_Struct* psSocket = nullptr;
		Overlapped_Struct* psOverlapped = nullptr;

		// GetQueuedCompletionStatus() - GQCS 라고 부름
		// WSARead(), WSAWrite() 등의 Overlapped I/O 관련 처리 결과를 받아오는 함수
		// PostQueuedCompletionStatus() 를 통해서도 GQCS 를 리턴시킬 수 있다.( 일반적으로 쓰레드 종료 처리 )
		BOOL bSuccessed = GetQueuedCompletionStatus(g_IOCP,												// IOCP 핸들
			&dwNumberOfBytesTransferred,						    // I/O 에 사용된 데이터의 크기
			reinterpret_cast<PULONG_PTR>(&psSocket),           // 소켓의 IOCP 등록시 넘겨준 키 값
																   // ( connect() 이 후, CreateIoCompletionPort() 시 )
			reinterpret_cast<LPOVERLAPPED*>(&psOverlapped),   // WSARead(), WSAWrite() 등에 사용된 WSAOVERLAPPED
			INFINITE);										    // 신호가 발생될 때까지 무제한 대기

		// 키가 nullptr 일 경우 쓰레드 종료를 의미
		if (nullptr == psSocket)
		{
			// 다른 WorkerThread() 들의 종료를 위해서
			PostQueuedCompletionStatus(g_IOCP, 0, 0, nullptr);
			break;
		}

		assert(nullptr != psOverlapped);

		// 오버랩드 결과 체크
		if (!bSuccessed)
		{
			printf_s("[TCP 서버] 오버랩드 결과를 체크하는 도중 서버와 연결이 끊겼습니다.\n");
			EnterCriticalSection(&g_SCS);
			Safe_CloseSocket(psSocket);
			LeaveCriticalSection(&g_SCS);

			delete psOverlapped;
			continue;
		}

		// 연결 종료
		if (0 == dwNumberOfBytesTransferred)
		{
			EnterCriticalSection(&g_SCS);
			Safe_CloseSocket(psSocket);
			LeaveCriticalSection(&g_SCS);
			delete psOverlapped;
			continue;
		}

		// Overlapped I/O 처리
		switch (psOverlapped->m_IOType)
		{
		case Overlapped_Struct::IOType::IOType_Recv: IOFunction_Recv(dwNumberOfBytesTransferred, psOverlapped, psSocket); break; // WSARecv() 의 Overlapped I/O 완료에 대한 처리
		case Overlapped_Struct::IOType::IOType_Send: IOFunction_Send(dwNumberOfBytesTransferred, psOverlapped, psSocket); break; // WSASend() 의 Overlapped I/O 완료에 대한 처리
		}
	}

}

int CALLBACK Server::AcceptFunction(LPWSABUF lpCallerId, LPWSABUF lpCallerData, LPQOS lpSQOS, LPQOS lpGQOS, LPWSABUF lpCalleeId, LPWSABUF lpCalleeData, GROUP FAR* g, DWORD_PTR dwCallbackData)
{
	/// 만약 MAX_USER_COUNT 가 0이면 최대 접속유저 제한이 없다.
	if (MAX_USER_COUNT == 0)
	{
		// 연결 수락.
		return CF_ACCEPT;
	}

	/// WSAAccept에서 받아오는 인자값. (우리가 list * 를 넘겼으니 똑같이 캐스팅 해줘서 원래 데이터로 복원한다. )
	const std::list< std::shared_ptr<Socket_Struct> >* const cpclistClients = reinterpret_cast<std::list< std::shared_ptr<Socket_Struct> >*>(dwCallbackData);

	/// list의 사이즈를 체크하면서 현재 Accept를 받아도 되는지 안되는지 체크한다.
	/// 최대 유저의 수를 확인하기 위함.
	if (cpclistClients->size() >= MAX_USER_COUNT)
	{
		// 연결 거절.
		return CF_REJECT;
	}

	// 연결 수락.
	return CF_ACCEPT;
}

bool Server::AddClientSocket(std::shared_ptr<Socket_Struct> psSocket)
{
	assert(nullptr != psSocket);

	// 임계 영역
	{
		// 클라이언트 정보를 list 에 추가

		EnterCriticalSection(&g_SCS);

		// 클라이언트 중복 검사
		for (auto it : g_Client_Socket_List)
		{
			if (it == psSocket)
			{
				LeaveCriticalSection(&g_SCS);
				return FALSE;
			}
		}

		// 클라이언트 등록
		g_Client_Socket_List.push_back(psSocket);

		LeaveCriticalSection(&g_SCS);
	}

	return TRUE;
}

bool Server::RemoveClientSocket(std::shared_ptr<Socket_Struct> psSocket)
{
	assert(INVALID_SOCKET != psSocket->m_Socket);

	// 임계 영역
	{
		EnterCriticalSection(&g_SCS);

		// 클라이언트 종료 및 삭제
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
			// 서버에서 연결 거부( AcceptCondition() )를 한 경우는 오류 출력을 안하게 함
			if (WSAGetLastError() != WSAECONNREFUSED)
			{
				sprintf_s(szBuffer, "[TCP 서버] 에러 발생 -- WSAAccept() :");
				err_display(szBuffer);
			}

			continue;
		}

		// 클라이언트 구조체 생성 및 등록
		std::shared_ptr<Socket_Struct> psSocket = std::make_shared<Socket_Struct>();
		psSocket->m_Socket = socket;
		psSocket->IP = inet_ntoa(clientaddr.sin_addr);
		psSocket->PORT = ntohs(clientaddr.sin_port);

		/// IOCP에 소켓의 핸들과 같이 등록한다.
		if (CreateIoCompletionPort(reinterpret_cast<HANDLE>(psSocket->m_Socket), g_IOCP, reinterpret_cast<ULONG_PTR>(psSocket.get()), 0) != g_IOCP)
		{
			// 실패 시에 현재 생성한 소켓은 삭제되어야 한다.
			psSocket.reset();
			continue;
		}

		// 연결된 클라이언트 정보를 등록
		if (!AddClientSocket(psSocket))
		{
			psSocket.reset();
			continue;
		}

		printf_s("[TCP 서버] [%15s:%5d] 클라이언트가 접속\n", psSocket->IP.c_str(), psSocket->PORT);

		/// WSARecv를 걸어둬야 정보를 받을 수 있겠죠?!
		if (!Reserve_WSAReceive(psSocket->m_Socket))
		{
			// 클라이언트 종료
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

void Server::Safe_CloseSocket(std::shared_ptr<Socket_Struct> psSocket)
{
	/// Critical Section은 클라이언트 소켓을 지울땐 함수를 호출하기전에 이미 걸려있는 상태가 아닐까?
	auto it = g_Client_Socket_List.begin();

	for (it; it != g_Client_Socket_List.end(); it++)
	{
		/// 해당하는 포인터를 들고있는 shared_ptr을 찾는다.
		if (*it == psSocket)
		{
			break;
		}
	}

	while (!psSocket.unique())
	{
		/// 다른 곳에 참조되고 있는 부분이 있다면, 종료가 될 때까지 기다린다.
		Sleep(0);
	}

	if (psSocket.unique())
	{
		shutdown(psSocket->m_Socket, SD_BOTH);
		closesocket(psSocket->m_Socket);
		psSocket->m_Socket = INVALID_SOCKET;
		psSocket->Is_Connected = FALSE;

		printf_s("[TCP 서버] [%15s:%5d] 클라이언트와 종료\n", psSocket->IP.c_str(), psSocket->PORT);
	}

	/// 해당하는 클라이언트 소켓을 리스트에서 삭제.
	g_Client_Socket_List.erase(it);
}

void Server::Safe_CloseSocket(Socket_Struct* psSocket)
{
	auto it = g_Client_Socket_List.begin();

	for (it; it != g_Client_Socket_List.end(); it++)
	{
		/// 해당하는 포인터를 들고있는 shared_ptr을 찾는다.
		if (it->get() == psSocket)
		{
			break;
		}
	}

	while (!it->unique())
	{
		/// 다른 곳에 참조되고 있는 부분이 있다면, 종료가 될 때까지 기다린다.
		Sleep(0);
	}

	if (it->unique())
	{
		shutdown(psSocket->m_Socket, SD_BOTH);
		closesocket(psSocket->m_Socket);
		psSocket->m_Socket = INVALID_SOCKET;
		psSocket->Is_Connected = FALSE;

		printf_s("[TCP 서버] [%15s:%5d] 클라이언트와 종료\n", psSocket->IP.c_str(), psSocket->PORT);
	}

	/// 해당하는 클라이언트 소켓을 리스트에서 삭제.
	g_Client_Socket_List.erase(it);
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
		&psOverlapped->wsaOverlapped,
		nullptr);

	if ((SOCKET_ERROR == iResult) && (WSAGetLastError() != WSA_IO_PENDING))
	{
		char szBuffer[PACKET_BUFSIZE] = { 0, };
		sprintf_s(szBuffer, "[TCP 서버] 에러 발생 -- WSARecv() :");
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
		&psOverlapped->wsaOverlapped,
		nullptr);

	if ((SOCKET_ERROR == iResult) && (WSAGetLastError() != WSA_IO_PENDING))
	{
		char szBuffer[STRUCT_BUFSIZE] = { 0, };
		sprintf_s(szBuffer, "[TCP 서버] 에러 발생 -- WSASend() :");
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
			EnterCriticalSection(&g_SCS);
			Safe_CloseSocket(psSocket);
			LeaveCriticalSection(&g_SCS);
			delete psOverlapped;
			return;
		}

		Recv_Data_Queue.push(cpcHeader);

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
		EnterCriticalSection(&g_SCS);
		Safe_CloseSocket(psSocket);
		LeaveCriticalSection(&g_SCS);
		delete psOverlapped;
		return;
	}
}

void Server::IOFunction_Send(DWORD dwNumberOfBytesTransferred, Overlapped_Struct* psOverlapped, Socket_Struct* psSocket)
{
	printf_s("[TCP 서버] [%15s:%5d] 패킷 송신 완료 -> %d 바이트\n", psSocket->IP.c_str()
		, psSocket->PORT, dwNumberOfBytesTransferred);

	delete psOverlapped;
}
