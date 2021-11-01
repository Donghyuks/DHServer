#pragma once

#include <QtWidgets/QMainWindow>
#include <QMessageBox>
#include <QTextCodec>
#include <QShortcut>
#include "ui_GameLauncher.h"
#include "GamePlayLauncher.h"
#include "AccountCreate.h"
#include "C2DB.h"

class LoginLauncher : public QMainWindow
{
    Q_OBJECT

public:
    LoginLauncher(QWidget *parent = Q_NULLPTR);

private:
    AccountCreate* m_MakeAccount = nullptr;       // �α��� ����
    GamePlayLauncher* m_PlayLauncher = nullptr;     // ���� ���� ����

private:
    Ui::GameLauncherClass ui;
    // �α��������� ��ȸ�ϱ����� DB
    C2DB* my_LoginDB = nullptr;
    QTextCodec* codec;	// �ѱ� ����� ���� �ڵ�
    QShortcut* Enter_Key; // ����Ű�� ���� �α���

public slots:
    void ButtonClicked();
    void CreateAccountButtonClicked();
    void EnterEvent();

private:
    QString ConvertKR(QByteArray _Text);
    void ClearText();
};
