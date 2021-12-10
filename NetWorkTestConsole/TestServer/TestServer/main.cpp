#include "DHNetWorkAPI.h"
#include "SharedDataStruct.h"

int main()
{
	std::vector<Network_Message> Msg_Vec;

	DHNetWorkAPI* my_NetWork = new DHNetWorkAPI();
	my_NetWork->Initialize(DHNetWork_Name::DHNet);
	my_NetWork->Accept(729, 1000);

	while (true)
	{	
		if (my_NetWork->Recv(Msg_Vec))
		{
			for (auto Msg_Packet : Msg_Vec)
			{
				SOCKET _Recv_Socket_Num = Msg_Packet.Socket;
				C2S_Packet* C2S_Msg = static_cast<C2S_Packet*>(Msg_Packet.Packet);

				switch (C2S_Msg->Packet_Type)
				{
				case C2S_Packet_Type_Message:         // 채팅 메세지
				{
					/// 채팅 메세지가 있으면 MsgBuff에 저장해준다.
					char* Msg_Buff = new char[ERROR_MSG_BUFIZE];
					memcpy_s(Msg_Buff, ERROR_MSG_BUFIZE, C2S_Msg->Packet_Buffer, ERROR_MSG_BUFIZE);
				}
				break;
				}
			}

			Msg_Vec.clear();
		}
		

		Sleep(0);
	}
	
}