#include "GamePlayLauncher.h"
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QTextCodec>
#include "DHNetWorkAPI.h"
#include "SharedDataStruct.h"
#include "LoginLauncher_generated.h"
#include "LauncherLoginPacketDefine.h"
#include <chrono>

GamePlayLauncher::GamePlayLauncher(DHNetWorkAPI* m_NetWork, QDialog* parent)
	: QDialog(parent)
{
	Main_Content_Count = 3;
	m_Login_NetWork = m_NetWork;

	ui.setupUi(this);
	codec = QTextCodec::codecForName("EUC-KR");	// 한글코덱
	Bold_Rarge = new QFont("century gothic", 16, QFont::Bold);
	Bold_Small = new QFont("century gothic", 12, QFont::Bold);

	// 트리 리스트의 칼럼 간격조정
	ui.Friend_List->setColumnWidth(0, 130);
	ui.Friend_List->setHeaderHidden(true);
	ui.Friend_List->setIconSize(QSize(60, 60));

	// Background List
	BackGround_List.insert({ std::string("Main_Content1"),		QPixmap("../Resource/LauncherUI/Main_Content1.png") });
	BackGround_List.insert({ std::string("Main_Content2"),		QPixmap("../Resource/LauncherUI/Main_Content2.png") });
	BackGround_List.insert({ std::string("Main_Content3"),		QPixmap("../Resource/LauncherUI/Main_Content3.png") });

	// Icon image List
	Icon_img_List.insert({ std::string("Main_icon"),			QPixmap("../Resource/iconUI/Main_icon1.png") });

	// Icon List
	Icon_List.insert({ std::string("Profile_icon1_off"),		QIcon("../Resource/iconUI/Profile_icon1_off.png") });
	Icon_List.insert({ std::string("Profile_icon1_on"),			QIcon("../Resource/iconUI/Profile_icon1_on.png") });
	Icon_List.insert({ std::string("Profile_icon2_off"),		QIcon("../Resource/iconUI/Profile_icon2_off.png") });
	Icon_List.insert({ std::string("Profile_icon2_on"),			QIcon("../Resource/iconUI/Profile_icon2_on.png") });
	Icon_List.insert({ std::string("Profile_icon3_off"),		QIcon("../Resource/iconUI/Profile_icon3_off.png") });
	Icon_List.insert({ std::string("Profile_icon3_on"),			QIcon("../Resource/iconUI/Profile_icon3_on.png") });
	Icon_List.insert({ std::string("Content_Focus"),			QIcon("../Resource/LauncherUI/Content_Focus.png") });
	Icon_List.insert({ std::string("Content_Non_Focus"),		QIcon("../Resource/LauncherUI/Content_Non_Focus.png") });
	Icon_List.insert({ std::string("Friend_Cancle"),			QIcon("../Resource/LauncherUI/Friend_Cancle.png") });
	Icon_List.insert({ std::string("Friend_OK"),				QIcon("../Resource/LauncherUI/Friend_OK.png") });

	// Icon Type to String Name
	Icon_Type_Name.insert({ USER_ICON_TYPE_1, std::string("Profile_icon1") });
	Icon_Type_Name.insert({ USER_ICON_TYPE_2, std::string("Profile_icon2") });

	connect(ui.GameStart, SIGNAL(clicked()), this, SLOT(PushGameStartButton()));	// 푸쉬버튼 등록
	connect(ui.AddFriend, SIGNAL(clicked()), this, SLOT(PushFriendRequestButton()));	// 푸쉬버튼 등록

	Friend_Request_Result = new QLabel(this);
	Friend_Request_Result->setStyleSheet("QLabel {background:transparent; font-weight:bold; font-size:16px; color:#BEB6A5; qproperty-alignment: AlignRight;}");
	Friend_Request_Result->setGeometry(_Visible_UI_Start_x, _Visible_UI_Start_y, _Visible_UI_Size.width(), _Visible_UI_Size.height());

	for (int i = 0; i < Main_Content_Count; i++)
	{
		QPushButton* _Content_Push_Button = new QPushButton(this);

		if (i == 0)
		{
			_Content_Push_Button->setStyleSheet("QPushButton {background:transparent; qproperty-iconSize:29px;}");
			_Start_Icon_X = Content_x - ((Icon_Interval + Non_Focus_Icon_x) * (Main_Content_Count - 1)) - Focus_Icon_x;
			_Content_Push_Button->setGeometry(_Start_Icon_X, Content_y+3, Focus_Icon_x, Focus_Icon_y);
			_Content_Push_Button->setIcon(Icon_List["Content_Focus"]);
				connect(_Content_Push_Button, SIGNAL(clicked()), this, SLOT(PushContentButton()));	// 푸쉬버튼 등록
		}
		else
		{
			_Content_Push_Button->setStyleSheet("QPushButton {background:transparent; qproperty-iconSize:9px;}");
			int _X = _Start_Icon_X + Focus_Icon_x + Icon_Interval + ((Icon_Interval + Non_Focus_Icon_x) * (i - 1));
			_Content_Push_Button->setGeometry(_X, Content_y, Non_Focus_Icon_x, Non_Focus_Icon_y);
			_Content_Push_Button->setIcon(Icon_List["Content_Non_Focus"]);
			connect(_Content_Push_Button, SIGNAL(clicked()), this, SLOT(PushContentButton()));	// 푸쉬버튼 등록
		}

		Button_List.push_back(_Content_Push_Button);
	}

	int Width = ui.User_Icon->width();
	int Height = ui.User_Icon->height();
	ui.User_Icon->setPixmap(Icon_img_List["Main_icon"].scaled(Width, Height,Qt::KeepAspectRatio));

	Width = ui.Main_Patch_Image->width();
	Height = ui.Main_Patch_Image->height();
	ui.Main_Patch_Image->setPixmap(BackGround_List["Main_Content1"].scaled(Width, Height, Qt::KeepAspectRatio));

	// Html 파일을 읽어서 보여줌 (패치노트)
	//QFile HtmlFile("PathNote.htm");
	//HtmlFile.open(QFile::ReadOnly | QFile::Text);
	//QTextStream HtmlStream(&HtmlFile);
	//ui.textBrowser->setHtml(HtmlStream.readAll());

	// 트리 구조에 추가.
	addTreeRoot(ConvertKR("Eater"), "", ColColorType::Ingame_Green);
	addTreeRoot(ConvertKR("온라인"), "", ColColorType::Online_Blue);
	addTreeRoot(ConvertKR("오프라인"), "", ColColorType::Offline_Gray);
}

GamePlayLauncher::~GamePlayLauncher()
{
}

void GamePlayLauncher::ThreadCreate()
{
	connect(this, SIGNAL(_UI_Hide()), this, SLOT(HideUI()));
	connect(this, SIGNAL(_UI_Visible()), this, SLOT(VisibleUI()));
	m_Work_Thread = new std::thread(std::bind(&GamePlayLauncher::RecvServerPacket, this));
}

void GamePlayLauncher::RecvServerPacket()
{	
	// Recv 데이터 받아오기.
	std::vector<Network_Message> Msg_Vec;
	S2C_Packet* Recv_Packet = nullptr;
	C2S_Packet Send_Pakcet;

	// 현재 동작중임을 알려줄 패킷
	Send_Pakcet.Packet_Type = C2S_KEEP_ALIVE_CHECK_RES;
	Send_Pakcet.Packet_Buffer[0] = true;
	Send_Pakcet.Packet_Size = 1;

	auto _Current_Time = std::chrono::system_clock::now();
	auto _Prev_Time = std::chrono::system_clock::now();
	std::chrono::duration<double> _Passed_Time;

	// 서버로 부터 친구 목록을 업데이트 받는다.
	while (true)
	{
		// 친구요청에 대한 결과 UI가 필요하다면..
		if (_Current_Visible_State)
		{
			_Current_Time = std::chrono::system_clock::now();

			if (_Is_Visible_Start == false)
			{
				_Prev_Time = _Current_Time;
				_Is_Visible_Start.exchange(true);
			}

			// 경과 시간 체크
			_Passed_Time = _Current_Time - _Prev_Time;

			// 일정시간이 지나면 UI를 없앰..
			if (_Passed_Time.count() > (double)Visible_Time)
			{
				_Prev_Time = _Current_Time;
				SetVisibleRequestResult(false);
			}
		}

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

					RemoveTreeChildAll();

					for (int i = 0; i < Recv_Friend_State_Vector->size(); i++)
					{
						auto Friend_Data = Recv_Friend_State_Vector->Get(i);
						auto _User_ID = Friend_Data->id()->str();
						auto _User_State = Friend_Data->state();

						// 각 유저의 상태에 따라 처리해준다.
						if (_User_State == USER_OFFLINE)
						{
							addTreeChild(ConvertKR("오프라인"), _User_ID, "오프라인", USER_ICON_TYPE_1, ColColorType::Offline_Gray);
						}
						else if (_User_State == USER_ONLINE)
						{
							addTreeChild(ConvertKR("온라인"), _User_ID, "온라인", USER_ICON_TYPE_2, ColColorType::Online_Blue);
						}
						else if (_User_State == USER_IN_GAME)
						{
							addTreeChild(ConvertKR("Eater"), _User_ID, "게임중", USER_ICON_TYPE_1, ColColorType::Ingame_Green);
						}
						else if (_User_State == USER_IN_LOBBY)
						{
							addTreeChild(ConvertKR("Eater"), _User_ID, "로비", USER_ICON_TYPE_1, ColColorType::Ingame_Green);
						}

					}

				}

				delete S2C_Msg.Packet;
				S2C_Msg.Packet = nullptr;
			}

			Msg_Vec.clear();
			
			// 동작중임을 알림.
			m_Login_NetWork->Send(&Send_Pakcet);
		}
	}
}

void GamePlayLauncher::SetUser(std::string _User_Name)
{
	m_User_Name = _User_Name;
	ui.User_ID->setStyleSheet("QLabel {color:#6DCFF6; font-size:24px;}");
	ui.User_ID->setText(ConvertKR(m_User_Name.c_str()));
}


void GamePlayLauncher::HideUI()
{
	// 올릴 사이즈 크기
	int _Up_Size = _Visible_UI_Size.height() + Friend_UI_Interval;
	_Friend_List_Size.setHeight(_Friend_List_Size.height() + _Up_Size);
	_Friend_List_Start_y -= _Up_Size;

	Friend_Request_Result->setVisible(false);

	// 크기 재조정.
	ui.Friend_List->setGeometry(_Friend_List_Start_x, _Friend_List_Start_y, _Friend_List_Size.width(), _Friend_List_Size.height());
}

void GamePlayLauncher::VisibleUI()
{
	// 내릴 사이즈 크기
	int _Drop_Down_Size = _Visible_UI_Size.height() + Friend_UI_Interval;
	_Friend_List_Size.setHeight(_Friend_List_Size.height() - _Drop_Down_Size);
	_Friend_List_Start_y += _Drop_Down_Size;

	Friend_Request_Result->setVisible(true);

	// 크기 재조정.
	ui.Friend_List->setGeometry(_Friend_List_Start_x, _Friend_List_Start_y, _Friend_List_Size.width(), _Friend_List_Size.height());
}

void GamePlayLauncher::PushGameStartButton()
{
	// SVN 업데이트 실행 ( SVN은 디폴트로 다음과 같은 경로에 깔린다 => C:\\Program Files\\TortoiseSVN\\bin )
	//system("cd C:\\Program Files\\TortoiseSVN\\bin && TortoiseProc.exe /command:update /path:D:\\GA2ndFinal_Eater && D:\\GA2ndFinal_Eater\\1_Executable\\TestClient\\TestClient.exe");
	system("..\\TestClient\\TestClient.exe");
}

void GamePlayLauncher::PushContentButton()
{
	QPushButton* Clicked_Button = qobject_cast<QPushButton*>(sender());

	for (int i = 0; i < Button_List.size(); i++)
	{
		if (Button_List[i] == Clicked_Button)
		{
			int _Start_Offset = _Start_Icon_X;

			for (int j = 0; j < Button_List.size(); j++)
			{
				if (j == i)
				{
					Button_List[j]->setGeometry(_Start_Offset, Content_y + 3, Focus_Icon_x, Focus_Icon_y);
					Button_List[j]->setIcon(Icon_List["Content_Focus"]);
					Button_List[j]->setIconSize(QSize(Focus_Icon_x, Focus_Icon_y));
					_Start_Offset = _Start_Offset + Icon_Interval + Focus_Icon_x;

					int Width = ui.Main_Patch_Image->width();
					int Height = ui.Main_Patch_Image->height();

					std::string Map_Name;

					if (j == 0)
					{
						Map_Name = "Main_Content1";
					}
					else if (j == 1)
					{
						Map_Name = "Main_Content2";
					}
					else if (j == 2)
					{
						Map_Name = "Main_Content3";
					}

					ui.Main_Patch_Image->setPixmap(BackGround_List[Map_Name].scaled(Width, Height, Qt::KeepAspectRatio));

				}
				else
				{
					Button_List[j]->setGeometry(_Start_Offset, Content_y, Non_Focus_Icon_x, Non_Focus_Icon_y);
					Button_List[j]->setIcon(Icon_List["Content_Non_Focus"]);
					Button_List[j]->setIconSize(QSize(Non_Focus_Icon_x, Non_Focus_Icon_y));
					_Start_Offset = _Start_Offset + Icon_Interval + Non_Focus_Icon_x;
				}
			}
		}
	}
}

void GamePlayLauncher::PushFriendRequestButton()
{
	// 입력받은 아이디를 가져온다.
	QString _ID = ui.Friend_Request->text();
	std::string _ID_String = _ID.toLocal8Bit().constData();


	if (_ID_String.size() > 20)
	{
		Friend_Request_Result->setText(ConvertKR("아이디나 패스워드는 20자까지 가능합니다."));
		SetVisibleRequestResult(true);
		return;
	}

	if (_ID_String == "")
	{
		Friend_Request_Result->setText(ConvertKR("친구 ID를 입력해주세요."));
		SetVisibleRequestResult(true);
		return;
	}

	//SetVisibleRequestResult(true);

}

void GamePlayLauncher::SetVisibleRequestResult(bool _Visible)
{
	// 현재 보여주고 있는 상태에서 또 들어오면 시간연장..
	if (_Visible == true)
	{
		if (_Current_Visible_State == _Visible)
		{
			_Is_Visible_Start.exchange(false);
			return;
		}
	}

	// 친구요청에 대한 응답을 보여줘야 한다면..
	if (_Visible)
	{
		emit _UI_Visible();
	}
	// 안보여줘도 되는 상태라면..
	else
	{
		emit _UI_Hide();
	}

	_Current_Visible_State.exchange(_Visible);
}

QString GamePlayLauncher::ConvertKR(QByteArray _Text)
{
	return codec->toUnicode(_Text);
}

void GamePlayLauncher::addTreeRoot(QString _Name, QString _description, ColColorType _Color_Type)
{
	QTreeWidgetItem* treeItem = new QTreeWidgetItem(ui.Friend_List);
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

void GamePlayLauncher::addTreeChild(QString _Parent_Name, std::string _Name, std::string _description, int _Icon_Type, ColColorType _Color_Type)
{
	std::string Column_Msg = _Name + std::string("\n") + _description;

	// 이미 데이터가 존재하는가?
	int _Child_Index = -1;

	// 위젯의 부모로부터 자식들 (행에 해당하는 부분)을 가져옴. (이미 존재하는지 확인)
	for (int i = 0; i < WidgetRoot_List[_Parent_Name]->childCount(); i++)
	{
		auto ColumnString = WidgetRoot_List[_Parent_Name]->child(i)->text(1);
		// 해당하는 자식 번호
		if (ColumnString == ConvertKR(Column_Msg.c_str()))
		{
			_Child_Index = i;
			break;
		}
	}

	// 해당하는 부모 위젯을 가져와서 Child를 추가해줌.
	QTreeWidgetItem* ParentWidget = WidgetRoot_List[_Parent_Name];

	// 해당 자식이 존재하지 않는다면..
	if (_Child_Index == -1)
	{
		QTreeWidgetItem* ChildTreeItem = new QTreeWidgetItem();

		std::string Icon_Name = Icon_Type_Name[_Icon_Type];

		if (_Parent_Name == ConvertKR("오프라인"))
		{
			Icon_Name += std::string("_off");
		}
		else
		{
			Icon_Name += std::string("_on");
		}

		ChildTreeItem->setIcon(0, Icon_List[Icon_Name]);
		ChildTreeItem->setText(1, ConvertKR(Column_Msg.c_str()));

		ChildTreeItem->setFont(0, *Bold_Small);
		ChildTreeItem->setFont(1, *Bold_Small);

		switch (_Color_Type)
		{
		case GamePlayLauncher::Ingame_Green:
			ChildTreeItem->setTextColor(1, QColor(217, 244, 187));
			break;
		case GamePlayLauncher::Online_Blue:
			ChildTreeItem->setTextColor(1, QColor(109, 207, 246));
			break;
		case GamePlayLauncher::Offline_Gray:
			ChildTreeItem->setTextColor(1, QColor(112, 112, 113));
			break;
		}

		ParentWidget->addChild(ChildTreeItem);
	}
	// 해당 자식이 존재한다면 상태메세지를 바꿔준다.
	else
	{
		std::string Text_Msg = _Name + std::string("\n") + _description;

		WidgetRoot_List[_Parent_Name]->child(_Child_Index)->setText(1, ConvertKR(Text_Msg.c_str()));
	}

	ParentWidget->sortChildren(1,Qt::SortOrder::DescendingOrder);

}

void GamePlayLauncher::ExceptRemoveTreeChild(QString _Except_Parent, QString _Item_Name)
{
	// 해당하는 부모를 제외한 나머지에서 해당 아이템을 지운다.
	for (auto& _Root_Data : WidgetRoot_List)
	{
		QString _Parent_Name = _Root_Data.first;

		if (_Parent_Name == _Except_Parent) continue;

		// 위젯의 부모로부터 자식들 (행에 해당하는 부분)을 가져옴.
		for (int i = 0; i < WidgetRoot_List[_Parent_Name]->childCount(); i++)
		{
			QString ColumnString = WidgetRoot_List[_Parent_Name]->child(i)->text(0);
			// 해당하는 자식 삭제
			if (ColumnString == _Item_Name)
			{
				WidgetRoot_List[_Parent_Name]->removeChild(WidgetRoot_List[_Parent_Name]->child(i));
				return;
			}
		}
	}
}

void GamePlayLauncher::RemoveTreeChildAll()
{
	// 해당하는 부모를 제외한 나머지에서 해당 아이템을 지운다.
	for (auto& _Root_Data : WidgetRoot_List)
	{
		QString _Parent_Name = _Root_Data.first;

		int Child_Count = WidgetRoot_List[_Parent_Name]->childCount();
		// 위젯의 부모로부터 자식들 (행에 해당하는 부분)을 가져옴.
		for (int i = 0; i < Child_Count; i++)
		{
			WidgetRoot_List[_Parent_Name]->removeChild(WidgetRoot_List[_Parent_Name]->child(0));
		}
	}
}
