#pragma once

#include <QtWidgets/QMainWindow>
#include <QMessageBox>
#include <QShortcut>
#include "ui_GameLauncher.h"
#include "GamePlayLauncher.h"
#include "C2DB.h"

class GameLauncher : public QMainWindow
{
    Q_OBJECT

public:
    GameLauncher(QWidget *parent = Q_NULLPTR);

private:
    Ui::GameLauncherClass ui;
    // �α��������� ��ȸ�ϱ����� DB
    C2DB* my_LoginDB = nullptr;
    QTextCodec* codec;	// �ѱ� ����� ���� �ڵ�
    QShortcut* Enter_Key; // ����Ű�� ���� �α���

public slots:
    void ButtonClicked();
    void EnterEvent();

private:
    QString ConvertKR(QByteArray _Text);
};
