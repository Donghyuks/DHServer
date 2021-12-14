#include "DHNetWorkAPI.h"
#include "SharedDataStruct.h"

int main()
{
	std::vector<Network_Message> Msg_Vec;

	/// 채팅 메세지가 있으면 MsgBuff에 저장해준다.
	char* Msg_Buff = new char[MAX_PACKET_SIZE];

	std::string Test_Server_Msg("[Server Effective Data]");
	S2C_Packet* _Server_Msg = new S2C_Packet;
	strcpy_s(_Server_Msg->Packet_Buffer, Test_Server_Msg.c_str());
	_Server_Msg->Packet_Type = S2C_Packet_Type::S2C_Packet_Type_Message;
	_Server_Msg->Packet_Size = Test_Server_Msg.size() + 1;


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
					memcpy_s(Msg_Buff, MAX_PACKET_SIZE, C2S_Msg->Packet_Buffer, C2S_Msg->Packet_Size);

					// 특정 쓰레드에 메세지 보내기
					//my_NetWork->Send(_Server_Msg, Msg_Packet.Socket);

					//printf_s(Msg_Buff);
					//printf_s("\n");
				}
				break;
				}
			}

			// Broad Cast
			my_NetWork->Send(_Server_Msg);

			Msg_Vec.clear();
		}
		

		Sleep(0);
	}
	
}