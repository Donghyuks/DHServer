#pragma once

#include <QtWidgets/QMainWindow>
#include <QMessageBox>
#include "ui_GameLauncher.h"
#include "GamePlayLauncher.h"

class GameLauncher : public QMainWindow
{
    Q_OBJECT

public:
    GameLauncher(QWidget *parent = Q_NULLPTR);

private:
    Ui::GameLauncherClass ui;

public slots:
    void ButtonClicked();
};
