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
    AccountCreate* m_MakeAccount = nullptr;       // 로그인 관리
    GamePlayLauncher* m_PlayLauncher = nullptr;     // 게임 런쳐 관리

private:
    Ui::GameLauncherClass ui;
    // 로그인정보를 조회하기위한 DB
    C2DB* my_LoginDB = nullptr;
    QTextCodec* codec;	// 한글 사용을 위한 코덱
    QShortcut* Enter_Key; // 엔터키를 통한 로그인

public slots:
    void ButtonClicked();
    void CreateAccountButtonClicked();
    void EnterEvent();

private:
    QString ConvertKR(QByteArray _Text);
    void ClearText();
};
