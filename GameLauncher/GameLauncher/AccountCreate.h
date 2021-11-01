#pragma once

#include <QDialog>
#include <QTextCodec>

class C2DB;

namespace Ui { class AccountCreate; };

class AccountCreate : public QDialog
{
	Q_OBJECT

public:
	AccountCreate(C2DB* _C2DB, QWidget *parent = Q_NULLPTR);
	~AccountCreate();

private:
	Ui::AccountCreate *ui;
	QTextCodec* codec;	// 한글 사용을 위한 코덱
	C2DB* my_LoginDB = nullptr;

public slots:
	void ButtonClicked();

private:
	QString ConvertKR(QByteArray _Text);
	void ClearText();
};
