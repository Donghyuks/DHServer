#pragma once
/// ������ Ŭ���̾�Ʈ�� ���������� �ʿ��� �κ��� Ŭ������ ���� ������ ����Ѵ�.
#include "SharedDataStruct.h"

/// �޸� / ������Ʈ Ǯ
#include "MemoryPool.h"
#include "ObjectPool.h"

class NetWorkBase
{
public:
	NetWorkBase();

private:
	// DisconnectEx�� ����� �Լ�������.
	LPFN_DISCONNECTEX lpfnDisconnectEx = NULL;

protected:
	/// ���� �߻��� ������ ������ִ� �Լ�.
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

