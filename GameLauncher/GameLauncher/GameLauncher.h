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
    // 로그인정보를 조회하기위한 DB
    C2DB* my_LoginDB = nullptr;
    QTextCodec* codec;	// 한글 사용을 위한 코덱
    QShortcut* Enter_Key; // 엔터키를 통한 로그인

public slots:
    void ButtonClicked();
    void EnterEvent();

private:
    QString ConvertKR(QByteArray _Text);
};
