
#include <time.h>
#include "DummyClient.h"

int main()
{
	DummyClient Play_Dummy;

	/// Recv �������ִٸ� ������ ���� ���.
	//std::vector<Network_Message> Msg_Vec;

	//if (my_NetWork->Recv(Msg_Vec))
	//{
	//	for (auto Msg_Packet : Msg_Vec)
	//	{
	//		SOCKET _Recv_Socket_Num = Msg_Packet.Socket;
	//		S2C_Packet* S2C_Msg = static_cast<S2C_Packet*>(Msg_Packet.Packet);

	//		switch (S2C_Msg->Packet_Type)
	//		{
	//		case S2C_Packet_Type_Message:         // ä�� �޼���
	//		{
	//			/// ä�� �޼����� ������ MsgBuff�� �������ش�.
	//			char* Msg_Buff = new char[MAX_PACKET_SIZE];
	//			memcpy_s(Msg_Buff, MAX_PACKET_SIZE, S2C_Msg->Packet_Buffer, S2C_Msg->Packet_Size);

	//			//printf_s(Msg_Buff);
	//			//printf_s("\n");
	//		}
	//		break;
	//		}
	//	}
	//}


	return 0;
}