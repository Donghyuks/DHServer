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

bool DHDB::QueryDB(std::string _Query, int* _Fields_Count, std::vector<std::string>& _Query_Data)
{
	return m_C2DB->QueryDB(_Query, _Fields_Count, _Query_Data);
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

bool DHDB::FriendRequest(std::string _User_ID, std::string _Friend_ID)
{
	return m_C2DB->FriendRequest(_User_ID, _Friend_ID);
}

bool DHDB::FriendAccept(std::string _User_ID, std::string _Friend_ID, bool _Is_Accept)
{
	return m_C2DB->FriendAccept(_User_ID, _Friend_ID, _Is_Accept);
}

bool DHDB::GetFriendList(std::string _User_ID, std::set<std::string>& _Friend_List, std::set<std::string>& _Friend_Request_List)
{
	return m_C2DB->GetFriendList(_User_ID, _Friend_List, _Friend_Request_List);
}

unsigned int DHDB::GetIdentifier(std::string _User_ID)
{
	return m_C2DB->GetIdentifier(_User_ID);
}

void DHDB::DeleteAccount(std::string _User_ID)
{
	m_C2DB->DeleteAccount(_User_ID);
}
