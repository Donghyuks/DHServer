#include "DHDB.h"

int main()
{
	bool result_flag = false;

	DHDB my_DB;

	result_flag = my_DB.ConnectDB("221.163.91.100", "CDH", "ehxk2Rnfwoa!", "LOGIN", 3306);

	std::vector<std::string> Friend_List;

	//unsigned int _Identifier = my_DB.GetIdentifier("어니부기");

	//my_DB.FriendRequest("꼬부기","파이리");

	//my_DB.CreateNewAccount("꼬부기", "1234");

	//my_DB.FriendAccept("꼬부기", "파이리");

	my_DB.GetFriendList("파이리", Friend_List);

	while (true)
	{

	}
}