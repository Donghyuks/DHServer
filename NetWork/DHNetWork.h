#pragma once

#ifdef NETWORK_EXPORTS
#define NETWORK_DLL __declspec(dllexport)
#else
#define NETWORK_DLL __declspec(dllimport)
#endif

#include "Server.h"
#include "Client.h"

enum class NETWORK_DLL NetWork_Type
{
	Server,
	Client
};

class NETWORK_DLL DHNetWork : public C2NetworkAPIBase
{

private:
	/// 현재 네트워크를 클라 / 서버 무엇으로 사용할지 ??
	NetWork_Type m_NetWork_Type;

	/// 클라이언트와 서버셋팅.
	Client* m_Client = nullptr;
	Server* m_Server = nullptr;

public:
	DHNetWork(NetWork_Type Create_NetWork_Type, unsigned short _PORT, std::string _IP = "127.0.0.1", unsigned short MAX_USER_COUNT = 100);

	virtual BOOL	Recv(std::vector<Network_Message*>& _Message_Vec) override;
	virtual BOOL	Send(Packet_Header* _Packet, SOCKET _Socket = INVALID_SOCKET) override;
	virtual BOOL	Connect(unsigned short _Port, std::string _IP) override;
	virtual BOOL	Accept() override;
	virtual BOOL	Disconnect(SOCKET _Socket) override;
	virtual BOOL	Start() override;
	virtual BOOL	End() override;
};

