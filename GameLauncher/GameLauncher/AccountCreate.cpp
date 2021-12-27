#include "AccountCreate.h"
#include "ui_AccountCreate.h"
#include "DHNetWorkAPI.h"
#include "SharedDataStruct.h"
#include "LoginLauncher_generated.h"
#include "LauncherLoginPacketDefine.h"

AccountCreate::AccountCreate(DHNetWorkAPI* _Network, QWidget *parent)
	: QDialog(parent)
{
	m_Login_NetWork = _Network;
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

	if (_Password_String.size() > 20 || _ID_String.size() > 20)
	{
		ui->Label_Comment->setText(ConvertKR("아이디나 패스워드는 20자까지 가능합니다."));
		return;
	}

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

	// 로그인 서버에게 해당 아이디 비밀번호로 게정생성을 요청한다.
	flatbuffers::FlatBufferBuilder m_Builder;
	auto _fID = m_Builder.CreateString(_ID_String);
	auto _fPassword = m_Builder.CreateString(_Password_String);
	auto _User_Data = Eater::LoginLauncher::CreateCreateUser(m_Builder, _fID, _fPassword);
	m_Builder.Finish(_User_Data);

	// 패킷 헤더를 붙여 보내준다.
	C2S_Packet m_Send_Packet;
	m_Send_Packet.Packet_Type = C2S_CREATE_USER_REQ;
	m_Send_Packet.Packet_Size = m_Builder.GetSize();
	memcpy_s(m_Send_Packet.Packet_Buffer, m_Builder.GetSize(), m_Builder.GetBufferPointer(), m_Builder.GetSize());

	m_Login_NetWork->Send(&m_Send_Packet);

	// 서버에서 응답을 받고 그에대한 처리를 진행한다.
	std::vector<Network_Message> Msg_Vec;
	S2C_Packet* m_Recv_Packet = nullptr;

	while (true)
	{
		if (m_Login_NetWork->Recv(Msg_Vec))
		{
			// 서버로부터 회원가입에 대한 성공여부를 받아온다.
			for (auto& S2C_Msg : Msg_Vec)
			{
				m_Recv_Packet = static_cast<S2C_Packet*>(S2C_Msg.Packet);

				// 서버의 응답결과에 따라 화면을 처리한다.
				if (m_Recv_Packet->Packet_Type == S2C_CREATE_USER_RES)
				{
					bool Packet_Result = m_Recv_Packet->Packet_Buffer[0];

					// 아이디를 성공적으로 생성하였음
					if (Packet_Result == true)
					{		
						// 사용한 데이터 초기화
						ClearText();
						delete S2C_Msg.Packet;
						S2C_Msg.Packet = nullptr;
						Msg_Vec.clear();
						this->hide();
						return;
					}
					// 아이디를 생성하지 못함(아이디가 중복인 경우)
					else
					{
						ui->Label_Comment->setText(ConvertKR("이미 등록된 아이디입니다."));
					}

					delete S2C_Msg.Packet;
					S2C_Msg.Packet = nullptr;
					Msg_Vec.clear();
					return;
				}
			}
		}
		Sleep(0);
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
