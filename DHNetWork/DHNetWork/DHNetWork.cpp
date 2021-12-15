#include "DHNetWork.h"
#include "DHServer.h"
#include "DHClient.h"
#include <iostream>
#include <assert.h>


DHNetWork::DHNetWork()
{

}

DHNetWork::~DHNetWork()
{
	End();
}

BOOL DHNetWork::Initialize()
{
	/// 초기화 작업이 필요하면 넣음..

	return LOGIC_SUCCESS;
}

BOOL DHNetWork::Send(Packet_Header* _Packet, SOCKET _Socket /*= INVALID_SOCKET*/)
{
	if (Current_Type == TYPE_NONSET)
	{
		PrintTypeErrMessage();
		return LOGIC_FAIL;
	}

	if (Current_Type == TYPE_DHCLIENT && _Socket != INVALID_SOCKET)
	{
		std::cout << "[DHNetWork] Client로 생성된 네트워크에선 연결된 서버소켓에 Send를 합니다.\n[Send 경고] 해당 Socket 정보가 무시됨.\n" << std::endl;
	}

	return m_NetWork->Send(_Packet, _Socket);
}

BOOL DHNetWork::Connect(unsigned short _Port, std::string _IP)
{
	/// 이미 서버로 생성된 경우
	if (Current_Type == TYPE_DHSERVER)
	{
		PrintTypeErrMessage();
		return LOGIC_FAIL;
	}

	if (Current_Type == TYPE_NONSET)
	{
		/// 클라이언트로써 NetWork 생성.
		Current_Type = TYPE_DHCLIENT;
		m_NetWork = new DHClient();
	}

	/// 들어온 포트와 IP에 따라 네트워크 연결.
	return m_NetWork->Connect(_Port, _IP);
}

BOOL DHNetWork::Accept(unsigned short _Port, unsigned short _Max_User_Count)
{
	/// 이미 서버로 생성되었거나 Accept를 두번이상 호출하는 경우.
	if (m_NetWork != nullptr || Current_Type == TYPE_DHCLIENT)
	{
		PrintTypeErrMessage();
		return LOGIC_FAIL;
	}

	/// 서버로써 NetWork 생성.
	Current_Type = TYPE_DHSERVER;
	m_NetWork = new DHServer();

	/// 들어온 포트로써 서버를 열고, Max_User 수를 지정해준다.
	return m_NetWork->Accept(_Port, _Max_User_Count);
}

BOOL DHNetWork::Disconnect(SOCKET _Socket)
{
	/// 클라이언트로 생성되었거나 아무런 생성이 되어있지 않다면 Disconnect 함수를 사용할 수 없다.
	if (Current_Type == TYPE_DHCLIENT || Current_Type == TYPE_NONSET)
	{
		std::cout << "[DHNetWork] Disconnect 함수는 서버로 생성된 네트워크에서 사용가능합니다.\n" << std::endl;
		return LOGIC_FAIL;
	}

	/// 해당하는 소켓의 연결을 끊어줌.
	return m_NetWork->Disconnect(_Socket);
}

BOOL DHNetWork::Recv(std::vector<Network_Message>& _Message_Vec)
{
	/// 현재 네트워크가 설정되어있지 않은경우.
	if (Current_Type == TYPE_NONSET)
	{
		PrintTypeErrMessage();
		return LOGIC_FAIL;
	}

	/// 서버 / 클라이언트에 해당하는 함수를 호출해줌.
	return m_NetWork->Recv(_Message_Vec);
}

BOOL DHNetWork::End()
{	
	if (m_NetWork != nullptr)
	{
		/// 서버 / 클라이언트에 해당하는 함수를 호출해줌.
		bool End_result = m_NetWork->End();
		delete m_NetWork;
		m_NetWork = nullptr;
		return End_result;
	}

	return true;
}

void DHNetWork::PrintTypeErrMessage()
{
	if (Current_Type == TYPE_NONSET)
	{
		std::cout << "[DHNetWork] Connect()나 Accept()를 호출하지 않았습니다.\n- 네트워크가 연결이 안되어있음 -\n" << std::endl;
		return;
	}
	else if (Current_Type == TYPE_DHSERVER)
	{
		std::cout << "[DHNetWork] Accept()에 의해 이미 서버로 생성된 네트워크 입니다.\n- 잘못된 네트워크 함수 호출 -\n" << std::endl;
		return;
	}
	else if (Current_Type == TYPE_DHCLIENT)
	{
		std::cout << "[DHNetWork] Connect()에 의해 이미 클라이언트로 생성된 네트워크 입니다.\n- 잘못된 네트워크 함수 호출 -\n" << std::endl;
		return;
	}
}