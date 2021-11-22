#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "NetWorkDefine.h"

/*
	2021/10/22 07:21 - CDH

	< 변경사항 >
		1. NetWorkStruct와 Packet 헤더를 하나로 합침.

	2021/11/22 11:24 - CDH

	< 변경사항 >
		1. FlatBuffer 에 대응하는 메세지 타입 재정의.
		2. 데이터 구조의 변경.
*/

///////////////////////////////////////////////////////////////////////////
///					Socket & Overlapped Struct 정의						///
///////////////////////////////////////////////////////////////////////////

//////////////////////////////////
//	Socket & Overlapped Struct	//
//////////////////////////////////

struct Socket_Struct
{
	Socket_Struct()
	{
		// Placement new 를 사용하기 위해 생성자에서 m_Socket 초기화.
		m_Socket = INVALID_SOCKET;
	}

	~Socket_Struct()
	{
		// 소멸자를 호출하는 시점에서, 소켓이 연결되어있으면 해당 소켓의 해제를 진행.
		if (INVALID_SOCKET != m_Socket)
		{
			shutdown(m_Socket, SD_BOTH);
			closesocket(m_Socket);
		}
	}

	SOCKET m_Socket;			// 연결된 소켓
	std::string IP;				// 연결 된 곳의 IP
	unsigned short PORT = 0;	// 연결 된 곳의 PORT
};

// 기본 오버랩드인 WSAOVERLAPPED 를 상속받는 Overlapped Struct
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

	/// Overlapped I/O의 작업 종류.
	/// 비동기 처리를 위해서 Type을 지정해두고, 해당 IOCP에 대한 처리를 진행한다.
	enum class IOType
	{
		IOType_Accept,
		// 소켓의 재사용을 위해 DisconnectEx 를 사용.
		IOType_Disconnect,
		IOType_Recv,
		IOType_Send
	};

	IOType			m_IOType;					// 처리결과를 통보받은 후 작업을 구분하기 위해.
	SOCKET			m_Socket;					// 오버랩드의 대상이되는 소켓
	char			m_Buffer[PACKET_BUFIZE];	// Send/Recv 버퍼
	int				m_Data_Size;				// 처리해야하는 데이터의 양
};

//////////////////////////////////////////////////////////////////////////
///			클라이언트 to 서버 패킷들의 타입을 정의한다.				   ///
//////////////////////////////////////////////////////////////////////////

/// 사용할 패킷 구조체들 1바이트로 정렬을 한다.
#pragma pack(push , 1)

enum C2S_Packet_Type
{
	//									TYPE				INDEX
	C2S_Packet_Type_None = 0,		/// Default 타입			0
	C2S_Packet_Type_Message,		/// 메세지 타입			1
	C2S_Packet_Type_Unit,			/// 유닛 데이터 타입		2
	C2S_Packet_Type_World,			/// 게임 월드 데이터		3
};

struct C2S_Packet : public Packet_Header
{
	C2S_Packet()
	{
		// 패킷의 총 사이즈는, 현재 C2S_Packet 구조체의 크기이다.
		Packet_Size = sizeof(*this) - sizeof(Packet_Size);
		Packet_Type = C2S_Packet_Type_None;
	}

	// 서버로 보낼 데이터를 담을 Packet_Buffer 로 구성되어있음.
	char Packet_Buffer[PACKET_BUFIZE] = { 0, };
};

//////////////////////////////////////////////////////////////////////////
///			서버 to 클라이언트 패킷들의 타입을 정의한다.				   ///
//////////////////////////////////////////////////////////////////////////

enum S2C_Packet_Type
{
	//									TYPE				INDEX
	S2C_Packet_Type_None = 0,		/// Default 타입			0
	S2C_Packet_Type_Message,		/// 메세지 타입			1
	S2C_Packet_Type_Unit,			/// 유닛 데이터 타입		2
	S2C_Packet_Type_World,			/// 게임 월드 데이터		3
};

struct S2C_Packet : public Packet_Header
{
	S2C_Packet()
	{
		// 패킷의 총 사이즈는, 현재 C2S_Packet 구조체의 크기이다.
		Packet_Size = sizeof(*this) - sizeof(Packet_Size);
		Packet_Type = S2C_Packet_Type_None;
	}

	// 클라이언트 IP/Port 정보 및 데이터를 담을 Packet_Buffer 로 구성되어있음.
	char			Client_IP[IP_SIZE]					= { 0, };
	unsigned short	Client_Port							= 0;
	char			Packet_Buffer[PACKET_BUFIZE]		= { 0, };
};

/// 1바이트 정렬 끝.
#pragma pack(pop)