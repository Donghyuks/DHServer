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

	SOCKET m_Socket;		// ����� ����

	/// �������� �ʿ��� ������.
	std::string IP;			// ���� �� ���� IP
	unsigned short PORT;	// ���� �� ���� PORT

	/// Ŭ���̾�Ʈ���� �ʿ��� ������.
	bool Is_Connected;		// ���� ����.
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

	/// Overlapped I/O�� �۾� ����.
	enum class IOType
	{
		IOType_Recv,
		IOType_Send
	};

	WSAOVERLAPPED	wsaOverlapped;				// �������� I/O�� ���� ����ü.
	IOType			m_IOType;					// ó������� �뺸���� �� �۾��� �����ϱ� ����.
	SOCKET			m_Socket;					// ���������� ����̵Ǵ� ����
	char			m_Buffer[STRUCT_BUFSIZE];	// Send/Recv ����
	int				m_Data_Size;				// ó���ؾ��ϴ� �������� ��
};