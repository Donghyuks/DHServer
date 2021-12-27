#include "DHDBCore.h"
#include <stdlib.h>
#include <stdio.h>


DHDBCore::DHDBCore()
{
	m_MYSQL = mysql_init(m_MYSQL);
	if (!m_MYSQL)
	{
		puts("[C2DB 생성자 오류] MYSQL 생성에 실패하였습니다.");
	}

	mysql_options(m_MYSQL, MYSQL_READ_DEFAULT_FILE, (void*)"./my.cnf");
}

DHDBCore::~DHDBCore()
{
	mysql_close(m_MYSQL);
}

bool DHDBCore::QueryDB(std::string _Query, int* _Fields_Count, std::vector<std::string>& _Query_Data)
{
	if (Is_Connect)
	{
		// 쿼리실행 실패시
		if (mysql_query(m_MYSQL, _Query.c_str()))
		{
			return false;
		}
		// 쿼리실행 성공시
		else
		{
			// 쿼리저장
			MYSQL_RES* Query_Result = mysql_store_result(m_MYSQL);

			// 해당데이터가 존재하지 않는다면
			if (!Query_Result)
			{
				mysql_free_result(Query_Result);
				return false;
			}
			else
			{
				// 행에 대한 데이터를 받아옴.
				MYSQL_ROW Row_Data;
				// 조회된 데이터에서 칼럼의 개수.
				*_Fields_Count = mysql_num_fields(Query_Result);

				// 해당 데이터들을 모두 순회한다.
				while (Row_Data = mysql_fetch_row(Query_Result))
				{
					for (int i = 0; i < *_Fields_Count; i++)
					{
						std::string _Data(Row_Data[i]);

						// 순회한 데이터를 넣어서 보내줌.
						_Query_Data.push_back(_Data);
					}
				}

				mysql_free_result(Query_Result);
				return true;
			}
		}
	}
}

bool DHDBCore::ConnectDB(std::string _Server_IP, std::string _User_ID, std::string _User_Password, std::string _DB_Name, unsigned int _Port)
{
	if (!mysql_real_connect(
		m_MYSQL,							// 사용할 MYSQL 구조체
		_Server_IP.c_str(),					// 접속할 서버 IP
		_User_ID.c_str(),					// DB에 로그인할 ID
		_User_Password.c_str(),				// DB에 로그인할 Password
		_DB_Name.c_str(),					// 연결할 DB 이름
		_Port,								// 연결할 DB Port
		NULL,								// 소켓 파일이나 파이프 이름
		CLIENT_FOUND_ROWS					// 연결 옵션 flag
	))
	/// 접속 실패시
	{
		fprintf(stderr, "[C2DB Connect 오류] DB 서버 연결에 실패하였습니다.\n Error : %s\n", mysql_error(m_MYSQL));
		return false;
	}
	/// 접속 성공시
	else
	{
		// 한글 깨짐 문제 해결.
		mysql_query(m_MYSQL, "set names euckr");
		Is_Connect = true;
		return true;
	}
}

bool DHDBCore::SearchID(std::string _User_ID)
{
	if (Is_Connect)
	{
		std::string _Query = SELECTSTRING + std::string("ID FROM LOGIN");

		// 쿼리검색
		mysql_query(m_MYSQL, _Query.c_str());
		// 쿼리저장
		MYSQL_RES* Query_Result = mysql_store_result(m_MYSQL);

		// 해당데이터가 존재하지 않는다면
		if (!Query_Result)
		{
			return false;
		}
		// 해당 아이디가 존재한다면
		else
		{
			// 행에 대한 데이터를 받아옴.
			MYSQL_ROW Row_Data;
			// 조회된 데이터에서 칼럼의 개수.
			unsigned int Fields_Count = mysql_num_fields(Query_Result);

			// 해당 데이터들을 모두 순회한다.
			while (Row_Data = mysql_fetch_row(Query_Result))
			{
				for (int i = 0; i < Fields_Count; i++)
				{
					std::string _Data(Row_Data[i]);
					// 해당아이디가 존재하면 true 를 반환.
					if (_User_ID == _Data)
					{
						mysql_free_result(Query_Result);
						return true;
					}
				}
			}

			mysql_free_result(Query_Result);
			return false;
		}
		
	}
}

bool DHDBCore::ComparePassword(std::string _User_ID, std::string _User_Password)
{
	if (Is_Connect)
	{
		std::string _Query = SELECTSTRING + std::string("PASSWORD FROM LOGIN WHERE ID='") + _User_ID + std::string("'");

		// 쿼리검색
		mysql_query(m_MYSQL, _Query.c_str());
		// 쿼리저장
		MYSQL_RES* Query_Result = mysql_store_result(m_MYSQL);

		// 해당데이터가 존재하지 않는다면
		if (!Query_Result)
		{
			return false;
		}
		// 해당 아이디에 대한 비밀번호를 받아와서
		else
		{
			// 행에 대한 데이터를 받아옴.
			MYSQL_ROW Row_Data;
			// 조회된 데이터에서 칼럼의 개수.
			unsigned int Fields_Count = mysql_num_fields(Query_Result);

			// 해당 데이터들을 모두 순회한다.
			while (Row_Data = mysql_fetch_row(Query_Result))
			{
				for (int i = 0; i < Fields_Count; i++)
				{
					std::string _Data(Row_Data[i]);
					// 해당아이디에 대하여 비밀번호가 일치하면 true 반환.
					if (_User_Password == _Data)
					{
						mysql_free_result(Query_Result);
						return true;
					}
				}
			}

			mysql_free_result(Query_Result);
			return false;
		}

	}
}

bool DHDBCore::CreateNewAccount(std::string _User_ID, std::string _User_Password)
{
	if (Is_Connect)
	{
		std::string _Query = INSERTSTRING + std::string("LOGIN (ID,PASSWORD) VALUES ('") + _User_ID + std::string("', '") + _User_Password + std::string("')");
		
		// 쿼리실행 실패시
		if (mysql_query(m_MYSQL, _Query.c_str()))
		{
			return false;
		}
		// 쿼리실행 성공시
		else
		{
			return true;
		}
	}
}

bool DHDBCore::FriendRequest(std::string _User_ID, std::string _Friend_ID)
{
	if (Is_Connect)
	{
		// 이미 친구등록(요청)이 되어있는지 검색한다.
		std::string Find_Query = SELECTSTRING + std::string("* FROM FRIEND WHERE USERID='") + _User_ID  + std::string("' AND FRIENDID ='") + _Friend_ID + std::string("'");
		// 쿼리검색
		mysql_query(m_MYSQL, Find_Query.c_str());
		MYSQL_RES* Query_Result = mysql_store_result(m_MYSQL);

		// 해당데이터가 이미 존재한다면 생성하지 않는다.
		if (Query_Result->row_count != 0)
		{
			mysql_free_result(Query_Result);
			return true;
		}

		mysql_free_result(Query_Result);

		// 친구등록이 되어있지 않다면 해당 친구요청을 등록한다.
		std::string _Query = INSERTSTRING + std::string("FRIEND (USERID,FRIENDID,ACCEPTFLAG) VALUES ('") + _User_ID + std::string("', '") + _Friend_ID + std::string("', 0)");

		// 쿼리실행 실패시
		if (mysql_query(m_MYSQL, _Query.c_str()))
		{
			return false;
		}
		else
		{
			_Query = INSERTSTRING + std::string("FRIEND (USERID,FRIENDID,ACCEPTFLAG) VALUES ('") + _Friend_ID + std::string("', '") + _User_ID + std::string("', 0)");

			if (mysql_query(m_MYSQL, _Query.c_str()))
			{
				// 넣었던 데이터 삭제
				_Query = DELETESTRING + std::string("FRIEND WHERE USERID='") + _User_ID + std::string("' AND FRIENDID ='") + _Friend_ID + std::string("'");
				mysql_query(m_MYSQL, _Query.c_str());
				return false;
			}
			else
			{
				return true;
			}
		}
	}
}

bool DHDBCore::FriendAccept(std::string _User_ID, std::string _Friend_ID)
{
	if (Is_Connect)
	{
		// 이미 친구요청이 되어있는지 검색한다.
		std::string Find_Query = SELECTSTRING + std::string("* FROM FRIEND WHERE USERID='") + _User_ID + std::string("' AND FRIENDID ='") + _Friend_ID + std::string("'");
		// 쿼리검색
		mysql_query(m_MYSQL, Find_Query.c_str());
		MYSQL_RES* Query_Result = mysql_store_result(m_MYSQL);

		// 서로 친구요청을 받은적이 없는 경우 false 리턴.
		if (Query_Result->row_count == 0)
		{
			mysql_free_result(Query_Result);
			return false;
		}

		mysql_free_result(Query_Result);

		// 친구요청이 온적이 있다면 해당 관계의 친구들의 값을 바꿔준다.
		std::string _Query = UPDATESTRING + std::string("FRIEND SET ACCEPTFLAG=1 WHERE USERID='") + _User_ID + std::string("' AND FRIENDID ='") + _Friend_ID + std::string("'");

		// 쿼리실행 실패시
		if (mysql_query(m_MYSQL, _Query.c_str()))
		{
			return false;
		}
		else
		{
			_Query = UPDATESTRING + std::string("FRIEND SET ACCEPTFLAG=1 WHERE USERID='") + _Friend_ID + std::string("' AND FRIENDID ='") + _User_ID + std::string("'");

			if (mysql_query(m_MYSQL, _Query.c_str()))
			{
				// 넣었던 데이터 삭제
				_Query = DELETESTRING + std::string("FRIEND WHERE USERID='") + _User_ID + std::string("' AND FRIENDID ='") + _Friend_ID + std::string("'");
				mysql_query(m_MYSQL, _Query.c_str());
				return false;
			}
			else
			{
				return true;
			}
		}
	}


	return true;
}

bool DHDBCore::GetFriendList(std::string _User_ID, std::vector<std::string>& _Friend_List)
{
	if (Is_Connect)
	{
		std::string _Query = SELECTSTRING + std::string("ACCEPTFLAG,FRIENDID FROM FRIEND WHERE USERID='") + _User_ID + std::string("'");

		// 쿼리검색
		mysql_query(m_MYSQL, _Query.c_str());
		// 쿼리저장
		MYSQL_RES* Query_Result = mysql_store_result(m_MYSQL);

		// 해당데이터가 존재하지 않는다면
		if (!Query_Result)
		{
			mysql_free_result(Query_Result);
			return false;
		}
		// 해당 아이디에 대한 친구들을 받아온다.
		else
		{
			// 행에 대한 데이터를 받아옴.
			MYSQL_ROW Row_Data;
			// 조회된 데이터에서 칼럼의 개수.
			unsigned int Fields_Count = mysql_num_fields(Query_Result);

			// 해당 데이터들을 모두 순회한다.
			while (Row_Data = mysql_fetch_row(Query_Result))
			{
				// 만약 아직 친구관계가 아니라면 계속 진행..
				if (*Row_Data[0] == false) continue;

				std::string _Data(Row_Data[1]);

				_Friend_List.push_back(_Data);
			}

			mysql_free_result(Query_Result);
			return true;
		}
	}
}

unsigned int DHDBCore::GetIdentifier(std::string _User_ID)
{
	if (Is_Connect)
	{
		std::string _Query = SELECTSTRING + std::string("IDENTIFIER FROM LOGIN WHERE ID='") + _User_ID + std::string("'");

		// 쿼리검색
		mysql_query(m_MYSQL, _Query.c_str());
		// 쿼리저장
		MYSQL_RES* Query_Result = mysql_store_result(m_MYSQL);

		// 해당데이터가 존재하지 않는다면
		if (!Query_Result)
		{
			return false;
		}
		// 해당 아이디에 대한 비밀번호를 받아와서
		else
		{
			// 행에 대한 데이터를 받아옴.
			MYSQL_ROW Row_Data;
			// 조회된 데이터에서 칼럼의 개수.
			unsigned int Fields_Count = mysql_num_fields(Query_Result);

			// 해당 데이터들을 모두 순회한다.
			while (Row_Data = mysql_fetch_row(Query_Result))
			{
				for (int i = 0; i < Fields_Count; i++)
				{
					std::string _Data_String(Row_Data[i]);
					unsigned int _Data = std::stoi(_Data_String);
					// 해당아이디에 대한 식별자 반환.
					mysql_free_result(Query_Result);
					return _Data;
				}
			}

			mysql_free_result(Query_Result);
			return false;
		}

	}
}

void DHDBCore::DeleteAccount(std::string _User_ID)
{
	/// 데이터가 없는 상태에서 쿼리문을 실행할 경우가 발생하면
	/// 데이터 삭제 쿼리를 하기전에 해당 쿼리에 대한 확인절차를 거쳐야 하는데 이것이 그냥 데이터 없는 상태에서 쿼리문을 실행하는 것 보다 비용이 더든다..
	/// 따라서 return 값을 false나 true로 하지않고, 데이터가 있던없던 쿼리문을 한번 실행시키는게 속도가 더빠르겠지?!
	if (Is_Connect)
	{
		// 데이터 삭제 쿼리 실행.
		std::string _Query = DELETESTRING + std::string("LOGIN WHERE ID='") + _User_ID + std::string("'");
		mysql_query(m_MYSQL, _Query.c_str());

		_Query = DELETESTRING + std::string("FRIEND WHERE USERID='") + _User_ID + std::string("'");
		mysql_query(m_MYSQL, _Query.c_str());

		_Query = DELETESTRING + std::string("FRIEND WHERE FRIENDID='") + _User_ID + std::string("'");
		mysql_query(m_MYSQL, _Query.c_str());
	}
}
