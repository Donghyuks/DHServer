#include "GameLauncher.h"
#include <QTextCodec>

GameLauncher::GameLauncher(QWidget *parent)
    : QMainWindow(parent)
{
    // DB생성 및 로그인
    my_LoginDB = new C2DB();
    my_LoginDB->ConnectDB("192.168.0.56", "CDH", "ehxk2Rnfwoa!", "LOGIN", 3306);
    codec = QTextCodec::codecForName("EUC-KR");	// 한글코덱
    ui.setupUi(this);

    connect(ui.pushButton, SIGNAL(clicked()), this, SLOT(ButtonClicked()));

    // 엔터키를 통한 로그인가능.
    Enter_Key = new QShortcut(this);
    Enter_Key->setKey(Qt::Key_Return);
    connect(Enter_Key, SIGNAL(activated()), this, SLOT(EnterEvent()));
}

void GameLauncher::ButtonClicked()
{
    /// 입력받은 아이디와 패스워드 가져오기.
    QString _ID = ui.lineEdit->text();
    QString _Password = ui.lineEdit_2->text();
	std::string _ID_String = _ID.toLocal8Bit().constData();
    std::string _Password_String = _Password.toLocal8Bit().constData();

    if (my_LoginDB->SearchID(_ID_String))
    {
        if (my_LoginDB->ComparePassword(_ID_String, _Password_String))
        {
            this->hide();
			GamePlayLauncher playLauncher;
			playLauncher.exec();
            return;
        }
        else
        {
            ui.label_4->setText(ConvertKR("잘못된 패스워드입니다."));
        }
    }
    else
    {
		ui.label_4->setText(ConvertKR("등록되지 않은 아이디입니다."));
    }
}

void GameLauncher::EnterEvent()
{
    ButtonClicked();
}

QString GameLauncher::ConvertKR(QByteArray _Text)
{
    return codec->toUnicode(_Text);
}
