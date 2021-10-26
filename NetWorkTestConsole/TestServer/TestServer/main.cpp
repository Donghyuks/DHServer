#include "C2NetworkAPI.h"

int main()
{
	std::vector<Network_Message*> Msg_Vec;

	C2NetWorkAPI* my_NetWork = new C2NetWorkAPI(NetWork_Name::DHNet, NetWork_Type::Server, 9000);
	my_NetWork->Start();

	while (true)
	{	
		if (my_NetWork->Recv(Msg_Vec))
		{
			for (auto Msg_Packet : Msg_Vec)
			{
				SOCKET _Recv_Socket_Num = (*Msg_Packet).Socket;
				C2S_Message* C2S_Msg = static_cast<C2S_Message*>((*Msg_Packet).Packet);

				switch (C2S_Msg->Packet_Type)
				{
				case C2S_Packet_Type_Message:         // 채팅 메세지
				{
					/// 채팅 메세지가 있으면 MsgBuff에 저장해준다.
					char* Msg_Buff = new char[MSG_BUFSIZE];
					memcpy_s(Msg_Buff, MSG_BUFSIZE, C2S_Msg->Message_Buffer, MSG_BUFSIZE);
				}
				case C2S_Packet_Type_Data:
				{
					/// 추후 필요시 구현..
				}
				break;
				}
			}

			Msg_Vec.clear();
		}
		

		Sleep(0);
	}
	
}