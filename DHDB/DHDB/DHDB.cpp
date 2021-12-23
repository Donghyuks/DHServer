#include "DHDB.h"
#include "DHDBCore.h"

DHDB::DHDB()
{
	m_C2DB = new DHDBCore();
}

DHDB::~DHDB()
{
	m_C2DB->~DHDBCore();
}

bool DHDB::ConnectDB(std::string _Server_IP, std::string _User_ID, std::string _User_Password, std::string _DB_Name, unsigned int _Port)
{
	return m_C2DB->ConnectDB(_Server_IP, _User_ID, _User_Password, _DB_Name, _Port);
}

bool DHDB::SearchID(std::string _User_ID)
{
	return m_C2DB->SearchID(_User_ID);
}

bool DHDB::ComparePassword(std::string _User_ID, std::string _User_Password)
{
	return m_C2DB->ComparePassword(_User_ID, _User_Password);
}

bool DHDB::CreateNewAccount(std::string _User_ID, std::string _User_Password)
{
	return m_C2DB->CreateNewAccount(_User_ID, _User_Password);
}

bool DHDB::GetFriendList(std::string _User_ID, std::vector<std::string>& _Friend_List)
{
	return m_C2DB->GetFriendList(_User_ID, _Friend_List);
}

unsigned int DHDB::GetIdentifier(std::string _User_ID)
{
	return m_C2DB->GetIdentifier(_User_ID);
}

void DHDB::DeleteAccount(std::string _User_ID)
{
	m_C2DB->DeleteAccount(_User_ID);
}
