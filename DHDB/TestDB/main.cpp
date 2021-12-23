#include "DHDB.h"

int main()
{
	bool result_flag = false;

	DHDB my_DB;

	result_flag = my_DB.ConnectDB("221.163.91.100", "CDH", "ehxk2Rnfwoa!", "LOGIN", 3306);

	std::vector<std::string> Friend_List;

	unsigned int _Identifier = my_DB.GetIdentifier("��Ϻα�");

	my_DB.GetFriendList("���α�", Friend_List);

	/// ������
	result_flag = my_DB.CreateNewAccount("����Ȳ", "1234");	// true

	result_flag = my_DB.CreateNewAccount("���α�", "12345");	// true

	result_flag = my_DB.CreateNewAccount("���α�", "1234");	// false

	result_flag = my_DB.SearchID("���α�");					// true
	
	result_flag = my_DB.SearchID("����");					// false

	result_flag = my_DB.ComparePassword("����Ȳ", "1234");	// true

	result_flag = my_DB.ComparePassword("���α�", "1234");	// false

	my_DB.DeleteAccount("����Ȳ");

	my_DB.DeleteAccount("���α�");

	while (true)
	{

	}
}