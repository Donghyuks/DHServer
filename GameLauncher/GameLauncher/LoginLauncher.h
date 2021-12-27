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
    AccountCreate* m_MakeAccount = nullptr;       // �α��� ����
    GamePlayLauncher* m_PlayLauncher = nullptr;     // ���� ���� ����

private:
    Ui::GameLauncherClass ui;
    // �α��� ������ �����ϱ����� ��Ʈ��ũ �� Send_Recv ��Ŷ
    DHNetWorkAPI* m_Login_NetWork = nullptr;
	S2C_Packet* m_Recv_Packet = new S2C_Packet();
	C2S_Packet m_Send_Packet;
    // ������ ���� ���� �޼����� ó���ϱ� ���� Vector
    std::vector<Network_Message> Msg_Vec;
    // �Է¹��� ���̵�/�н����� string �� ���� Key��
    QString _ID;
    QString _Password;
    std::string _ID_String;
    std::string _Password_String;
    int _User_Key = -1;


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
