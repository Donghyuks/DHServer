#include "DHDB.h"

int main()
{
	bool result_flag = false;

	DHDB my_DB;

	result_flag = my_DB.ConnectDB("221.163.91.100", "CDH", "ehxk2Rnfwoa!", "LOGIN", 3306);

	std::vector<std::string> Friend_List;

	unsigned int _Identifier = my_DB.GetIdentifier("어니부기");

	my_DB.GetFriendList("꼬부기", Friend_List);

	/// 예상결과
	result_flag = my_DB.CreateNewAccount("서규황", "1234");	// true

	result_flag = my_DB.CreateNewAccount("꼬부기", "12345");	// true

	result_flag = my_DB.CreateNewAccount("꼬부기", "1234");	// false

	result_flag = my_DB.SearchID("꼬부기");					// true
	
	result_flag = my_DB.SearchID("석유");					// false

	result_flag = my_DB.ComparePassword("서규황", "1234");	// true

	result_flag = my_DB.ComparePassword("꼬부기", "1234");	// false

	my_DB.DeleteAccount("서규황");

	my_DB.DeleteAccount("꼬부기");

	while (true)
	{

	}
}