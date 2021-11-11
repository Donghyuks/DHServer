#include "C2NetworkAPI.h"
#include "DHNetWork.h"

C2NetWorkAPI::C2NetWorkAPI()
{

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

BOOL C2NetWorkAPI::Initialize(C2NetWork_Name _Using_NetWork_Name)
{
	switch (_Using_NetWork_Name)
	{
	case C2NetWork_Name::DHNet:
	{
		Set_NetWork = new DHNetWork();
		return Set_NetWork->Initialize();
	}
	break;
	case C2NetWork_Name::MGNet:
	{

	}
	break;
	}

	return false;
}

BOOL C2NetWorkAPI::Accept(unsigned short _Port, unsigned short _Max_User_Count)
{
	return Set_NetWork->Accept(_Port, _Max_User_Count);
}

BOOL C2NetWorkAPI::Disconnect(SOCKET _Socket)
{
	return Set_NetWork->Disconnect(_Socket);
}

BOOL C2NetWorkAPI::End()
{
	Set_NetWork->End();
	this->~C2NetWorkAPI();

	return true;
}
