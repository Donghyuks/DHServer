#include "DHNetworkAPI.h"
#include "DHNetWork.h"

DHNetWorkAPI::DHNetWorkAPI()
{

}

DHNetWorkAPI::~DHNetWorkAPI()
{
	End();
}

BOOL DHNetWorkAPI::Recv(std::vector<Network_Message>& _Message_Vec)
{
	return Set_NetWork->Recv(_Message_Vec);
}

BOOL DHNetWorkAPI::Send(Packet_Header* _Packet, SOCKET _Socket /*= INVALID_SOCKET*/)
{
	return Set_NetWork->Send(_Packet, _Socket);
}

BOOL DHNetWorkAPI::Connect(unsigned short _Port, std::string _IP)
{
	return Set_NetWork->Connect(_Port, _IP);
}

BOOL DHNetWorkAPI::Initialize(DHNetWork_Name _Using_NetWork_Name)
{
	switch (_Using_NetWork_Name)
	{
	case DHNetWork_Name::DHNet:
	{
		Set_NetWork = new DHNetWork();
		return Set_NetWork->Initialize();
	}
	break;
	}

	return false;
}

BOOL DHNetWorkAPI::Accept(unsigned short _Port, unsigned short _Max_User_Count)
{
	return Set_NetWork->Accept(_Port, _Max_User_Count);
}

BOOL DHNetWorkAPI::Disconnect(SOCKET _Socket)
{
	return Set_NetWork->Disconnect(_Socket);
}

BOOL DHNetWorkAPI::End()
{
	return Set_NetWork->End();
}
