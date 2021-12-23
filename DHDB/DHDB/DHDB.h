#pragma once

#ifdef DHDB_EXPORTS
#define DHDB_DLL __declspec(dllexport)
#else
#define DHDB_DLL __declspec(dllimport)

#ifdef _DEBUG
#pragma comment(lib,"DHDB_x64d")
#else
#pragma comment(lib,"DHDB_x64r")
#endif
#endif

#include <string>
#include <vector>
class DHDBCore;

class DHDB_DLL DHDB
{
private:
	DHDBCore* m_C2DB = nullptr;

public:
	DHDB();
	~DHDB();

public:
	bool ConnectDB(std::string _Server_IP, std::string _User_ID, std::string _User_Password, std::string _DB_Name, unsigned int _Port);
	bool SearchID(std::string _User_ID);
	bool ComparePassword(std::string _User_ID, std::string _User_Password);
	bool CreateNewAccount(std::string _User_ID, std::string _User_Password);
	bool GetFriendList(std::string _User_ID, std::vector<std::string>& _Friend_List);
	unsigned int GetIdentifier(std::string _User_ID);
	void DeleteAccount(std::string _User_ID);
};

