#include "C2DB.h"
#include "C2DBCore.h"

C2DB::C2DB()
{
	m_C2DB = new C2DBCore();
}

C2DB::~C2DB()
{
	m_C2DB->~C2DBCore();
}

bool C2DB::ConnectDB(std::string _Server_IP, std::string _User_ID, std::string _User_Password, std::string _DB_Name, unsigned int _Port)
{
	return m_C2DB->ConnectDB(_Server_IP, _User_ID, _User_Password, _DB_Name, _Port);
}

bool C2DB::SearchID(std::string _User_ID)
{
	return m_C2DB->SearchID(_User_ID);
}

bool C2DB::ComparePassword(std::string _User_ID, std::string _User_Password)
{
	return m_C2DB->ComparePassword(_User_ID, _User_Password);
}

bool C2DB::CreateNewAccount(std::string _User_ID, std::string _User_Password)
{
	return m_C2DB->CreateNewAccount(_User_ID, _User_Password);
}

void C2DB::DeleteAccount(std::string _User_ID)
{
	m_C2DB->DeleteAccount(_User_ID);
}
