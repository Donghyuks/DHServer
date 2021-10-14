#include "GameLauncher.h"

GameLauncher::GameLauncher(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    connect(ui.pushButton, SIGNAL(clicked()), this, SLOT(ButtonClicked()));
}

void GameLauncher::ButtonClicked()
{
    this->hide();
    GamePlayLauncher playLauncher;
    playLauncher.exec();
    //QMessageBox::information(this, "Title", "Hello");
}
