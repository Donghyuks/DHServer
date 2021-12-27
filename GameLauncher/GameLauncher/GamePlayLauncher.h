#pragma once

#include <map>
#include <thread>

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

public slots:
	void PushGameStartButton();

private:
	// 서버와 연결하기 위한 네트워크
	DHNetWorkAPI* m_Login_NetWork = nullptr;

	// 서버에서 정보를 받아오기 위한 쓰레드.
	std::thread* m_Work_Thread = nullptr;

	Ui::GamePlayLauncher ui;
	QTextCodec* codec;	// 한글 사용을 위한 코덱
	std::map<QString, QTreeWidgetItem*> WidgetRoot_List;

	QFont* Bold_Rarge;
	QFont* Bold_Small;

private:
	QString ConvertKR(QByteArray _Text);

	void addTreeRoot(QString _Name, QString _description, ColColorType _Color_Type);
	void addTreeChild(QString _Parent_Name, QString _Name, QString _description, ColColorType _Color_Type);
	void ExceptRemoveTreeChild(QString _Except_Parent, QString _Item_Name);
};
