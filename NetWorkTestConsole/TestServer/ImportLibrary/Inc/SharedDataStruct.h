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

	2021/11/24 10:47 - CDH

	< ������� >
		1. ū �����͸� ������쿡 ���� ����ü ����.

	2021/12/10 11:15 - CDH

	< ������� >
		1. ū �����Ϳ� ���� ����ü�� �����ϰ� ������ ����ü�� ���� ���Ǹ� �ּ�ȭ ��.
		2. 1. ���� ������ �����ϱ� ���� Overlapped ����ü�� Big Data�� ���Ͽ� ���� �������� ũ�⸦ ������ ���� �߰�.

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
		ZeroMemory(m_Processing_Packet_Buffer, sizeof(m_Processing_Packet_Buffer));
		m_Data_Size = 0;
		m_Processing_Packet_Size = 0;
		m_Processed_Packet_Size = 0;
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

	IOType			m_IOType;						// ó������� �뺸���� �� �۾��� �����ϱ� ����.
	SOCKET			m_Socket;						// ���������� ����̵Ǵ� ����
	size_t			m_Data_Size;					// ó���ؾ��ϴ� �������� ��
	char			m_Buffer[OVERLAPPED_BUFIZE];	// Send/Recv ����

	/// ������� ���ۻ���� 100�ε�, ����� 60�� �����Ͱ� ���ÿ� �ΰ� ������ 100 / 20���� ���� ������ ������, 60�� ó���ϰ� 40�� �����ص� ��, ���� 20 ��Ŷ�� ���� ���ļ� ó���ؾ��Ѵ�.
	size_t			m_Processing_Packet_Size;						// ���� �������忡�� �޾Ҵ� ��Ŷ�� ��
	char			m_Processing_Packet_Buffer[MAX_PACKET_SIZE];	// ���� �������忡�� �޾ƿ� ����������.

	/// Big Data�� �� ��� ó���� ��Ŷ�� ����� ����ϱ� ���� �뵵.
	size_t			m_Processed_Packet_Size;
};

//////////////////////////////////////////////////////////////////////////
///			Ŭ���̾�Ʈ to ���� ��Ŷ���� Ÿ���� �����Ѵ�.				   ///
//////////////////////////////////////////////////////////////////////////

/// ����� ��Ŷ ����ü�� 1����Ʈ�� ������ �Ѵ�.
#pragma pack(push , 1)

enum C2S_Packet_Type
{
	//									TYPE				INDEX
	C2S_Packet_Type_Message = 0,	/// �޼��� Ÿ��			1
	C2S_Packet_Type_Unit,			/// ���� ������ Ÿ��		2
	C2S_Packet_Type_World,			/// ���� ���� ������		3
	C2S_Packet_Type_None,			/// Default Ÿ��			0
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
	char Packet_Buffer[MAX_PACKET_SIZE] = { 0, };
};

//////////////////////////////////////////////////////////////////////////
///			���� to Ŭ���̾�Ʈ ��Ŷ���� Ÿ���� �����Ѵ�.				   ///
//////////////////////////////////////////////////////////////////////////

enum S2C_Packet_Type
{
	//									TYPE				INDEX
	S2C_Packet_Type_Message = 0,	/// �޼��� Ÿ��			1
	S2C_Packet_Type_Unit,			/// ���� ������ Ÿ��		2
	S2C_Packet_Type_World,			/// ���� ���� ������		3
	S2C_Packet_Type_None,			/// Default Ÿ��			0
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
	char			Packet_Buffer[MAX_PACKET_SIZE]		= { 0, };
};

/// 1����Ʈ ���� ��.
#pragma pack(pop)