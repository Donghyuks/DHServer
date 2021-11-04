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
	
protected:
	// 디버거 생성.
	//DHDebugger m_Debugger;
	// 메모리풀 생성.
	MemoryPool m_MemoryPool;

private:
	// DisconnectEx를 사용할 함수포인터.
	LPFN_DISCONNECTEX lpfnDisconnectEx = NULL;

protected:
	/// 에러 발생시 에러를 출력해주는 함수.
	void err_display(const TCHAR* const cpcMSG);
	void Disconnect_EX(SOCKET _Disconnect_Socket, LPOVERLAPPED _Overlapped, DWORD _dwflags, DWORD _dwReserved);
};

