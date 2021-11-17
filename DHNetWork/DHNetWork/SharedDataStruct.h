#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "NetWorkDefine.h"

/*
	2021/10/22 07:21 - CDH

	< ������� >
		1. NetWorkStruct�� Packet ����� �ϳ��� ��ħ.
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
	enum class IOType
	{
		/// �񵿱� ó���� ���ؼ� Type�� �����صΰ�, �ش� IOCP�� ���� ó���� �����Ѵ�.
		IOType_Accept,
		/// ������ ������ ���� DisconnectEx �� ���.
		IOType_Disconnect,
		IOType_Recv,
		IOType_Send
	};

	IOType			m_IOType;					// ó������� �뺸���� �� �۾��� �����ϱ� ����.
	SOCKET			m_Socket;					// ���������� ����̵Ǵ� ����
	char			m_Buffer[STRUCT_BUFSIZE];	// Send/Recv ����
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
	C2S_Packet_Type_Message = 0,	/// �޼��� Ÿ��			0
	C2S_Packet_Type_Unit,			/// ���� ������ Ÿ��		1
	C2S_Packet_Type_World,			/// ���� ���� ������		2
};

struct C2S_Message : public Packet_Header
{
	C2S_Message()
	{
		Packet_Size = sizeof(*this) - sizeof(Packet_Size);
		Packet_Type = C2S_Packet_Type_Message;
	}

	char Message_Buffer[PACKET_MSG_BUFIZE];	/// �޼��� ����
};

//////////////////////////////////////////////////////////////////////////
///			���� to Ŭ���̾�Ʈ ��Ŷ���� Ÿ���� �����Ѵ�.				   ///
//////////////////////////////////////////////////////////////////////////

enum S2C_Packet_Type
{
	//									TYPE			INDEX
	S2C_Packet_Type_Message = 0,	/// �޼��� Ÿ��		0
	S2C_Packet_Type_Data,			/// ������ Ÿ��		1
	S2C_Packet_Type_MAX,
};

struct S2C_Message : public Packet_Header
{
	S2C_Message()
	{
		Packet_Size = sizeof(*this) - sizeof(Packet_Size);
		Packet_Type = S2C_Packet_Type_Message;
	}

	SYSTEMTIME m_Time;
	char Client_IP[IP_SIZE];
	unsigned short Client_Port;
	char Message_Buffer[PACKET_MSG_BUFIZE];
};

/// 1����Ʈ ���� ��.
#pragma pack(pop)