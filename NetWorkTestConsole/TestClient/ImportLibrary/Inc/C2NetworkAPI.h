#pragma once

#ifdef C2NETWORKAPI_EXPORTS
#define C2NETWORKAPI_DLL __declspec(dllexport)
#else
#define C2NETWORKAPI_DLL __declspec(dllimport)

#ifdef _DEBUG
#pragma comment(lib,"C2NetworkAPI_x64d")
#else
#pragma comment(lib,"C2NetworkAPI_x64r")
#endif

#endif

#include <WinSock2.h>
#include <string>
#include <vector>

struct Packet_Header;
struct Network_Message;
class C2NetworkAPIBase;

enum class C2NetWork_Name
{
	DHNet,
	MGNet
};

class C2NETWORKAPI_DLL C2NetWorkAPI
{
public:
	C2NetWorkAPI();
	~C2NetWorkAPI();

	/// ���� ���õ� ��Ʈ��ũ�� ���� ������.
	C2NetworkAPIBase* Set_NetWork = nullptr;

	/// �⺻���� ��ɵ�.. 
	BOOL	Initialize(C2NetWork_Name _Using_NetWork_Name);
	BOOL	Accept(unsigned short _Port, unsigned short _Max_User_Count);
	BOOL	Connect(unsigned short _Port, std::string _IP);
	BOOL	Recv(std::vector<Network_Message*>& _Message_Vec);
	BOOL	Send(Packet_Header* _Packet, SOCKET _Socket = -1);
	BOOL	Disconnect(SOCKET _Socket);
	BOOL	End();

	/// ��Ÿ �߰� ����� ������ ����..
};