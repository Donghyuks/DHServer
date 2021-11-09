#include "DHNetWork.h"
#include "DHServer.h"
#include "DHClient.h"
#include <iostream>
#include <assert.h>

DHNetWork::DHNetWork(NetWork_Type Create_NetWork_Type, unsigned short _PORT, std::string _IP /*= "127.0.0.1"*/, unsigned short MAX_USER_COUNT /*= 0*/)
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
		m_Server = new DHServer(_PORT, MAX_USER_COUNT);
	}
	break;
	case NetWork_Type::Client:
	{
		if (m_Server != nullptr)
		{
			std::cout << "이미 Server로 선언된 Class 입니다. - 생성자 에러" << std::endl;
			return;
		}
		m_Client = new DHClient();
	}
	break;
	default:
		break;
	}
}

BOOL DHNetWork::Start()
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

BOOL DHNetWork::Send(Packet_Header* _Packet, SOCKET _Socket /*= INVALID_SOCKET*/)
{
	switch (m_NetWork_Type)
	{
	case NetWork_Type::Server:
		return m_Server->Send(_Packet);
	break;
	case NetWork_Type::Client:
		return m_Client->Send(_Packet);
	default:
		break;
	}

	return LOGIC_FAIL;
}

BOOL DHNetWork::Connect(unsigned short _Port, std::string _IP)
{
	switch (m_NetWork_Type)
	{
	case NetWork_Type::Server:
		std::cout << "Server로 생성되었습니다. Connect 호출은 Client에서만 가능합니다." << std::endl;
		return LOGIC_FAIL;
	case NetWork_Type::Client:
		return m_Client->Connect(_Port, _IP);
	default:
		break;
	}

	return LOGIC_FAIL;
}

BOOL DHNetWork::Accept()
{
	return LOGIC_SUCCESS;
}

BOOL DHNetWork::Disconnect(SOCKET _Socket)
{
	return LOGIC_SUCCESS;
}

BOOL DHNetWork::Recv(std::vector<Network_Message*>& _Message_Vec)
{
	switch (m_NetWork_Type)
	{
	case NetWork_Type::Server:
		return m_Server->Recv(_Message_Vec);
	break;
	case NetWork_Type::Client:
		return m_Client->Recv(_Message_Vec);
	default:
		break;
	}

	return LOGIC_FAIL;
}

BOOL DHNetWork::End()
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
