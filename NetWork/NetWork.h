#pragma once

#ifndef NETWORK_EXPORTS
	#define NETWORK_DLL __declspec(dllimport)
#else
	#define NETWORK_DLL __declspec(dllexport)
#endif

class NETWORK_DLL NetWork
{
	enum class NetWork_Type
	{
		Server,
		Client
	};

private:
	/// ���� ��Ʈ��ũ�� Ŭ�� / ���� �������� ������� ??
	NetWork_Type m_NetWork_Type;

	/// Ŭ���̾�Ʈ�� ��������.
	DHClient* m_DHClient = nullptr;
	DHServer* m_DHServer = nullptr;

public:
	DHNetWork(NetWork_Type Create_NetWork_Type, unsigned short _PORT, std::string _IP = "127.0.0.1", unsigned short MAX_USER_COUNT = 0);

	bool Start();
	bool Send(Packet_Header* Send_Packet);
	bool Recv(CatchMind_Data** Recv_Packet, char* MsgBuff, CatchMind_Player* _Player = nullptr);
	bool End();

	/// Ŭ���̾�Ʈ ���� �Լ�.
	bool IsConnectServer();

	/// ���� ���� �Լ�.
	/// ���� ���� ����� 4������ üũ
	bool IsFullJoin();
	/// ������� ����� ������� üũ.
	int GetCurrentClientNumber();
	/// ���� Ÿ���� ������ ��� Ŭ���̾�Ʈ���� �޼����� ������ �Լ�.
	bool SendExceptTargetPlayer(Packet_Header* Send_Packet, CatchMind_Player _Target_Player);
	/// ���� Ÿ�� ���� �޼����� ������ �Լ�.
	bool SendTargetPlayer(Packet_Header* Send_Packet, CatchMind_Player _Target_Player);
	/// Ŭ���̾�Ʈ�� ������ �ߴ��� ����.
	bool IsAcceptBefore();
	/// Ŭ���̾�Ʈ�� ��Ų���� �ִ°�?
	bool IsDisconnectBefore();
	/// ȣ��Ʈ ����.
	void SetHost(CatchMind_Player _Player);
	/// ���� ȣ��Ʈ �ѱ��. (�������� ȣ��Ʈ)
	void SetNextHost(CatchMind_Player _Player);
	/// ���� ȣ��Ʈ �޾ƿ���.
	CatchMind_Player GetCurrentHost();
};

