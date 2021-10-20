#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "NetWorkDefine.h"
#include <WinSock2.h>
#include <string>

#pragma comment( lib, "Ws2_32" )

struct Socket_Struct
{
	Socket_Struct()
	{
		m_Socket = INVALID_SOCKET;
		Is_Connected = FALSE;
	}

	~Socket_Struct()
	{
		if (INVALID_SOCKET != m_Socket)
		{
			shutdown(m_Socket, SD_BOTH);
			closesocket(m_Socket);
		}
	}

	SOCKET m_Socket;		// 연결된 소켓

	/// 서버에서 필요한 데이터.
	std::string IP;			// 연결 된 곳의 IP
	unsigned short PORT;	// 연결 된 곳의 PORT

	/// 클라이언트에서 필요한 데이터.
	bool Is_Connected;		// 연결 여부.
};

struct Overlapped_Struct
{
	Overlapped_Struct()
	{
		ZeroMemory(&wsaOverlapped, sizeof(wsaOverlapped));
		m_Socket = INVALID_SOCKET;
		ZeroMemory(m_Buffer, sizeof(m_Buffer));
		m_Data_Size = 0;
	}

	/// Overlapped I/O의 작업 종류.
	enum class IOType
	{
		IOType_Recv,
		IOType_Send
	};

	WSAOVERLAPPED	wsaOverlapped;				// 오버랩드 I/O에 사용될 구조체.
	IOType			m_IOType;					// 처리결과를 통보받은 후 작업을 구분하기 위해.
	SOCKET			m_Socket;					// 오버랩드의 대상이되는 소켓
	char			m_Buffer[STRUCT_BUFSIZE];	// Send/Recv 버퍼
	int				m_Data_Size;				// 처리해야하는 데이터의 양
};