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
	QTextCodec* codec;	// �ѱ� ����� ���� �ڵ�

	// ���̵� ������ ���� ��Ŷ�� ���� ��Ʈ��ũ
	DHNetWorkAPI* m_Login_NetWork = nullptr;

public slots:
	void ButtonClicked();

private:
	QString ConvertKR(QByteArray _Text);
	void ClearText();
};
