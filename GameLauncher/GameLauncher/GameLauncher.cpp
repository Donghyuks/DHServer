#include "GameLauncher.h"
#include <QTextCodec>

GameLauncher::GameLauncher(QWidget *parent)
    : QMainWindow(parent)
{
    // DB���� �� �α���
    my_LoginDB = new C2DB();
    my_LoginDB->ConnectDB("192.168.0.56", "CDH", "ehxk2Rnfwoa!", "LOGIN", 3306);
    codec = QTextCodec::codecForName("EUC-KR");	// �ѱ��ڵ�
    ui.setupUi(this);

    connect(ui.pushButton, SIGNAL(clicked()), this, SLOT(ButtonClicked()));

    // ����Ű�� ���� �α��ΰ���.
    Enter_Key = new QShortcut(this);
    Enter_Key->setKey(Qt::Key_Return);
    connect(Enter_Key, SIGNAL(activated()), this, SLOT(EnterEvent()));
}

void GameLauncher::ButtonClicked()
{
    /// �Է¹��� ���̵�� �н����� ��������.
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
            ui.label_4->setText(ConvertKR("�߸��� �н������Դϴ�."));
        }
    }
    else
    {
		ui.label_4->setText(ConvertKR("��ϵ��� ���� ���̵��Դϴ�."));
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
