#include "LoginLauncher.h"
#include "DHNetWorkAPI.h"
#include "PortIPDefine.h"
#include "LoginLauncher_generated.h"
#include "LauncherLoginPacketDefine.h"

LoginLauncher::LoginLauncher(QWidget *parent)
    : QMainWindow(parent)
{
    // 로그인 서버에 연결
    m_Login_NetWork = new DHNetWorkAPI();
    m_Login_NetWork->Initialize(DHNetWork_Name::DHNet);
    while (!m_Login_NetWork->Connect(LOGIN_SERVER_PORT, SERVER_CONNECT_IP)) { Sleep(0); }

    codec = QTextCodec::codecForName("EUC-KR");	// 한글코덱
    ui.setupUi(this);

    // 각각 로그인과 회원가입 버튼 연동.
    connect(ui.pushButton, SIGNAL(clicked()), this, SLOT(ButtonClicked()));
    connect(ui.CreateAccount, SIGNAL(clicked()), this, SLOT(CreateAccountButtonClicked()));
    // 엔터키를 통한 로그인가능.
    Enter_Key = new QShortcut(this);
    Enter_Key->setKey(Qt::Key_Return);
    connect(Enter_Key, SIGNAL(activated()), this, SLOT(EnterEvent()));

    /// 사용할 Dialog들 초기화.
    // 게임 런처
    m_PlayLauncher = new GamePlayLauncher(m_Login_NetWork);
    // 회원가입
    m_MakeAccount = new AccountCreate(m_Login_NetWork);
}

void LoginLauncher::ButtonClicked()
{
    // 입력받은 아이디와 패스워드 가져오기.
    _ID = ui.lineEdit->text();
    _Password = ui.lineEdit_2->text();
	_ID_String = _ID.toLocal8Bit().constData();
    _Password_String = _Password.toLocal8Bit().constData();

    if (_Password_String.size() > 20 || _ID_String.size() > 20)
    {
		ui.label_4->setText(ConvertKR("아이디나 패스워드는 20자까지 가능합니다."));
        return;
    }

	// 로그인 서버에게 해당 아이디 비밀번호로 로그인을 요청한다.
	flatbuffers::FlatBufferBuilder m_Builder;
    auto _fID = m_Builder.CreateString(_ID_String);
    auto _fPassword = m_Builder.CreateString(_Password_String);
    auto _Login_Data = Eater::LoginLauncher::CreateLoginReqData(m_Builder, _fID, _fPassword);
    m_Builder.Finish(_Login_Data);

	// 패킷 헤더를 붙여 보내준다.
    m_Send_Packet.Packet_Type = C2S_LOGIN_SAFE_REQ;
    m_Send_Packet.Packet_Size = m_Builder.GetSize();
	memcpy_s(m_Send_Packet.Packet_Buffer, m_Builder.GetSize(), m_Builder.GetBufferPointer(), m_Builder.GetSize());

    // 로그인 시도
    m_Login_NetWork->Send(&m_Send_Packet);

    // 서버로부터 인증요청에 대한 처리를 기다린다.
    while (true)
    {
        if (m_Login_NetWork->Recv(Msg_Vec))
        {
            // 서버로부터 로그인 시도결과값이 전달 되었을때
			for (auto& S2C_Msg : Msg_Vec)
			{
                m_Recv_Packet = static_cast<S2C_Packet*>(S2C_Msg.Packet);

                // 서버의 응답결과에 따라 화면을 처리한다.
				if (m_Recv_Packet->Packet_Type == S2C_LOGIN_SAFE_RES)
				{
					const uint8_t* Recv_Data_Ptr = (unsigned char*)m_Recv_Packet->Packet_Buffer;

					const auto Recv_Login_Result = flatbuffers::GetRoot<Eater::LoginLauncher::LoginResData>(Recv_Data_Ptr);

					auto _Login_Result = Recv_Login_Result->result();

                    if (_Login_Result == LOGIN_SUCCESS)
                    {
                        // 유저의 고유식별번호를 받아옴.
						_User_Key = Recv_Login_Result->key();
                        // 사용한 데이터 초기화
						delete S2C_Msg.Packet;
						S2C_Msg.Packet = nullptr;
                        Msg_Vec.clear();

                        // 런처화면으로의 전환.
						this->hide();
                        m_PlayLauncher->ThreadCreate();
						m_PlayLauncher->exec();
						return;
                    }
                    else if (_Login_Result == LOGIN_ID_FAIL)
                    {
                        ui.label_4->setText(ConvertKR("등록되지 않은 아이디입니다."));
                    }
                    else if (_Login_Result == LOGIN_PASSWORD_FAIL)
                    {
                        ui.label_4->setText(ConvertKR("잘못된 패스워드입니다."));
                    }
					else if (_Login_Result == LOGIN_ALREADY_FAIL)
					{
						ui.label_4->setText(ConvertKR("이미 다른 클라이언트가 실행중입니다."));
					}

                    // 사용한 데이터 초기화.
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
