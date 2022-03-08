#pragma once

#define SELECTSTRING std::string("SELECT ") 
#define INSERTSTRING std::string("INSERT INTO ") 
#define DELETESTRING std::string("DELETE FROM ") 
#define UPDATESTRING std::string("UPDATE ") 

#include <string>
#include <mysql.h>
#include <vector>
#include <set>

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
	bool QueryDB(std::string _Query, int* _Fields_Count, std::vector<std::string>& _Query_Data);
	bool ConnectDB(std::string _Server_IP, std::string _User_ID, std::string _User_Password, std::string _DB_Name, unsigned int _Port);
	bool SearchID(std::string _User_ID);
	bool ComparePassword(std::string _User_ID, std::string _User_Password);
	bool CreateNewAccount(std::string _User_ID, std::string _User_Password);
	bool FriendRequest(std::string _User_ID, std::string _Friend_ID);
	bool FriendAccept(std::string _User_ID, std::string _Friend_ID, bool _Is_Accept);
	bool GetFriendList(std::string _User_ID, std::set<std::string>& _Friend_List, std::set<std::string>& _Friend_Request_List);
	unsigned int GetIdentifier(std::string _User_ID);
	void DeleteAccount(std::string _User_ID);
};

