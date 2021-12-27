#pragma once

#include <QDialog>
#include <QTextCodec>

namespace Ui { class AccountCreate; };

class DHNetWorkAPI;

class AccountCreate : public QDialog
{
	Q_OBJECT

public:
	AccountCreate(DHNetWorkAPI* _Network, QWidget *parent = Q_NULLPTR);
	~AccountCreate();

private:
	Ui::AccountCreate *ui;
	QTextCodec* codec;	// 한글 사용을 위한 코덱

	// 아이디 생성에 대한 패킷을 보낼 네트워크
	DHNetWorkAPI* m_Login_NetWork = nullptr;

public slots:
	void ButtonClicked();

private:
	QString ConvertKR(QByteArray _Text);
	void ClearText();
};
