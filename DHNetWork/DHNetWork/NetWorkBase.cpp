#include "NetWorkBase.h"
#include <locale.h>
#include <tchar.h>

NetWorkBase::NetWorkBase()
{
	/// �ѱۼ���.
	_wsetlocale(LC_ALL, L"korean");
	/// ������ �ʱ�ȭ.
	WSADATA wsa;

	TCHAR Error_Buffer[ERROR_MSG_BUFIZE] = { 0, };

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		/// TCHAR�� ���� �����ڵ�/��Ƽ����Ʈ�� ������ ��Ȳ�� ���׸��ϰ� ������ �� �ֵ��� �Ѵ�.
		_stprintf_s(Error_Buffer, _countof(Error_Buffer), _T("[TCP ����] ���� �߻� -- WSAStartup() :"));
		err_display(Error_Buffer);
	}

	/// DisconnectEX �� ����ϱ� ���� �Լ� ������ ���۰���.

	// 1. temp ������ ����.
	SOCKET tempSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, NULL, WSA_FLAG_OVERLAPPED);
	DWORD dwByte;	/// ó�� Byte
	GUID GuidDisconnect = WSAID_DISCONNECTEX;

	// 2. DisconnectEx �Լ� ������ ȹ�� �� �޸𸮿� ���.
	WSAIoctl(tempSocket, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidDisconnect, sizeof(GuidDisconnect),
		&lpfnDisconnectEx, sizeof(lpfnDisconnectEx), &dwByte, NULL, NULL);

	// ���� ���н�..
	if (!lpfnDisconnectEx)
	{
		err_display(_T("DisconnectEX ������ �����߻�."));
		assert(false);
	}

	// ������ ���� ������ ������ ����.
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
		_stprintf_s(Error_Buffer, _countof(Error_Buffer), _T("[TCP ����] ���� �߻� -- DisconnectEx �Լ�"));
		err_display(Error_Buffer);
		WSACleanup();
	}
}
