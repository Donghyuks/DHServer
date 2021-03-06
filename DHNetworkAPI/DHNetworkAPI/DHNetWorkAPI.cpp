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

BOOL DHNetWorkAPI::Send(Packet_Header* _Packet, int _SendType /*= 0*/, SOCKET _Socket /*= INVALID_SOCKET*/)
{
	return Set_NetWork->Send(_Packet, _SendType, _Socket);
}

BOOL DHNetWorkAPI::Connect(unsigned short _Port, std::string _IP)
{
	return Set_NetWork->Connect(_Port, _IP);
}

BOOL DHNetWorkAPI::Initialize(DHNetWork_Name _Using_NetWork_Name, unsigned short _Debug_Option)
{
	switch (_Using_NetWork_Name)
	{
	case DHNetWork_Name::DHNet:
	{
		Set_NetWork = new DHNetWork();
		return Set_NetWork->Initialize(_Debug_Option);
	}
	break;
	}

	return false;
}

BOOL DHNetWorkAPI::Accept(unsigned short _Port, unsigned short _Max_User_Count, unsigned short _Work_Thread_Count)
{
	return Set_NetWork->Accept(_Port, _Max_User_Count, _Work_Thread_Count);
}

BOOL DHNetWorkAPI::Disconnect(SOCKET _Socket)
{
	return Set_NetWork->Disconnect(_Socket);
}

BOOL DHNetWorkAPI::End()
{
	if (Set_NetWork != nullptr)
	{
		Set_NetWork->End();
		delete Set_NetWork;
	}

	return true;
}
