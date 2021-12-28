#include "LoginLauncher.h"
#include "DHNetWorkAPI.h"
#include "PortIPDefine.h"
#include "LoginLauncher_generated.h"
#include "LauncherLoginPacketDefine.h"

LoginLauncher::LoginLauncher(QWidget *parent)
    : QMainWindow(parent)
{
    // �α��� ������ ����
    m_Login_NetWork = new DHNetWorkAPI();
    m_Login_NetWork->Initialize(DHNetWork_Name::DHNet);
    while (!m_Login_NetWork->Connect(LOGIN_SERVER_PORT, SERVER_CONNECT_IP)) { Sleep(0); }

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
    m_PlayLauncher = new GamePlayLauncher(m_Login_NetWork);
    // ȸ������
    m_MakeAccount = new AccountCreate(m_Login_NetWork);
}

void LoginLauncher::ButtonClicked()
{
    // �Է¹��� ���̵�� �н����� ��������.
    _ID = ui.lineEdit->text();
    _Password = ui.lineEdit_2->text();
	_ID_String = _ID.toLocal8Bit().constData();
    _Password_String = _Password.toLocal8Bit().constData();

    if (_Password_String.size() > 20 || _ID_String.size() > 20)
    {
		ui.label_4->setText(ConvertKR("���̵� �н������ 20�ڱ��� �����մϴ�."));
        return;
    }

	// �α��� �������� �ش� ���̵� ��й�ȣ�� �α����� ��û�Ѵ�.
	flatbuffers::FlatBufferBuilder m_Builder;
    auto _fID = m_Builder.CreateString(_ID_String);
    auto _fPassword = m_Builder.CreateString(_Password_String);
    auto _Login_Data = Eater::LoginLauncher::CreateLoginReqData(m_Builder, _fID, _fPassword);
    m_Builder.Finish(_Login_Data);

	// ��Ŷ ����� �ٿ� �����ش�.
    m_Send_Packet.Packet_Type = C2S_LOGIN_SAFE_REQ;
    m_Send_Packet.Packet_Size = m_Builder.GetSize();
	memcpy_s(m_Send_Packet.Packet_Buffer, m_Builder.GetSize(), m_Builder.GetBufferPointer(), m_Builder.GetSize());

    // �α��� �õ�
    m_Login_NetWork->Send(&m_Send_Packet);

    // �����κ��� ������û�� ���� ó���� ��ٸ���.
    while (true)
    {
        if (m_Login_NetWork->Recv(Msg_Vec))
        {
            // �����κ��� �α��� �õ�������� ���� �Ǿ�����
			for (auto& S2C_Msg : Msg_Vec)
			{
                m_Recv_Packet = static_cast<S2C_Packet*>(S2C_Msg.Packet);

                // ������ �������� ���� ȭ���� ó���Ѵ�.
				if (m_Recv_Packet->Packet_Type == S2C_LOGIN_SAFE_RES)
				{
					const uint8_t* Recv_Data_Ptr = (unsigned char*)m_Recv_Packet->Packet_Buffer;

					const auto Recv_Login_Result = flatbuffers::GetRoot<Eater::LoginLauncher::LoginResData>(Recv_Data_Ptr);

					auto _Login_Result = Recv_Login_Result->result();

                    if (_Login_Result == LOGIN_SUCCESS)
                    {
                        // ������ �����ĺ���ȣ�� �޾ƿ�.
						_User_Key = Recv_Login_Result->key();
                        // ����� ������ �ʱ�ȭ
						delete S2C_Msg.Packet;
						S2C_Msg.Packet = nullptr;
                        Msg_Vec.clear();

                        // ��óȭ�������� ��ȯ.
						this->hide();
                        m_PlayLauncher->ThreadCreate();
						m_PlayLauncher->exec();
						return;
                    }
                    else if (_Login_Result == LOGIN_ID_FAIL)
                    {
                        ui.label_4->setText(ConvertKR("��ϵ��� ���� ���̵��Դϴ�."));
                    }
                    else if (_Login_Result == LOGIN_PASSWORD_FAIL)
                    {
                        ui.label_4->setText(ConvertKR("�߸��� �н������Դϴ�."));
                    }
					else if (_Login_Result == LOGIN_ALREADY_FAIL)
					{
						ui.label_4->setText(ConvertKR("�̹� �ٸ� Ŭ���̾�Ʈ�� �������Դϴ�."));
					}

                    // ����� ������ �ʱ�ȭ.
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
