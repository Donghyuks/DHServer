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
	// ���� ������ �ѱ�� UI ����
	int Main_Content_Count = 1;
	int Current_Focus_Content_Num = 0;

	// UI �� X ��ǥ ���� ������
	int _Start_Icon_X = 0;

	// UI�� �ִ� x ��ǥ�� ���� y ��ǥ
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

	// ��ư ���� ����.
	std::vector<QPushButton*> Button_List;

private:
	// ģ�� ��û�� ���� �����̳� ��û�� ���� ��� ó���� �ʿ��� UI������
	QSize _Visible_UI_Size = QSize(255,30);
	int _Visible_UI_Start_x = 937;
	int _Visible_UI_Start_y = 195;

	// ģ�� ��� Default size
	QSize _Friend_List_Size = QSize(267,531);
	int _Friend_List_Start_x = 930;
	int _Friend_List_Start_y = 190;

	// ģ�� ��û�� ���� UI ����
	QLabel* Friend_Request_Result = nullptr;

	// Friend UI â ����
	int Friend_UI_Interval = 5;

	// ����� ������ �ð�
	int Visible_Time = 3;

	// ���� ���â�� �������°�?
	std::atomic<bool> _Current_Visible_State = false;
	std::atomic<bool> _Is_Visible_Start = false;
	void SetVisibleRequestResult(bool _Visible);

private:
	// ���� ���� �̸�
	std::string m_User_Name;

	// ������ �����ϱ� ���� ��Ʈ��ũ
	DHNetWorkAPI* m_Login_NetWork = nullptr;

	// �������� ������ �޾ƿ��� ���� ������.
	std::thread* m_Work_Thread = nullptr;

	Ui::GamePlayLauncher ui;
	QTextCodec* codec;	// �ѱ� ����� ���� �ڵ�
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
