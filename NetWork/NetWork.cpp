
#include "NetWork.h"
#include <iostream>
#include <assert.h>

NetWork::NetWork(NetWork_Type Create_NetWork_Type, unsigned short _PORT, std::string _IP /*= "127.0.0.1"*/, unsigned short MAX_USER_COUNT /*= 0*/)
{
	m_NetWork_Type = Create_NetWork_Type;

	switch (Create_NetWork_Type)
	{
	case NetWork_Type::Server:
	{
		if (m_Client != nullptr)
		{
			std::cout << "이미 Client로 선언된 Class 입니다. - 생성자 에러" << std::endl;
			return;
		}
		m_Server = new Server(_PORT, MAX_USER_COUNT);
	}
	break;
	case NetWork_Type::Client:
	{
		if (m_Server != nullptr)
		{
			std::cout << "이미 Server로 선언된 Class 입니다. - 생성자 에러" << std::endl;
			return;
		}
		m_Client = new Client(_PORT, _IP);
	}
	break;
	default:
		break;
	}
}

bool NetWork::Start()
{
	switch (m_NetWork_Type)
	{
	case NetWork_Type::Server:
		return m_Server->Start();
	case NetWork_Type::Client:
		return m_Client->Start();
	default:
		break;
	}

	return LOGIC_FAIL;
}

bool NetWork::Send(Packet_Header* Send_Packet)
{
	switch (m_NetWork_Type)
	{
	case NetWork_Type::Server:
		return m_Server->Send(Send_Packet);
	break;
	case NetWork_Type::Client:
		return m_Client->Send(Send_Packet);
	default:
		break;
	}

	return LOGIC_FAIL;
}

bool NetWork::Recv(Packet_Header** Recv_Packet, char* MsgBuff)
{
	switch (m_NetWork_Type)
	{
	case NetWork_Type::Server:
		return m_Server->Recv(Recv_Packet, MsgBuff);
	break;
	case NetWork_Type::Client:
		return m_Client->Recv(Recv_Packet, MsgBuff);
	default:
		break;
	}

	return LOGIC_FAIL;
}

bool NetWork::End()
{
	switch (m_NetWork_Type)
	{
	case NetWork_Type::Server:
		return m_Server->End();
	case NetWork_Type::Client:
		return m_Client->End();
	default:
		break;
	}

	return LOGIC_FAIL;
}
