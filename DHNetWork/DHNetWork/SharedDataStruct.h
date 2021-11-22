#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "NetWorkDefine.h"

/*
	2021/10/22 07:21 - CDH

	< ������� >
		1. NetWorkStruct�� Packet ����� �ϳ��� ��ħ.

	2021/11/22 11:24 - CDH

	< ������� >
		1. FlatBuffer �� �����ϴ� �޼��� Ÿ�� ������.
		2. ������ ������ ����.
*/

///////////////////////////////////////////////////////////////////////////
///					Socket & Overlapped Struct ����						///
///////////////////////////////////////////////////////////////////////////

//////////////////////////////////
//	Socket & Overlapped Struct	//
//////////////////////////////////

struct Socket_Struct
{
	Socket_Struct()
	{
		// Placement new �� ����ϱ� ���� �����ڿ��� m_Socket �ʱ�ȭ.
		m_Socket = INVALID_SOCKET;
	}

	~Socket_Struct()
	{
		// �Ҹ��ڸ� ȣ���ϴ� ��������, ������ ����Ǿ������� �ش� ������ ������ ����.
		if (INVALID_SOCKET != m_Socket)
		{
			shutdown(m_Socket, SD_BOTH);
			closesocket(m_Socket);
		}
	}

	SOCKET m_Socket;			// ����� ����
	std::string IP;				// ���� �� ���� IP
	unsigned short PORT = 0;	// ���� �� ���� PORT
};

// �⺻ ���������� WSAOVERLAPPED �� ��ӹ޴� Overlapped Struct
struct Overlapped_Struct : public WSAOVERLAPPED
{
	Overlapped_Struct()
	{
		hEvent = 0;
		Internal = 0;
		InternalHigh = 0;
		Offset = 0;
		OffsetHigh = 0;
		m_Socket = INVALID_SOCKET;
		ZeroMemory(m_Buffer, sizeof(m_Buffer));
		m_Data_Size = 0;
	}

	/// Overlapped I/O�� �۾� ����.
	/// �񵿱� ó���� ���ؼ� Type�� �����صΰ�, �ش� IOCP�� ���� ó���� �����Ѵ�.
	enum class IOType
	{
		IOType_Accept,
		// ������ ������ ���� DisconnectEx �� ���.
		IOType_Disconnect,
		IOType_Recv,
		IOType_Send
	};

	IOType			m_IOType;					// ó������� �뺸���� �� �۾��� �����ϱ� ����.
	SOCKET			m_Socket;					// ���������� ����̵Ǵ� ����
	char			m_Buffer[PACKET_BUFIZE];	// Send/Recv ����
	int				m_Data_Size;				// ó���ؾ��ϴ� �������� ��
};

//////////////////////////////////////////////////////////////////////////
///			Ŭ���̾�Ʈ to ���� ��Ŷ���� Ÿ���� �����Ѵ�.				   ///
//////////////////////////////////////////////////////////////////////////

/// ����� ��Ŷ ����ü�� 1����Ʈ�� ������ �Ѵ�.
#pragma pack(push , 1)

enum C2S_Packet_Type
{
	//									TYPE				INDEX
	C2S_Packet_Type_None = 0,		/// Default Ÿ��			0
	C2S_Packet_Type_Message,		/// �޼��� Ÿ��			1
	C2S_Packet_Type_Unit,			/// ���� ������ Ÿ��		2
	C2S_Packet_Type_World,			/// ���� ���� ������		3
};

struct C2S_Packet : public Packet_Header
{
	C2S_Packet()
	{
		// ��Ŷ�� �� �������, ���� C2S_Packet ����ü�� ũ���̴�.
		Packet_Size = sizeof(*this) - sizeof(Packet_Size);
		Packet_Type = C2S_Packet_Type_None;
	}

	// ������ ���� �����͸� ���� Packet_Buffer �� �����Ǿ�����.
	char Packet_Buffer[PACKET_BUFIZE] = { 0, };
};

//////////////////////////////////////////////////////////////////////////
///			���� to Ŭ���̾�Ʈ ��Ŷ���� Ÿ���� �����Ѵ�.				   ///
//////////////////////////////////////////////////////////////////////////

enum S2C_Packet_Type
{
	//									TYPE				INDEX
	S2C_Packet_Type_None = 0,		/// Default Ÿ��			0
	S2C_Packet_Type_Message,		/// �޼��� Ÿ��			1
	S2C_Packet_Type_Unit,			/// ���� ������ Ÿ��		2
	S2C_Packet_Type_World,			/// ���� ���� ������		3
};

struct S2C_Packet : public Packet_Header
{
	S2C_Packet()
	{
		// ��Ŷ�� �� �������, ���� C2S_Packet ����ü�� ũ���̴�.
		Packet_Size = sizeof(*this) - sizeof(Packet_Size);
		Packet_Type = S2C_Packet_Type_None;
	}

	// Ŭ���̾�Ʈ IP/Port ���� �� �����͸� ���� Packet_Buffer �� �����Ǿ�����.
	char			Client_IP[IP_SIZE]					= { 0, };
	unsigned short	Client_Port							= 0;
	char			Packet_Buffer[PACKET_BUFIZE]		= { 0, };
};

/// 1����Ʈ ���� ��.
#pragma pack(pop)