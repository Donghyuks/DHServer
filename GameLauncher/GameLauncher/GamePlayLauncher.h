#pragma once

#include <map>

#include <QDialog>
#include <QString>
#include <QTextBrowser>
#include <QTreeWidget>
#include <QBrush>
#include <QFont>

#include "ui_GamePlayLauncher.h"

class GamePlayLauncher : public QDialog
{
	Q_OBJECT

	enum ColColorType
	{
		Green,
		Red,
	};
public:
	GamePlayLauncher(QDialog*parent = Q_NULLPTR);
	~GamePlayLauncher();

private:
	Ui::GamePlayLauncher ui;
	QTextCodec* codec;	// 한글 사용을 위한 코덱
	std::map<QString, QTreeWidgetItem*> WidgetRoot_List;

	QBrush* Green_Brush;
	QBrush* Red_Brush;

	QFont* Bold_Rarge;

private:
	QString ConvertKR(QByteArray _Text);

	void addTreeRoot(QString _Name, QString _description, ColColorType _Color_Type);
	void addTreeChild(QString _Parent_Name, QString _Name, QString _description, ColColorType _Color_Type);
	void RemoveTreeChild(QString _Parent_Name, QString _Item_Name);

};
