#pragma once
/// stl 을 사용할 것 이기때문에 템플릿의 명시적 인스턴스화가 힘들다.. 따라서 4251 경고에 대한 부분은 무시해주자.
#pragma warning(disable : 4251)

#ifdef C2NETWORKAPI_EXPORTS
#define C2NETWORKAPI_DLL __declspec(dllexport)
#else
#define C2NETWORKAPI_DLL __declspec(dllimport)
#endif

#include <WS2tcpip.h>
#include <MSWSock.h>
#include <vector>
#include <string>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

/// 1바이트 정렬 시작.
#pragma pack(push , 1)

struct C2NETWORKAPI_DLL Packet_Header
{
	unsigned short Packet_Size;
	unsigned short Packet_Type;
};

struct C2NETWORKAPI_DLL Network_Message
{
	SOCKET			Socket;
	Packet_Header*	Packet;
};

/// 1바이트 정렬 끝.
#pragma pack(pop)

/// 네트워크 타입에 대한 정의.
enum class C2NETWORKAPI_DLL NetWork_Type
{
	Server,
	Client
};

/// Adapter Pattern 을 사용하기위한 InterFace
class C2NETWORKAPI_DLL C2NetworkAPIBase
{
public:
	virtual BOOL	Recv(std::vector<Network_Message*>& _Message_Vec) abstract;
	virtual BOOL	Send(Packet_Header* _Packet, SOCKET _Socket = INVALID_SOCKET) abstract;
	virtual BOOL	Connect(unsigned short _Port, std::string _IP) abstract;
	virtual BOOL	Accept() abstract;
	virtual BOOL	Disconnect(SOCKET _Socket) abstract;
	virtual BOOL	Start() abstract;
	virtual BOOL	End() abstract;
};