#include "DHDB.h"

int main()
{
	bool result_flag = false;

	DHDB my_DB;

	result_flag = my_DB.ConnectDB("221.163.91.100", "CDH", "ehxk2Rnfwoa!", "LOGIN", 3306);

	std::vector<std::string> Friend_List;

	//unsigned int _Identifier = my_DB.GetIdentifier("��Ϻα�");

	//my_DB.FriendRequest("���α�","���̸�");

	//my_DB.CreateNewAccount("���α�", "1234");

	//my_DB.FriendAccept("���α�", "���̸�");

	my_DB.GetFriendList("���̸�", Friend_List);

	while (true)
	{

	}
}