#include "LoginLauncher.h"

LoginLauncher::LoginLauncher(QWidget *parent)
    : QMainWindow(parent)
{
    // DB���� �� �α���
    my_LoginDB = new C2DB();
    my_LoginDB->ConnectDB("221.163.91.100", "CDH", "ehxk2Rnfwoa!", "LOGIN", 3306);
    codec = QTextCodec::codecForName("EUC-KR");	// �ѱ��ڵ�
    ui.setupUi(this);

    // ���� �α��ΰ� ȸ������ ��ư ����.
    connect(ui.pushButton, SIGNAL(clicked()), this, SLOT(ButtonClicked()));
    connect(ui.CreateAccount, SIGNAL(clicked()), this, SLOT(CreateAccountButtonClicked()));
    // ����Ű�� ���� �α��ΰ���.
    Enter_Key = new QShortcut(this);
    Enter_Key->setKey(Qt::Key_Return);
    connect(Enter_Key, SIGNAL(activated()), this, SLOT(EnterEvent()));

    /// ����� Dialog�� �ʱ�ȭ.
    // ���� ��ó
    m_PlayLauncher = new GamePlayLauncher;
    // ȸ������
    m_MakeAccount = new AccountCreate(my_LoginDB);
}

void LoginLauncher::ButtonClicked()
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
            m_PlayLauncher->exec();
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

void LoginLauncher::CreateAccountButtonClicked()
{
    ClearText();
    this->hide();
	m_MakeAccount->exec();
	this->setVisible(true);
    return;
}

void LoginLauncher::EnterEvent()
{
    ButtonClicked();
}

QString LoginLauncher::ConvertKR(QByteArray _Text)
{
    return codec->toUnicode(_Text);
}

void LoginLauncher::ClearText()
{
	ui.lineEdit->setText(ConvertKR(""));
	ui.lineEdit_2->setText(ConvertKR(""));
	ui.label_4->setText(ConvertKR(""));
}
