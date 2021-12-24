#pragma once

#define SELECTSTRING std::string("SELECT ") 
#define INSERTSTRING std::string("INSERT INTO ") 
#define DELETESTRING std::string("DELETE FROM ") 

#include <string>
#include <mysql.h>
#include <vector>

class DHDBCore
{
private:
	MYSQL* m_MYSQL = nullptr;
	// ���� DB�������� ������ �Ǿ��ִ°�?
	bool Is_Connect = false;

public:
	DHDBCore();
	~DHDBCore();

public:
	bool ConnectDB(std::string _Server_IP, std::string _User_ID, std::string _User_Password, std::string _DB_Name, unsigned int _Port);
	bool SearchID(std::string _User_ID);
	bool ComparePassword(std::string _User_ID, std::string _User_Password);
	bool CreateNewAccount(std::string _User_ID, std::string _User_Password);
	bool GetFriendList(std::string _User_ID, std::vector<std::string>& _Friend_List);
	unsigned int GetIdentifier(std::string _User_ID);
	void DeleteAccount(std::string _User_ID);
};
