#pragma once

#include "NetWorkDefine.h"
#include <windows.h>

/// 사용할 패킷 구조체들 1바이트로 정렬을 한다.
#pragma pack(push , 1)

struct Packet_Header
{
	unsigned short Packet_Size;
	unsigned short Packet_Type;	// 데이터의 타입이 무엇인지?
};

//////////////////////////////////////////////////////////////////////////
///			클라이언트 to 서버 패킷들의 타입을 정의한다.				   ///
//////////////////////////////////////////////////////////////////////////

enum C2S_Packet_Type
{
	//									TYPE			INDEX
	C2S_Packet_Type_Message = 0,	/// 메세지 타입		0
	C2S_Packet_Type_Data,			/// 데이터 타입		1
	C2S_Packet_Type_MAX,
};

struct C2S_Message : public Packet_Header
{
	C2S_Message()
	{
		Packet_Size = sizeof(*this) - sizeof(Packet_Size);
		Packet_Type = C2S_Packet_Type_Message;
	}

	char Message_Buffer[PACKET_BUFSIZE];	/// 메세지 내용
};

//////////////////////////////////////////////////////////////////////////
///			서버 to 클라이언트 패킷들의 타입을 정의한다.				   ///
//////////////////////////////////////////////////////////////////////////

enum S2C_Packet_Type
{
	//									TYPE			INDEX
	S2C_Packet_Type_Message = 0,	/// 메세지 타입		0
	S2C_Packet_Type_Data,			/// 데이터 타입		1
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
	char Client_IP[16];
	unsigned short Client_Port;
	char Message_Buffer[PACKET_BUFSIZE];
};

/// 1바이트 정렬 끝.
#pragma pack(pop)