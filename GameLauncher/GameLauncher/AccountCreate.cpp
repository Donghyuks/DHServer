#include "AccountCreate.h"
#include "ui_AccountCreate.h"
#include "C2DB.h"

AccountCreate::AccountCreate(C2DB* _C2DB, QWidget *parent)
	: QDialog(parent)
{
	my_LoginDB = _C2DB;
	ui = new Ui::AccountCreate();
	codec = QTextCodec::codecForName("EUC-KR");	// 한글코덱
	ui->setupUi(this);

	connect(ui->Create_Button, SIGNAL(clicked()), this, SLOT(ButtonClicked()));

}

AccountCreate::~AccountCreate()
{
	delete ui;
}

void AccountCreate::ButtonClicked()
{
	/// 입력받은 아이디와 패스워드 가져오기.
	QString _ID = ui->Edit_Name->text();
	QString _Password = ui->Edit_Password->text();
	std::string _ID_String = _ID.toLocal8Bit().constData();
	std::string _Password_String = _Password.toLocal8Bit().constData();

	if (_ID_String == "")
	{
		ui->Label_Comment->setText(ConvertKR("ID를 입력해주세요."));
		return;
	}

	if (_Password_String == "")
	{
		ui->Label_Comment->setText(ConvertKR("비밀번호를 입력해주세요."));
		return;
	}

	if (my_LoginDB->SearchID(_ID_String))
	{
		ui->Label_Comment->setText(ConvertKR("이미 등록된 아이디입니다."));
	}
	else
	{

		// 회원가입을 시킴.
		my_LoginDB->CreateNewAccount(_ID_String, _Password_String);
		ClearText();
		this->hide();
		return;
	}
}

QString AccountCreate::ConvertKR(QByteArray _Text)
{
	return codec->toUnicode(_Text);
}

void AccountCreate::ClearText()
{
	ui->Edit_Name->setText(ConvertKR(""));
	ui->Edit_Password->setText(ConvertKR(""));
	ui->Label_Comment->setText(ConvertKR(""));
}
