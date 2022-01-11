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

BOOL DHNetWork::Initialize(unsigned short _Debug_Option/* = DHDEBUG_NONE*/)
{
	g_Debug_Option = _Debug_Option;

	return LOGIC_SUCCESS;
}

BOOL DHNetWork::Send(Packet_Header* _Packet, int _SendType /*= SEND_TYPE_BROADCAST*/, SOCKET _Socket /*= INVALID_SOCKET*/)
{
	if (Current_Type == TYPE_NONSET)
	{
		PrintTypeErrMessage();
		return LOGIC_FAIL;
	}

	if (Current_Type == TYPE_DHCLIENT && _Socket != INVALID_SOCKET)
	{
		std::cout << "[DHNetWork] Client�� ������ ��Ʈ��ũ���� ����� �������Ͽ� Send�� �մϴ�.\n[Send ���] �ش� Socket ������ ���õ�.\n" << std::endl;
	}

	return m_NetWork->Send(_Packet, (int)_SendType, _Socket);
}

BOOL DHNetWork::Connect(unsigned short _Port, std::string _IP)
{
	/// �̹� ������ ������ ���
	if (Current_Type == TYPE_DHSERVER)
	{
		PrintTypeErrMessage();
		return LOGIC_FAIL;
	}

	if (Current_Type == TYPE_NONSET)
	{
		/// Ŭ���̾�Ʈ�ν� NetWork ����.
		Current_Type = TYPE_DHCLIENT;
		m_NetWork = new DHClient();
		// ����� �ɼ� ����
		m_NetWork->SetDebug(g_Debug_Option);
	}

	/// ���� ��Ʈ�� IP�� ���� ��Ʈ��ũ ����.
	return m_NetWork->Connect(_Port, _IP);
}

BOOL DHNetWork::Accept(unsigned short _Port, unsigned short _Max_User_Count, unsigned short _Work_Thread_Count)
{
	/// �̹� Ŭ���̾�Ʈ�� �����Ǿ��ų� Accept�� �ι��̻� ȣ���ϴ� ���.
	if (Current_Type == TYPE_DHCLIENT)
	{
		PrintTypeErrMessage();
		return LOGIC_FAIL;
	}

	if (Current_Type != TYPE_DHSERVER)
	{
		/// �����ν� NetWork ����.
		Current_Type = TYPE_DHSERVER;
		m_NetWork = new DHServer();
		// ����� �ɼ� ����
		m_NetWork->SetDebug(g_Debug_Option);
	}

	/// ���� ��Ʈ�ν� ������ ����, Max_User ���� �������ش�.
	return m_NetWork->Accept(_Port, _Max_User_Count, _Work_Thread_Count);
}

BOOL DHNetWork::Disconnect(SOCKET _Socket)
{
	/// Ŭ���̾�Ʈ�� �����Ǿ��ų� �ƹ��� ������ �Ǿ����� �ʴٸ� Disconnect �Լ��� ����� �� ����.
	if (Current_Type == TYPE_DHCLIENT || Current_Type == TYPE_NONSET)
	{
		std::cout << "[DHNetWork] Disconnect �Լ��� ������ ������ ��Ʈ��ũ���� ��밡���մϴ�.\n" << std::endl;
		return LOGIC_FAIL;
	}

	/// �ش��ϴ� ������ ������ ������.
	return m_NetWork->Disconnect(_Socket);
}

BOOL DHNetWork::Recv(std::vector<Network_Message>& _Message_Vec)
{
	/// ���� ��Ʈ��ũ�� �����Ǿ����� �������.
	if (Current_Type == TYPE_NONSET)
	{
		PrintTypeErrMessage();
		return LOGIC_FAIL;
	}

	/// ���� / Ŭ���̾�Ʈ�� �ش��ϴ� �Լ��� ȣ������.
	return m_NetWork->Recv(_Message_Vec);
}

BOOL DHNetWork::End()
{	
	if (m_NetWork != nullptr)
	{
		/// ���� / Ŭ���̾�Ʈ�� �ش��ϴ� �Լ��� ȣ������.
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
		std::cout << "[DHNetWork] Connect()�� Accept()�� ȣ������ �ʾҽ��ϴ�.\n- ��Ʈ��ũ�� ������ �ȵǾ����� -\n" << std::endl;
		return;
	}
	else if (Current_Type == TYPE_DHSERVER)
	{
		std::cout << "[DHNetWork] Accept()�� ���� �̹� ������ ������ ��Ʈ��ũ �Դϴ�.\n- �߸��� ��Ʈ��ũ �Լ� ȣ�� -\n" << std::endl;
		return;
	}
	else if (Current_Type == TYPE_DHCLIENT)
	{
		std::cout << "[DHNetWork] Connect()�� ���� �̹� Ŭ���̾�Ʈ�� ������ ��Ʈ��ũ �Դϴ�.\n- �߸��� ��Ʈ��ũ �Լ� ȣ�� -\n" << std::endl;
		return;
	}
}