#pragma once

#include <QtWidgets/QMainWindow>
#include <QMessageBox>
#include <QTextCodec>
#include <QShortcut>
#include "ui_GameLauncher.h"
#include "GamePlayLauncher.h"
#include "AccountCreate.h"
#include "SharedDataStruct.h"

class DHNetWorkAPI;

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
    // 로그인 서버에 접속하기위한 네트워크 및 Send_Recv 패킷
    DHNetWorkAPI* m_Login_NetWork = nullptr;
	S2C_Packet* m_Recv_Packet = new S2C_Packet();
	C2S_Packet m_Send_Packet;
    // 서버로 부터 받은 메세지를 처리하기 위한 Vector
    std::vector<Network_Message> Msg_Vec;
    // 입력받은 아이디/패스워드 string 및 고유 Key값
    QString _ID;
    QString _Password;
    std::string _ID_String;
    std::string _Password_String;
    int _User_Key = -1;


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
