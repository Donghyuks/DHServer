#pragma once

#ifdef DHNETWORK_EXPORTS
#define NETWORK_DLL __declspec(dllexport)
#else
#define NETWORK_DLL __declspec(dllimport)

#ifdef _DEBUG
#pragma comment(lib,"DHNetWork_x64d")
#else
#pragma comment(lib,"DHNetWork_x64r")
#endif

#endif

#include "DHNetWorkAPIDefine.h"
// ���� ��Ʈ��ũ�� �������� ��, �������� Ŭ���̾�Ʈ����?
#define TYPE_NONSET				0b0000
#define TYPE_DHSERVER			0b0001
#define TYPE_DHCLIENT			0b0010

class NetWorkBase;

class NETWORK_DLL DHNetWork : public DHNetworkAPIBase
{

private:
	/// Ŭ���̾�Ʈ�� ��������.
	NetWorkBase* m_NetWork = nullptr;
	int Current_Type = TYPE_NONSET;

private:
	void PrintTypeErrMessage();

public:
	DHNetWork();
	~DHNetWork();

	/// �ʱ�ȭ
	virtual BOOL	Initialize() override;
	/// ������ ��� Accept ȣ��
	virtual BOOL	Accept(unsigned short _Port, unsigned short _Max_User_Count) override;
	/// Ŭ���� ��� Connect ȣ��
	virtual BOOL	Connect(unsigned short _Port, std::string _IP) override;
	/// ���� Function
	virtual BOOL	Recv(std::vector<Network_Message>& _Message_Vec) override;
	virtual BOOL	Send(Packet_Header* _Packet, SOCKET _Socket = INVALID_SOCKET) override;
	virtual BOOL	Disconnect(SOCKET _Socket) override;
	virtual BOOL	End() override;
};

