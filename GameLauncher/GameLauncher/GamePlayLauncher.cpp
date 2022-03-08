#include "GamePlayLauncher.h"
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QBoxLayout>
#include <QTextCodec>
#include "DHNetWorkAPI.h"
#include "SharedDataStruct.h"
#include "LauncherLoginPacketDefine.h"
#include <chrono>

GamePlayLauncher::GamePlayLauncher(DHNetWorkAPI* m_NetWork, QDialog* parent)
	: QDialog(parent)
{
	Main_Content_Count = 3;
	m_Login_NetWork = m_NetWork;
	Friend_Request_List.resize(3, "");

	this->setFixedSize(1200,880);

	ui.setupUi(this);
	codec = QTextCodec::codecForName("EUC-KR");	// 한글코덱
	Bold_Rarge = new QFont("century gothic", 16, QFont::Bold);
	Bold_Small = new QFont("century gothic", 12, QFont::Bold);

	// 트리 리스트의 칼럼 간격조정
	ui.Friend_List->setColumnWidth(0, 130);
	ui.Friend_List->setHeaderHidden(true);
	ui.Friend_List->setIconSize(QSize(60, 60));

	// 파트별 설명란 이미지
	Art_Image = new QPixmap("../Resource/LauncherUI/Art_Image.png");
	Design_Image = new QPixmap("../Resource/LauncherUI/GameDesign_Image.png");
	Programming_Image = new QPixmap("../Resource/LauncherUI/Programming_Image.png");

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

	Width = ui.GameDesign_Image->width();
	Height = ui.GameDesign_Image->height();
	ui.GameDesign_Image->setPixmap(Design_Image->scaled(Width, Height, Qt::KeepAspectRatio));

	Width = ui.Programming_Image->width();
	Height = ui.Programming_Image->height();
	ui.Programming_Image->setPixmap(Programming_Image->scaled(Width, Height, Qt::KeepAspectRatio));

	Width = ui.Art_Image->width();
	Height = ui.Art_Image->height();
	ui.Art_Image->setPixmap(Art_Image->scaled(Width, Height, Qt::KeepAspectRatio));

	// Html 파일을 읽어서 보여줌 (패치노트)
	//QFile HtmlFile("PathNote.htm");
	//HtmlFile.open(QFile::ReadOnly | QFile::Text);
	//QTextStream HtmlStream(&HtmlFile);
	//ui.textBrowser->setHtml(HtmlStream.readAll());

	// 트리 구조에 추가.
	addTreeRoot(ConvertKR("게임중"), "", ColColorType::Ingame_Green);
	addTreeRoot(ConvertKR("온라인"), "", ColColorType::Online_Blue);
	addTreeRoot(ConvertKR("오프라인"), "", ColColorType::Offline_Gray);
	addFriendRequest(ConvertKR("친구요청"), ColColorType::Offline_Gray);
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

	auto _Current_Time = std::chrono::system_clock::now();
	auto _Prev_Time = std::chrono::system_clock::now();
	std::chrono::duration<double> _Passed_Time;

	// 서버로 부터 친구 목록을 업데이트 받는다.
	while (true)
	{
		// 게임이 실행중인가?
		if (m_Process != nullptr)
		{
			PROCESS_INFORMATION* _pi = (PROCESS_INFORMATION*)(m_Process);
			HRESULT _Current_State = WaitForSingleObject(_pi->hProcess, 1);

			if (_Current_State == (DWORD)0xFFFFFFFF || _Current_State == S_OK)
			{
				if (m_Is_Playing)
				{
					flatbuffers::FlatBufferBuilder m_Builder;
					auto _My_ID = m_Builder.CreateString(m_User_Name);
					auto _Game_State = Eater::LoginLauncher::CreatePlayState(m_Builder, _My_ID, false);
					m_Builder.Finish(_Game_State);

					C2S_Packet m_Send_Packet;

					m_Send_Packet.Packet_Type = C2S_PLAY_STATE;
					m_Send_Packet.Packet_Size = m_Builder.GetSize();
					memcpy_s(m_Send_Packet.Packet_Buffer, m_Builder.GetSize(), m_Builder.GetBufferPointer(), m_Builder.GetSize());

					m_Login_NetWork->Send(&m_Send_Packet);
				}

				m_Is_Playing = false;
			}
		}

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

				if (Recv_Packet->Packet_Type == S2C_CURRENT_USER_STATE)
				{
					m_Recv_Friend.clear();
					m_Recv_Friend_Request.clear();

					const uint8_t* Recv_Data_Ptr = (unsigned char*)Recv_Packet->Packet_Buffer;

					const auto Recv_Login_Result = flatbuffers::GetRoot<Eater::LoginLauncher::RealTimeData>(Recv_Data_Ptr);

					auto Recv_Friend_State_Vector = Recv_Login_Result->friendstate();
					auto Recv_Friend_Request_Vector = Recv_Login_Result->friendrequest();

					RemoveTreeChildAll();

					for (int i = 0; i < Recv_Friend_State_Vector->size(); i++)
					{
						auto Friend_Data = Recv_Friend_State_Vector->Get(i);
						auto _User_ID = Friend_Data->id()->str();
						auto _User_State = Friend_Data->state();

						m_Recv_Friend.emplace(_User_ID);

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
							addTreeChild(ConvertKR("게임중"), _User_ID, "게임중", USER_ICON_TYPE_1, ColColorType::Ingame_Green);
						}

					}

					if (Recv_Friend_Request_Vector->size() == 0)
					{
						if (!_Friend_Request_Item->isHidden())
						{
							_Friend_Request_Item->setHidden(true);
						}

					}
					else
					{
						if (_Friend_Request_Item->isHidden())
						{
							_Friend_Request_Item->setHidden(false);
						}

						for (int i = 0; i < 3; i++)
						{
							Current_Friend_Request_Count = 0;
							Friend_Request_List[i] = "";
							QTreeWidgetItem* _Child_Item = _Friend_Request_Item->child(i);
							_Child_Item->setHidden(true);
						}

						for (int i = 0; i < Recv_Friend_Request_Vector->size(); i++)
						{
							auto Friend_Request_ID = Recv_Friend_Request_Vector->Get(i)->str();
							m_Recv_Friend_Request.emplace(Friend_Request_ID);

							// 최대 3개까지만 보여줌
							if (i > 4)
							{
								continue;;
							}

							Friend_Request_List[i] = Friend_Request_ID;

							QTreeWidgetItem* _Child_Item = _Friend_Request_Item->child(i);
							_Child_Item->setHidden(false);
							_Child_Item->setText(0, ConvertKR(Friend_Request_ID.c_str()));
							Current_Friend_Request_Count++;
						}

					}
				}

				delete S2C_Msg.Packet;
				S2C_Msg.Packet = nullptr;
			}

			Msg_Vec.clear();
		}

		Sleep(1);
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


void GamePlayLauncher::PushAcceptFriendRequestButton1()
{
	std::string _Current_Friend = Friend_Request_List[0];

	flatbuffers::FlatBufferBuilder m_Builder;
	auto _Friend_ID = m_Builder.CreateString(_Current_Friend);
	auto _My_ID = m_Builder.CreateString(m_User_Name);
	auto _Add_Friend_Data = Eater::LoginLauncher::CreateAcceptFriend(m_Builder, _My_ID, _Friend_ID, true);
	m_Builder.Finish(_Add_Friend_Data);

	C2S_Packet m_Send_Packet;

	m_Send_Packet.Packet_Type = C2S_ACCPET_FRIEND;
	m_Send_Packet.Packet_Size = m_Builder.GetSize();
	memcpy_s(m_Send_Packet.Packet_Buffer, m_Builder.GetSize(), m_Builder.GetBufferPointer(), m_Builder.GetSize());

	m_Login_NetWork->Send(&m_Send_Packet);

	SortFriendRequest(0);
}

void GamePlayLauncher::PushAcceptFriendRequestButton2()
{
	std::string _Current_Friend = Friend_Request_List[1];

	flatbuffers::FlatBufferBuilder m_Builder;
	auto _Friend_ID = m_Builder.CreateString(_Current_Friend);
	auto _My_ID = m_Builder.CreateString(m_User_Name);
	auto _Add_Friend_Data = Eater::LoginLauncher::CreateAcceptFriend(m_Builder, _My_ID, _Friend_ID, true);
	m_Builder.Finish(_Add_Friend_Data);

	C2S_Packet m_Send_Packet;

	m_Send_Packet.Packet_Type = C2S_ACCPET_FRIEND;
	m_Send_Packet.Packet_Size = m_Builder.GetSize();
	memcpy_s(m_Send_Packet.Packet_Buffer, m_Builder.GetSize(), m_Builder.GetBufferPointer(), m_Builder.GetSize());

	m_Login_NetWork->Send(&m_Send_Packet);

	SortFriendRequest(1);
}

void GamePlayLauncher::PushAcceptFriendRequestButton3()
{
	std::string _Current_Friend = Friend_Request_List[2];

	flatbuffers::FlatBufferBuilder m_Builder;
	auto _Friend_ID = m_Builder.CreateString(_Current_Friend);
	auto _My_ID = m_Builder.CreateString(m_User_Name);
	auto _Add_Friend_Data = Eater::LoginLauncher::CreateAcceptFriend(m_Builder, _My_ID, _Friend_ID, true);
	m_Builder.Finish(_Add_Friend_Data);

	C2S_Packet m_Send_Packet;

	m_Send_Packet.Packet_Type = C2S_ACCPET_FRIEND;
	m_Send_Packet.Packet_Size = m_Builder.GetSize();
	memcpy_s(m_Send_Packet.Packet_Buffer, m_Builder.GetSize(), m_Builder.GetBufferPointer(), m_Builder.GetSize());

	m_Login_NetWork->Send(&m_Send_Packet);

	SortFriendRequest(2);
}

void GamePlayLauncher::PushCancelFriendRequestButton1()
{
	std::string _Current_Friend = Friend_Request_List[0];

	flatbuffers::FlatBufferBuilder m_Builder;
	auto _Friend_ID = m_Builder.CreateString(_Current_Friend);
	auto _My_ID = m_Builder.CreateString(m_User_Name);
	auto _Add_Friend_Data = Eater::LoginLauncher::CreateAcceptFriend(m_Builder, _My_ID, _Friend_ID, false);
	m_Builder.Finish(_Add_Friend_Data);

	C2S_Packet m_Send_Packet;

	m_Send_Packet.Packet_Type = C2S_ACCPET_FRIEND;
	m_Send_Packet.Packet_Size = m_Builder.GetSize();
	memcpy_s(m_Send_Packet.Packet_Buffer, m_Builder.GetSize(), m_Builder.GetBufferPointer(), m_Builder.GetSize());

	m_Login_NetWork->Send(&m_Send_Packet);

	SortFriendRequest(0);
}

void GamePlayLauncher::PushCancelFriendRequestButton2()
{
	std::string _Current_Friend = Friend_Request_List[1];

	flatbuffers::FlatBufferBuilder m_Builder;
	auto _Friend_ID = m_Builder.CreateString(_Current_Friend);
	auto _My_ID = m_Builder.CreateString(m_User_Name);
	auto _Add_Friend_Data = Eater::LoginLauncher::CreateAcceptFriend(m_Builder, _My_ID, _Friend_ID, false);
	m_Builder.Finish(_Add_Friend_Data);

	C2S_Packet m_Send_Packet;

	m_Send_Packet.Packet_Type = C2S_ACCPET_FRIEND;
	m_Send_Packet.Packet_Size = m_Builder.GetSize();
	memcpy_s(m_Send_Packet.Packet_Buffer, m_Builder.GetSize(), m_Builder.GetBufferPointer(), m_Builder.GetSize());

	m_Login_NetWork->Send(&m_Send_Packet);

	SortFriendRequest(1);
}

void GamePlayLauncher::PushCancelFriendRequestButton3()
{
	std::string _Current_Friend = Friend_Request_List[2];

	flatbuffers::FlatBufferBuilder m_Builder;
	auto _Friend_ID = m_Builder.CreateString(_Current_Friend);
	auto _My_ID = m_Builder.CreateString(m_User_Name);
	auto _Add_Friend_Data = Eater::LoginLauncher::CreateAcceptFriend(m_Builder, _My_ID, _Friend_ID, false);
	m_Builder.Finish(_Add_Friend_Data);

	C2S_Packet m_Send_Packet;

	m_Send_Packet.Packet_Type = C2S_ACCPET_FRIEND;
	m_Send_Packet.Packet_Size = m_Builder.GetSize();
	memcpy_s(m_Send_Packet.Packet_Buffer, m_Builder.GetSize(), m_Builder.GetBufferPointer(), m_Builder.GetSize());

	m_Login_NetWork->Send(&m_Send_Packet);

	SortFriendRequest(2);
}

void GamePlayLauncher::PushGameStartButton()
{
	static PROCESS_INFORMATION pi;
	static STARTUPINFO si;

	if (m_Process == nullptr)
	{
		m_Process = &pi;
	}

	if (!m_Is_Playing)
	{
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));

		TCHAR _Path[] = _T("bin\\Eater.exe");
		HRESULT _result = CreateProcess(NULL, _Path, NULL, NULL, FALSE,
			NULL, NULL, NULL, &si, &pi);

		flatbuffers::FlatBufferBuilder m_Builder;
		auto _My_ID = m_Builder.CreateString(m_User_Name);
		auto _Game_State = Eater::LoginLauncher::CreatePlayState(m_Builder, _My_ID, true);
		m_Builder.Finish(_Game_State);

		C2S_Packet m_Send_Packet;

		m_Send_Packet.Packet_Type = C2S_PLAY_STATE;
		m_Send_Packet.Packet_Size = m_Builder.GetSize();
		memcpy_s(m_Send_Packet.Packet_Buffer, m_Builder.GetSize(), m_Builder.GetBufferPointer(), m_Builder.GetSize());

		m_Login_NetWork->Send(&m_Send_Packet);

		m_Is_Playing = true;
	}
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
	static bool _Is_In;
	static bool _Is_Request_In;
	_Is_In = false;
	_Is_Request_In = false;

	// 입력받은 아이디를 가져온다.
	QString _ID = ui.Friend_Request->text();
	std::string _ID_String = _ID.toLocal8Bit().constData();

	for (auto _Friend_ID : m_Recv_Friend)
	{
		// 이미 친구상태인 경우
		if (_ID_String == _Friend_ID)
		{
			_Is_In = true;
		}
	}

	if (_Is_In)
	{
		Friend_Request_Result->setText(ConvertKR("이미 친구로 등록되어 있습니다."));
		SetVisibleRequestResult(true);
		return;
	}

	for (auto _Friend_ID : m_Recv_Friend_Request)
	{
		// 이미 친구상태인 경우
		if (_ID_String == _Friend_ID)
		{
			_Is_In = true;
		}
	}
	if (_Is_In)
	{
		Friend_Request_Result->setText(ConvertKR("친구요청을 먼저 처리해주세요."));
		SetVisibleRequestResult(true);
		return;
	}

	if (_ID_String.size() > 20)
	{
		Friend_Request_Result->setText(ConvertKR("ID는 20자까지 가능합니다."));
		SetVisibleRequestResult(true);
		return;
	}

	if (_ID_String == "")
	{
		Friend_Request_Result->setText(ConvertKR("친구 ID를 입력해주세요."));
		SetVisibleRequestResult(true);
		return;
	}

	// 로그인 서버에게 해당 아이디 비밀번호로 친구요청을 보냄
	flatbuffers::FlatBufferBuilder m_Builder;
	auto _Friend_ID = m_Builder.CreateString(_ID_String);
	auto _My_ID = m_Builder.CreateString(m_User_Name);
	auto _Add_Friend_Data = Eater::LoginLauncher::CreateAddFriend(m_Builder, _My_ID, _Friend_ID);
	m_Builder.Finish(_Add_Friend_Data);

	C2S_Packet m_Send_Packet;

	// 패킷 헤더를 붙여 보내준다.
	m_Send_Packet.Packet_Type = C2S_ADD_FRIEND;
	m_Send_Packet.Packet_Size = m_Builder.GetSize();
	memcpy_s(m_Send_Packet.Packet_Buffer, m_Builder.GetSize(), m_Builder.GetBufferPointer(), m_Builder.GetSize());

	// 친구 추가 요청 메세지를 보낸다.
	m_Login_NetWork->Send(&m_Send_Packet);

	Friend_Request_Result->setText(ConvertKR("친구 요청을 보냈습니다."));
	SetVisibleRequestResult(true);
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

void GamePlayLauncher::addFriendRequest(QString _Name, ColColorType _Color_Type)
{
	_Friend_Request_Item = new QTreeWidgetItem(ui.Friend_List);
	_Friend_Request_Item->setText(0, _Name);
	_Friend_Request_Item->setFont(0, *Bold_Rarge);
	_Friend_Request_Item->setHidden(true);

	switch (_Color_Type)
	{
	case GamePlayLauncher::Ingame_Green:
		_Friend_Request_Item->setTextColor(0, QColor(217, 244, 187));
		break;
	case GamePlayLauncher::Online_Blue:
		_Friend_Request_Item->setTextColor(0, QColor(109, 207, 246));
		break;
	case GamePlayLauncher::Offline_Gray:
		_Friend_Request_Item->setTextColor(0, QColor(112, 112, 113));
		break;
	}

	for (int i = 0; i < 3; i++)
	{
		QTreeWidgetItem* ChildTreeItem = new QTreeWidgetItem();
		ChildTreeItem->setText(0, "");
		ChildTreeItem->setFont(0, *Bold_Small);
		QWidget* dualPushButtons = new QWidget();
		QHBoxLayout* hLayout = new QHBoxLayout();
		QPushButton* _Button_1 = new QPushButton("Accept");
		QPushButton* _Button_2 = new QPushButton("Cancel");
		if (i == 0)
		{
			connect(_Button_1, SIGNAL(clicked()), this, SLOT(PushAcceptFriendRequestButton1()));	// 푸쉬버튼 등록
			connect(_Button_2, SIGNAL(clicked()), this, SLOT(PushCancelFriendRequestButton1()));	// 푸쉬버튼 등록
		}
		else if (i == 1)
		{
			connect(_Button_1, SIGNAL(clicked()), this, SLOT(PushAcceptFriendRequestButton2()));	// 푸쉬버튼 등록
			connect(_Button_2, SIGNAL(clicked()), this, SLOT(PushCancelFriendRequestButton2()));	// 푸쉬버튼 등록
		}
		else if ( i == 2)
		{
			connect(_Button_1, SIGNAL(clicked()), this, SLOT(PushAcceptFriendRequestButton3()));	// 푸쉬버튼 등록
			connect(_Button_2, SIGNAL(clicked()), this, SLOT(PushCancelFriendRequestButton3()));	// 푸쉬버튼 등록
		}

		hLayout->addWidget(_Button_1);
		hLayout->addWidget(_Button_2);
		dualPushButtons->setLayout(hLayout);
		_Friend_Request_Item->addChild(ChildTreeItem);
		ChildTreeItem->setHidden(true);

		ui.Friend_List->setItemWidget(ChildTreeItem, 1, dualPushButtons);
		AddFriend_Button.insert({ i * 2, _Button_1 });
		AddFriend_Button.insert({ i * 2 + 1, _Button_2 });
	}
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

void GamePlayLauncher::SortFriendRequest(int _index)
{
	for (int i = _index; i < 2; i++)
	{
		Friend_Request_List[i] = Friend_Request_List[i + 1];
		QTreeWidgetItem* _Child_Item = _Friend_Request_Item->child(i);
		_Child_Item->setText(0, ConvertKR(Friend_Request_List[i].c_str()));
	}
	
	Current_Friend_Request_Count--;
	QTreeWidgetItem* _Child_Item = _Friend_Request_Item->child(Current_Friend_Request_Count);
	_Child_Item->setHidden(true);
}
