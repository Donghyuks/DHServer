#pragma once
/// stl �� ����� �� �̱⶧���� ���ø��� ����� �ν��Ͻ�ȭ�� �����.. ���� 4251 ��� ���� �κ��� ����������.
#pragma warning(disable : 4251)

#ifdef DHNETWORKAPI_EXPORTS
#define DHNETWORKAPI_DLL __declspec(dllexport)
#else
#define DHNETWORKAPI_DLL __declspec(dllimport)
#endif

#include <WS2tcpip.h>
#include <MSWSock.h>
#include <vector>
#include <string>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

/// 1����Ʈ ���� ����.
#pragma pack(push , 1)

struct DHNETWORKAPI_DLL Packet_Header
{
	size_t			Packet_Size;
	unsigned short	Packet_Type;
};

struct DHNETWORKAPI_DLL Network_Message
{
	SOCKET			Socket;
	Packet_Header*	Packet;
};

/// 1����Ʈ ���� ��.
#pragma pack(pop)

#define PACKET_HEADER_SIZE sizeof(Packet_Header)

/// Adapter Pattern �� ����ϱ����� InterFace
class DHNETWORKAPI_DLL DHNetworkAPIBase
{
//private:
//	void EndCall()
//	{
//		this->End();
//	}

public:
	virtual BOOL	Initialize() abstract;
	virtual BOOL	Recv(std::vector<Network_Message>& _Message_Vec) abstract;
	virtual BOOL	Send(Packet_Header* _Packet, SOCKET _Socket = INVALID_SOCKET) abstract;
	virtual BOOL	Connect(unsigned short _Port, std::string _IP) abstract;
	virtual BOOL	Accept(unsigned short _Port, unsigned short _Max_User_Count) abstract;
	virtual BOOL	Disconnect(SOCKET _Socket) abstract;
	virtual BOOL	End() abstract;
};