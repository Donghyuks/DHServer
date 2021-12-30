#pragma once

#include <map>
#include <thread>
#include <atomic>

#include <QDialog>
#include <QString>
#include <QTextBrowser>
#include <QTreeWidget>
#include <QBrush>
#include <QFont>

#include "ui_GamePlayLauncher.h"

class DHNetWorkAPI;

class GamePlayLauncher : public QDialog
{
	Q_OBJECT

	enum ColColorType
	{
		Ingame_Green,
		Online_Blue,
		Offline_Gray,
	};
public:
	GamePlayLauncher(DHNetWorkAPI* m_NetWork, QDialog* parent = Q_NULLPTR);
	~GamePlayLauncher();

	void ThreadCreate();
	void RecvServerPacket();
	void SetUser(std::string _User_Name);

signals:
	void _UI_Hide();
signals:
	void _UI_Visible();

public slots:
	void PushGameStartButton();
	void PushContentButton();
	void PushFriendRequestButton();
	void HideUI();
	void VisibleUI();

private:
	// 메인 컨텐츠 넘기기 UI 개수
	int Main_Content_Count = 1;
	int Current_Focus_Content_Num = 0;

	// UI 상 X 좌표 최종 시작점
	int _Start_Icon_X = 0;

	// UI의 최대 x 좌표와 시작 y 좌표
	int Content_x = 900;
	int Content_y = 535;

	// Icon_Interval
	int Icon_Interval = 5;

	// Focus_Icon_Size
	int Focus_Icon_x = 29;
	int Focus_Icon_y = 4;

	// Non_Focus_Icon_Size
	int Non_Focus_Icon_x = 9;
	int Non_Focus_Icon_y = 9;

	// 버튼 개수 관리.
	std::vector<QPushButton*> Button_List;

private:
	// 친구 요청에 대한 응답이나 요청에 따른 결과 처리에 필요한 UI데이터
	QSize _Visible_UI_Size = QSize(255,30);
	int _Visible_UI_Start_x = 937;
	int _Visible_UI_Start_y = 195;

	// 친구 목록 Default size
	QSize _Friend_List_Size = QSize(267,531);
	int _Friend_List_Start_x = 930;
	int _Friend_List_Start_y = 190;

	// 친구 요청에 따른 UI 관리
	QLabel* Friend_Request_Result = nullptr;

	// Friend UI 창 간격
	int Friend_UI_Interval = 5;

	// 결과가 보여질 시간
	int Visible_Time = 3;

	// 현재 결과창이 보여지는가?
	std::atomic<bool> _Current_Visible_State = false;
	std::atomic<bool> _Is_Visible_Start = false;
	void SetVisibleRequestResult(bool _Visible);

private:
	// 현재 유저 이름
	std::string m_User_Name;

	// 서버와 연결하기 위한 네트워크
	DHNetWorkAPI* m_Login_NetWork = nullptr;

	// 서버에서 정보를 받아오기 위한 쓰레드.
	std::thread* m_Work_Thread = nullptr;

	Ui::GamePlayLauncher ui;
	QTextCodec* codec;	// 한글 사용을 위한 코덱
	std::map<QString, QTreeWidgetItem*> WidgetRoot_List;

	QFont* Bold_Rarge = nullptr;
	QFont* Bold_Small = nullptr;

	std::map <int, std::string>		Icon_Type_Name;
	std::map <std::string, QIcon>	Icon_List;
	std::map <std::string, QPixmap> Icon_img_List;
	std::map <std::string, QPixmap> BackGround_List;

private:
	QString ConvertKR(QByteArray _Text);

	void addTreeRoot(QString _Name, QString _description, ColColorType _Color_Type);
	void addTreeChild(QString _Parent_Name, std::string _Name, std::string _description, int _Icon_Type, ColColorType _Color_Type);
	void ExceptRemoveTreeChild(QString _Except_Parent, QString _Item_Name);
	void RemoveTreeChildAll();
};
