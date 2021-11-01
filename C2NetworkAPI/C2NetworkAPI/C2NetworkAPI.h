#pragma once

#ifdef C2NETWORKAPI_EXPORTS
#define C2NETWORKAPI_DLL __declspec(dllexport)
#else
#define C2NETWORKAPI_DLL __declspec(dllimport)
#endif

#include <WinSock2.h>
#include <string>
#include <vector>

struct Packet_Header;
struct Network_Message;
class C2NetworkAPIBase;
enum class NetWork_Type;

enum class C2NETWORKAPI_DLL NetWork_Name
{
	DHNet,
	MGNet
};

class C2NETWORKAPI_DLL C2NetWorkAPI
{
public:
	C2NetWorkAPI(NetWork_Name _Using_NetWork_Name, NetWork_Type _Create_NetWork_Type, unsigned short _PORT, std::string _IP = "127.0.0.1", unsigned short MAX_USER_COUNT = 100);
	~C2NetWorkAPI();

	/// ���� ���õ� ��Ʈ��ũ�� ���� ������.
	C2NetworkAPIBase* Set_NetWork = nullptr;

	/// �⺻���� ��ɵ�.. 
	BOOL	Recv(std::vector<Network_Message*>& _Message_Vec);
	BOOL	Send(Packet_Header* _Packet, SOCKET _Socket = -1);
	BOOL	Connect(unsigned short _Port, std::string _IP);
	BOOL	Accept();
	BOOL	Disconnect(SOCKET _Socket);
	BOOL	Start();
	BOOL	End();

	/// ��Ÿ �߰� ����� ������ ����..
};