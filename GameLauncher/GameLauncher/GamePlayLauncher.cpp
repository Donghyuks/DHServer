#include "GamePlayLauncher.h"
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QTextCodec>
#include "DHNetWorkAPI.h"
#include "SharedDataStruct.h"
#include "LoginLauncher_generated.h"
#include "LauncherLoginPacketDefine.h"

GamePlayLauncher::GamePlayLauncher(DHNetWorkAPI* m_NetWork, QDialog* parent)
	: QDialog(parent)
{
	m_Login_NetWork = m_NetWork;

	ui.setupUi(this);
	codec = QTextCodec::codecForName("EUC-KR");	// �ѱ��ڵ�
	Bold_Rarge = new QFont("century gothic", 16, QFont::Bold);
	Bold_Small = new QFont("century gothic", 12, QFont::Bold);

	connect(ui.pushButton_3, SIGNAL(clicked()), this, SLOT(PushGameStartButton()));	// Ǫ����ư ���

	// Ʈ�� ����Ʈ�� Į�� ��������
	ui.treeWidget->setColumnWidth(0, 150);
	ui.treeWidget->setHeaderHidden(true);

	// Html ������ �о ������ (��ġ��Ʈ)
	QFile HtmlFile("PathNote.htm");
	HtmlFile.open(QFile::ReadOnly | QFile::Text);
	QTextStream HtmlStream(&HtmlFile);
	ui.textBrowser->setHtml(HtmlStream.readAll());

	// Ʈ�� ������ �߰�.
	addTreeRoot(ConvertKR("Eater"), "", ColColorType::Ingame_Green);
	addTreeRoot(ConvertKR("�¶��� ģ��"), "", ColColorType::Online_Blue);
	addTreeRoot(ConvertKR("��������"), "", ColColorType::Offline_Gray);
}

GamePlayLauncher::~GamePlayLauncher()
{
}

void GamePlayLauncher::ThreadCreate()
{
	m_Work_Thread = new std::thread(std::bind(&GamePlayLauncher::RecvServerPacket, this));
}

void GamePlayLauncher::RecvServerPacket()
{
	// ó���� �����͸� �޾ƿͼ� Tree ������ ȭ��ǥ�� ���������.. (�ٸ����� �����͸� �ѹ��̶� �߰��ؾ� ������󱸿�..)
	static bool _Is_Data_Init = false;
	
	// Recv ������ �޾ƿ���.
	std::vector<Network_Message> Msg_Vec;
	S2C_Packet* Recv_Packet = nullptr;
	C2S_Packet Send_Pakcet;

	// ���� ���������� �˷��� ��Ŷ
	Send_Pakcet.Packet_Type = C2S_KEEP_ALIVE_CHECK_RES;
	Send_Pakcet.Packet_Buffer[0] = true;
	Send_Pakcet.Packet_Size = 1;

	// ������ ���� ģ�� ����� ������Ʈ �޴´�.
	while (true)
	{
		if (m_Login_NetWork->Recv(Msg_Vec))
		{
			for (auto& S2C_Msg : Msg_Vec)
			{
				Recv_Packet = static_cast<S2C_Packet*>(S2C_Msg.Packet);

				if (Recv_Packet->Packet_Type == S2C_KEEP_ALIVE_CHECK_REQ)
				{
					const uint8_t* Recv_Data_Ptr = (unsigned char*)Recv_Packet->Packet_Buffer;

					const auto Recv_Login_Result = flatbuffers::GetRoot<Eater::LoginLauncher::RealTimeData>(Recv_Data_Ptr);

					auto Recv_Friend_State_Vector = Recv_Login_Result->friendstate();

					for (int i = 0; i < Recv_Friend_State_Vector->size(); i++)
					{
						auto Friend_Data = Recv_Friend_State_Vector->Get(i);
						auto _User_ID = Friend_Data->id()->str();
						auto _User_State = Friend_Data->state();

						if (_Is_Data_Init == false)
						{
							_Is_Data_Init = true;
							addTreeChild(ConvertKR("Eater"), ConvertKR(_User_ID.c_str()), ConvertKR(""), ColColorType::Ingame_Green);
							addTreeChild(ConvertKR("��������"), ConvertKR(_User_ID.c_str()), ConvertKR(""), ColColorType::Offline_Gray);
							addTreeChild(ConvertKR("�¶��� ģ��"), ConvertKR(_User_ID.c_str()), ConvertKR(""), ColColorType::Online_Blue);
						}

						// �� ������ ���¿� ���� ó�����ش�.
						if (_User_State == USER_OFFLINE)
						{
							ExceptRemoveTreeChild(ConvertKR("��������"), ConvertKR(_User_ID.c_str()));
							addTreeChild(ConvertKR("��������"), ConvertKR(_User_ID.c_str()), ConvertKR("��������"), ColColorType::Offline_Gray);
						}
						else if (_User_State == USER_ONLINE)
						{
							ExceptRemoveTreeChild(ConvertKR("�¶��� ģ��"), ConvertKR(_User_ID.c_str()));
							addTreeChild(ConvertKR("�¶��� ģ��"), ConvertKR(_User_ID.c_str()), ConvertKR("�¶���"), ColColorType::Online_Blue);
						}
						else if (_User_State == USER_IN_GAME)
						{
							ExceptRemoveTreeChild(ConvertKR("Eater"), ConvertKR(_User_ID.c_str()));
							addTreeChild(ConvertKR("Eater"), ConvertKR(_User_ID.c_str()), ConvertKR("������"), ColColorType::Ingame_Green);
						}
						else if (_User_State == USER_IN_LOBBY)
						{
							ExceptRemoveTreeChild(ConvertKR("Eater"), ConvertKR(_User_ID.c_str()));
							addTreeChild(ConvertKR("Eater"), ConvertKR(_User_ID.c_str()), ConvertKR("�κ�"), ColColorType::Ingame_Green);
						}

					}

				}

				delete S2C_Msg.Packet;
				S2C_Msg.Packet = nullptr;
			}

			Msg_Vec.clear();
			
			// ���������� �˸�.
			m_Login_NetWork->Send(&Send_Pakcet);
		}
	}
}

void GamePlayLauncher::PushGameStartButton()
{
	// SVN ������Ʈ ���� ( SVN�� ����Ʈ�� ������ ���� ��ο� �򸰴� => C:\\Program Files\\TortoiseSVN\\bin )
	//system("cd C:\\Program Files\\TortoiseSVN\\bin && TortoiseProc.exe /command:update /path:D:\\GA2ndFinal_Eater && D:\\GA2ndFinal_Eater\\1_Executable\\TestClient\\TestClient.exe");
	system("..\\TestClient\\TestClient.exe");
}

QString GamePlayLauncher::ConvertKR(QByteArray _Text)
{
	return codec->toUnicode(_Text);
}

void GamePlayLauncher::addTreeRoot(QString _Name, QString _description, ColColorType _Color_Type)
{
	QTreeWidgetItem* treeItem = new QTreeWidgetItem(ui.treeWidget);
	treeItem->setText(0, _Name);
	treeItem->setText(1, _description);
	treeItem->setFont(0, *Bold_Rarge);

	switch (_Color_Type)
	{
	case GamePlayLauncher::Ingame_Green:
		treeItem->setTextColor(0, QColor(217, 244, 187));
		break;
	case GamePlayLauncher::Online_Blue:
		treeItem->setTextColor(0, QColor(109, 207, 246));
		break;
	case GamePlayLauncher::Offline_Gray:
		treeItem->setTextColor(0, QColor(112, 112, 113));
		break;
	}

	WidgetRoot_List.insert(std::pair<QString, QTreeWidgetItem*>(_Name,treeItem));
}

void GamePlayLauncher::addTreeChild(QString _Parent_Name, QString _Name, QString _description, ColColorType _Color_Type)
{
	QString ColumnString;
	// �̹� �����Ͱ� �����ϴ°�?
	int _Child_Index = -1;

	// ������ �θ�κ��� �ڽĵ� (�࿡ �ش��ϴ� �κ�)�� ������. (�̹� �����ϴ��� Ȯ��)
	for (int i = 0; i < WidgetRoot_List[_Parent_Name]->childCount(); i++)
	{
		ColumnString = WidgetRoot_List[_Parent_Name]->child(i)->text(0);
		// �ش��ϴ� �ڽ� ��ȣ
		if (ColumnString == _Name)
		{
			_Child_Index = i;
			break;
		}
	}

	// �ش� �ڽ��� �������� �ʴ´ٸ�..
	if (_Child_Index == -1)
	{
		// �ش��ϴ� �θ� ������ �����ͼ� Child�� �߰�����.
		QTreeWidgetItem* ParentWidget = WidgetRoot_List[_Parent_Name];
		QTreeWidgetItem* ChildTreeItem = new QTreeWidgetItem();
		ChildTreeItem->setText(0, _Name);
		ChildTreeItem->setText(1, _description);
		ChildTreeItem->setFont(0, *Bold_Small);
		ChildTreeItem->setFont(1, *Bold_Small);

		switch (_Color_Type)
		{
		case GamePlayLauncher::Ingame_Green:
			ChildTreeItem->setTextColor(0, QColor(217, 244, 187));
			ChildTreeItem->setTextColor(1, QColor(217, 244, 187));
			break;
		case GamePlayLauncher::Online_Blue:
			ChildTreeItem->setTextColor(0, QColor(109, 207, 246));
			ChildTreeItem->setTextColor(1, QColor(109, 207, 246));
			break;
		case GamePlayLauncher::Offline_Gray:
			ChildTreeItem->setTextColor(0, QColor(112, 112, 113));
			ChildTreeItem->setTextColor(1, QColor(112, 112, 113));
			break;
		}

		ParentWidget->addChild(ChildTreeItem);
	}
	// �ش� �ڽ��� �����Ѵٸ� ���¸޼����� �ٲ��ش�.
	else
	{
		WidgetRoot_List[_Parent_Name]->child(_Child_Index)->setText(1, _description);
	}

}

void GamePlayLauncher::ExceptRemoveTreeChild(QString _Except_Parent, QString _Item_Name)
{
	// �ش��ϴ� �θ� ������ ���������� �ش� �������� �����.
	for (auto& _Root_Data : WidgetRoot_List)
	{
		QString _Parent_Name = _Root_Data.first;

		if (_Parent_Name == _Except_Parent) continue;

		// ������ �θ�κ��� �ڽĵ� (�࿡ �ش��ϴ� �κ�)�� ������.
		for (int i = 0; i < WidgetRoot_List[_Parent_Name]->childCount(); i++)
		{
			QString ColumnString = WidgetRoot_List[_Parent_Name]->child(i)->text(0);
			// �ش��ϴ� �ڽ� ����
			if (ColumnString == _Item_Name)
			{
				WidgetRoot_List[_Parent_Name]->removeChild(WidgetRoot_List[_Parent_Name]->child(i));
				return;
			}
		}
	}
}
