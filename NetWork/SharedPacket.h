#pragma once

#include "NetWorkDefine.h"
#include <windows.h>

/// ����� ��Ŷ ����ü�� 1����Ʈ�� ������ �Ѵ�.
#pragma pack(push , 1)

struct Packet_Header
{
	unsigned short Packet_Size;
	unsigned short Packet_Type;	// �������� Ÿ���� ��������?
};

//////////////////////////////////////////////////////////////////////////
///			Ŭ���̾�Ʈ to ���� ��Ŷ���� Ÿ���� �����Ѵ�.				   ///
//////////////////////////////////////////////////////////////////////////

enum C2S_Packet_Type
{
	//									TYPE			INDEX
	C2S_Packet_Type_Message = 0,	/// �޼��� Ÿ��		0
	C2S_Packet_Type_Data,			/// ������ Ÿ��		1
	C2S_Packet_Type_MAX,
};

struct C2S_Message : public Packet_Header
{
	C2S_Message()
	{
		Packet_Size = sizeof(*this) - sizeof(Packet_Size);
		Packet_Type = C2S_Packet_Type_Message;
	}

	char Message_Buffer[PACKET_BUFSIZE];	/// �޼��� ����
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
	char Client_IP[16];
	unsigned short Client_Port;
	char Message_Buffer[PACKET_BUFSIZE];
};

/// 1����Ʈ ���� ��.
#pragma pack(pop)