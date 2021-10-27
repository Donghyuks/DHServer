#pragma once

#ifdef C2DB_EXPORTS
#define C2DB_DLL __declspec(dllexport)
#else
#define C2DB_DLL __declspec(dllimport)
#endif

#define SELECTSTRING std::string("SELECT ") 
#define INSERTSTRING std::string("INSERT INTO ") 
#define DELETESTRING std::string("DELETE FROM ") 

#include <string>
#include <mysql.h>

class C2DB_DLL C2DB
{
private:
	MYSQL* m_MYSQL = nullptr;
	// 현재 DB서버와의 연결이 되어있는가?
	bool Is_Connect = false;

public:
	C2DB();
	~C2DB();

public:
	bool ConnectDB(std::string _Server_IP, std::string _User_ID, std::string _User_Password, std::string _DB_Name, unsigned int _Port);

	bool SearchID(std::string _User_ID);
	bool ComparePassword(std::string _User_ID, std::string _User_Password);
	bool CreateNewAccount(std::string _User_ID, std::string _User_Password);
	void DeleteAccount(std::string _User_ID);
};

