#pragma once

#ifdef DHNETWORK_EXPORTS
#define NETWORK_DLL __declspec(dllexport)
#else
#define NETWORK_DLL __declspec(dllimport)

#ifdef _DEBUG
#pragma comment(lib,"DHNetWork_x64d")
#else
#pragma comment(lib,"DHNetWork_x64r")
#endif

#endif

#include "C2NetworkAPIDefine.h"
#define TYPE_NONSET		0b0000
#define TYPE_DHSERVER	0b0001
#define TYPE_DHCLIENT	0b0010

class NetWorkBase;

class NETWORK_DLL DHNetWork : public DHNetworkAPIBase
{

private:
	/// 클라이언트와 서버셋팅.
	NetWorkBase* m_NetWork = nullptr;
	int Current_Type = TYPE_NONSET;

private:
	void PrintTypeErrMessage();

public:
	DHNetWork();
	~DHNetWork();

	/// 초기화
	virtual BOOL	Initialize() override;
	/// 서버일 경우 Accept 호출
	virtual BOOL	Accept(unsigned short _Port, unsigned short _Max_User_Count) override;
	/// 클라일 경우 Connect 호출
	virtual BOOL	Connect(unsigned short _Port, std::string _IP) override;
	/// 공통 Function
	virtual BOOL	Recv(std::vector<Network_Message*>& _Message_Vec) override;
	virtual BOOL	Send(Packet_Header* _Packet, SOCKET _Socket = INVALID_SOCKET) override;
	virtual BOOL	Disconnect(SOCKET _Socket) override;
	virtual BOOL	End() override;
};

