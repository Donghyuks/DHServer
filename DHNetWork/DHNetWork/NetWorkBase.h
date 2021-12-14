#pragma once
/// 서버나 클라이언트에 공통적으로 필요한 부분을 클래스로 따로 빼내어 상속한다.
#include "SharedDataStruct.h"

/// 메모리 / 오브젝트 풀
#include "MemoryPool.h"
#include "ObjectPool.h"

class NetWorkBase
{
public:
	NetWorkBase();

private:
	// DisconnectEx를 사용할 함수포인터.
	LPFN_DISCONNECTEX lpfnDisconnectEx = NULL;

protected:
	/// 에러 발생시 에러를 출력해주는 함수.
	void err_display(const TCHAR* const cpcMSG);
	void Disconnect_EX(SOCKET _Disconnect_Socket, LPOVERLAPPED _Overlapped, DWORD _dwflags, DWORD _dwReserved);

public:
	virtual BOOL	Accept(unsigned short _Port, unsigned short _Max_User_Count) { return LOGIC_FAIL; };
	virtual BOOL	Connect(unsigned short _Port, std::string _IP) { return LOGIC_FAIL; };
	virtual BOOL	Recv(std::vector<Network_Message>& _Message_Vec) = 0;
	virtual BOOL	Send(Packet_Header* _Packet, SOCKET _Socket = INVALID_SOCKET) = 0;
	virtual BOOL	Disconnect(SOCKET _Socket) { return LOGIC_FAIL; };
	virtual BOOL	End() = 0;
};

