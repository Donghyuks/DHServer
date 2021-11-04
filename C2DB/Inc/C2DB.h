#pragma once

#ifdef C2DB_EXPORTS
#define C2DB_DLL __declspec(dllexport)
#else
#define C2DB_DLL __declspec(dllimport)
#endif

#include <string>
class C2DBCore;

class C2DB_DLL C2DB
{
private:
	C2DBCore* m_C2DB = nullptr;

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

