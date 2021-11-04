#include "C2NetworkAPI.h"
#include "DHNetWork.h"

C2NetWorkAPI::C2NetWorkAPI(C2NetWork_Name _Using_NetWork_Name, C2NetWork_Type _Create_NetWork_Type, unsigned short _PORT,
	std::string _IP /*= "127.0.0.1"*/, unsigned short MAX_USER_COUNT /*= 100*/)
{
	switch (_Using_NetWork_Name)
	{
	case C2NetWork_Name::DHNet:
	{
		switch (_Create_NetWork_Type)
		{
		case C2NetWork_Type::Client:
			Set_NetWork = new DHNetWork(NetWork_Type::Client, _PORT, _IP, MAX_USER_COUNT);
			break;
		case C2NetWork_Type::Server:
			Set_NetWork = new DHNetWork(NetWork_Type::Server, _PORT, _IP, MAX_USER_COUNT);
			break;
		}
	}
	break;
	case C2NetWork_Name::MGNet:
	{
		// πŒ∞Ê≥› ∫Ÿ¿Ã∏Èµ .
	}
	break;
	default:
		break;
	}
}

C2NetWorkAPI::~C2NetWorkAPI()
{
	delete Set_NetWork;
}

BOOL C2NetWorkAPI::Recv(std::vector<Network_Message*>& _Message_Vec)
{
	return Set_NetWork->Recv(_Message_Vec);
}

BOOL C2NetWorkAPI::Send(Packet_Header* _Packet, SOCKET _Socket /*= INVALID_SOCKET*/)
{
	return Set_NetWork->Send(_Packet, _Socket);
}

BOOL C2NetWorkAPI::Connect(unsigned short _Port, std::string _IP)
{
	return Set_NetWork->Connect(_Port, _IP);
}

BOOL C2NetWorkAPI::Accept()
{
	return Set_NetWork->Accept();
}

BOOL C2NetWorkAPI::Disconnect(SOCKET _Socket)
{
	return Set_NetWork->Disconnect(_Socket);
}

BOOL C2NetWorkAPI::Start()
{
	return Set_NetWork->Start();
}

BOOL C2NetWorkAPI::End()
{
	Set_NetWork->End();
	this->~C2NetWorkAPI();

	return true;
}
