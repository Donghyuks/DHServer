#include "C2DB.h"

int main()
{
	bool result_flag = false;

	C2DB my_DB;

	result_flag = my_DB.ConnectDB("192.168.0.56", "CDH", "ehxk2Rnfwoa!", "LOGIN", 3306);

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