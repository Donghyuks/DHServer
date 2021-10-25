#pragma once
/// ������ Ŭ���̾�Ʈ�� ���������� �ʿ��� �κ��� Ŭ������ ���� ������ ����Ѵ�.
#include "SharedDataStruct.h"

/// �޸� / ������Ʈ Ǯ
#include "MemoryPool.h"
#include "ObjectPool.h"

#ifdef NETWORK_EXPORTS
#define NETWORK_DLL __declspec(dllexport)
#else
#define NETWORK_DLL __declspec(dllimport)
#endif

class NETWORK_DLL NetWorkBase
{
public:
	NetWorkBase();
	
protected:
	// ����� ����.
	//DHDebugger m_Debugger;
	// �޸�Ǯ ����.
	MemoryPool m_MemoryPool;

private:
	// DisconnectEx�� ����� �Լ�������.
	LPFN_DISCONNECTEX lpfnDisconnectEx = NULL;

protected:
	/// ���� �߻��� ������ ������ִ� �Լ�.
	void err_display(const TCHAR* const cpcMSG);
	void Disconnect_EX(SOCKET _Disconnect_Socket, LPOVERLAPPED _Overlapped, DWORD _dwflags, DWORD _dwReserved);
};

