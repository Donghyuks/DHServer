#pragma once

#include <QDialog>
#include "ui_GamePlayLauncher.h"

class GamePlayLauncher : public QDialog
{
	Q_OBJECT

public:
	GamePlayLauncher(QDialog*parent = Q_NULLPTR);
	~GamePlayLauncher();

private:
	Ui::GamePlayLauncher ui;
};
