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
	/// 현재 네트워크를 클라 / 서버 무엇으로 사용할지 ??
	NetWork_Type m_NetWork_Type;

	/// 클라이언트와 서버셋팅.
	DHClient* m_DHClient = nullptr;
	DHServer* m_DHServer = nullptr;

public:
	DHNetWork(NetWork_Type Create_NetWork_Type, unsigned short _PORT, std::string _IP = "127.0.0.1", unsigned short MAX_USER_COUNT = 0);

	bool Start();
	bool Send(Packet_Header* Send_Packet);
	bool Recv(CatchMind_Data** Recv_Packet, char* MsgBuff, CatchMind_Player* _Player = nullptr);
	bool End();

	/// 클라이언트 전용 함수.
	bool IsConnectServer();

	/// 서버 전용 함수.
	/// 현재 들어온 사람이 4명인지 체크
	bool IsFullJoin();
	/// 현재들어온 사람이 몇명인지 체크.
	int GetCurrentClientNumber();
	/// 현재 타겟을 제외한 모든 클라이언트에게 메세지를 보내는 함수.
	bool SendExceptTargetPlayer(Packet_Header* Send_Packet, CatchMind_Player _Target_Player);
	/// 현재 타겟 에게 메세지를 보내는 함수.
	bool SendTargetPlayer(Packet_Header* Send_Packet, CatchMind_Player _Target_Player);
	/// 클라이언트가 접속을 했는지 여부.
	bool IsAcceptBefore();
	/// 클라이언트가 끊킨적이 있는가?
	bool IsDisconnectBefore();
	/// 호스트 지정.
	void SetHost(CatchMind_Player _Player);
	/// 다음 호스트 넘기기. (맞춘사람이 호스트)
	void SetNextHost(CatchMind_Player _Player);
	/// 현재 호스트 받아오기.
	CatchMind_Player GetCurrentHost();
};

