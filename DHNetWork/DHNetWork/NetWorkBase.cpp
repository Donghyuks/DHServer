#include "NetWorkBase.h"
#include <locale.h>
#include <tchar.h>

NetWorkBase::NetWorkBase()
{
	/// 한글설정.
	_wsetlocale(LC_ALL, L"korean");
	/// 윈소켓 초기화.
	WSADATA wsa;

	TCHAR Error_Buffer[ERROR_MSG_BUFIZE] = { 0, };

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		/// TCHAR을 통해 유니코드/멀티바이트의 가변적 상황에 제네릭하게 동작할 수 있도록 한다.
		_stprintf_s(Error_Buffer, _countof(Error_Buffer), _T("[TCP 서버] 에러 발생 -- WSAStartup() :"));
		err_display(Error_Buffer);
	}

	/// DisconnectEX 를 사용하기 위한 함수 포인터 제작과정.

	// 1. temp 소켓을 생성.
	SOCKET tempSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, NULL, WSA_FLAG_OVERLAPPED);
	DWORD dwByte;	/// 처리 Byte
	GUID GuidDisconnect = WSAID_DISCONNECTEX;

	// 2. DisconnectEx 함수 포인터 획득 및 메모리에 등록.
	WSAIoctl(tempSocket, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidDisconnect, sizeof(GuidDisconnect),
		&lpfnDisconnectEx, sizeof(lpfnDisconnectEx), &dwByte, NULL, NULL);

	// 생성 실패시..
	if (!lpfnDisconnectEx)
	{
		err_display(_T("DisconnectEX 생성중 오류발생."));
		assert(false);
	}

	// 생성에 사용된 임의의 소켓은 해제.
	closesocket(tempSocket);
}

void NetWorkBase::err_display(const TCHAR* const cpcMSG)
{
	LPVOID lpMsgBuf = nullptr;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr,
		WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<TCHAR*>(&lpMsgBuf),
		0,
		nullptr);

	_tprintf(_T("%s %s"), cpcMSG, reinterpret_cast<TCHAR*>(lpMsgBuf));

	LocalFree(lpMsgBuf);
}

void NetWorkBase::Disconnect_EX(SOCKET _Disconnect_Socket, LPOVERLAPPED _Overlapped, DWORD _dwflags, DWORD _dwReserved)
{
	TCHAR Error_Buffer[ERROR_MSG_BUFIZE] = { 0, };

	lpfnDisconnectEx(_Disconnect_Socket, _Overlapped, _dwflags, _dwReserved);
	int Err = WSAGetLastError();
	if (ERROR_IO_PENDING != WSAGetLastError())
	{
		wprintf(L"DisconnectEX Function error: %u\n", WSAGetLastError());
		_stprintf_s(Error_Buffer, _countof(Error_Buffer), _T("[TCP 서버] 에러 발생 -- DisconnectEx 함수"));
		err_display(Error_Buffer);
		WSACleanup();
	}
}
