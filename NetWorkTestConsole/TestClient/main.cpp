#include "DHNetWork.h"

int main()
{
	DHNetWork* my_NetWork = new DHNetWork(NetWork_Type::Client, 9000);

	my_NetWork->Start();

	bool One_Flag = true;
	C2S_Message* _msg = new C2S_Message;

	while (true)
	{
		if (GetAsyncKeyState(VK_F2) & 0x8000)
		{
			if (One_Flag)
			{
				my_NetWork->Send(_msg);
				One_Flag = false;
			}
		}

		Sleep(0);
	}
}