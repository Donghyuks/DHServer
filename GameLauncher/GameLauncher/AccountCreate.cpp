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
	codec = QTextCodec::codecForName("EUC-KR");	// �ѱ��ڵ�
	ui->setupUi(this);

	connect(ui->Create_Button, SIGNAL(clicked()), this, SLOT(ButtonClicked()));
}

AccountCreate::~AccountCreate()
{
	delete ui;
}

void AccountCreate::ButtonClicked()
{
	/// �Է¹��� ���̵�� �н����� ��������.
	QString _ID = ui->Edit_Name->text();
	QString _Password = ui->Edit_Password->text();
	std::string _ID_String = _ID.toLocal8Bit().constData();
	std::string _Password_String = _Password.toLocal8Bit().constData();

	if (_Password_String.size() > 20 || _ID_String.size() > 20)
	{
		ui->Label_Comment->setText(ConvertKR("���̵� �н������ 20�ڱ��� �����մϴ�."));
		return;
	}

	if (_ID_String == "")
	{
		ui->Label_Comment->setText(ConvertKR("ID�� �Է����ּ���."));
		return;
	}

	if (_Password_String == "")
	{
		ui->Label_Comment->setText(ConvertKR("��й�ȣ�� �Է����ּ���."));
		return;
	}

	// �α��� �������� �ش� ���̵� ��й�ȣ�� ���������� ��û�Ѵ�.
	flatbuffers::FlatBufferBuilder m_Builder;
	auto _fID = m_Builder.CreateString(_ID_String);
	auto _fPassword = m_Builder.CreateString(_Password_String);
	auto _User_Data = Eater::LoginLauncher::CreateCreateUser(m_Builder, _fID, _fPassword);
	m_Builder.Finish(_User_Data);

	// ��Ŷ ����� �ٿ� �����ش�.
	C2S_Packet m_Send_Packet;
	m_Send_Packet.Packet_Type = C2S_CREATE_USER_REQ;
	m_Send_Packet.Packet_Size = m_Builder.GetSize();
	memcpy_s(m_Send_Packet.Packet_Buffer, m_Builder.GetSize(), m_Builder.GetBufferPointer(), m_Builder.GetSize());

	m_Login_NetWork->Send(&m_Send_Packet);

	// �������� ������ �ް� �׿����� ó���� �����Ѵ�.
	std::vector<Network_Message> Msg_Vec;
	S2C_Packet* m_Recv_Packet = nullptr;

	while (true)
	{
		if (m_Login_NetWork->Recv(Msg_Vec))
		{
			// �����κ��� ȸ�����Կ� ���� �������θ� �޾ƿ´�.
			for (auto& S2C_Msg : Msg_Vec)
			{
				m_Recv_Packet = static_cast<S2C_Packet*>(S2C_Msg.Packet);

				// ������ �������� ���� ȭ���� ó���Ѵ�.
				if (m_Recv_Packet->Packet_Type == S2C_CREATE_USER_RES)
				{
					bool Packet_Result = m_Recv_Packet->Packet_Buffer[0];

					// ���̵� ���������� �����Ͽ���
					if (Packet_Result == true)
					{		
						// ����� ������ �ʱ�ȭ
						ClearText();
						delete S2C_Msg.Packet;
						S2C_Msg.Packet = nullptr;
						Msg_Vec.clear();
						this->hide();
						return;
					}
					// ���̵� �������� ����(���̵� �ߺ��� ���)
					else
					{
						ui->Label_Comment->setText(ConvertKR("�̹� ��ϵ� ���̵��Դϴ�."));
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
