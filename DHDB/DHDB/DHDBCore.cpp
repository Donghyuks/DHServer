#include "DHDBCore.h"
#include <stdlib.h>
#include <stdio.h>


DHDBCore::DHDBCore()
{
	m_MYSQL = mysql_init(m_MYSQL);
	if (!m_MYSQL)
	{
		puts("[C2DB ������ ����] MYSQL ������ �����Ͽ����ϴ�.");
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
		// �������� ���н�
		if (mysql_query(m_MYSQL, _Query.c_str()))
		{
			return false;
		}
		// �������� ������
		else
		{
			// ��������
			MYSQL_RES* Query_Result = mysql_store_result(m_MYSQL);

			// �ش絥���Ͱ� �������� �ʴ´ٸ�
			if (!Query_Result)
			{
				mysql_free_result(Query_Result);
				return false;
			}
			else
			{
				// �࿡ ���� �����͸� �޾ƿ�.
				MYSQL_ROW Row_Data;
				// ��ȸ�� �����Ϳ��� Į���� ����.
				*_Fields_Count = mysql_num_fields(Query_Result);

				// �ش� �����͵��� ��� ��ȸ�Ѵ�.
				while (Row_Data = mysql_fetch_row(Query_Result))
				{
					for (int i = 0; i < *_Fields_Count; i++)
					{
						std::string _Data(Row_Data[i]);

						// ��ȸ�� �����͸� �־ ������.
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
		m_MYSQL,							// ����� MYSQL ����ü
		_Server_IP.c_str(),					// ������ ���� IP
		_User_ID.c_str(),					// DB�� �α����� ID
		_User_Password.c_str(),				// DB�� �α����� Password
		_DB_Name.c_str(),					// ������ DB �̸�
		_Port,								// ������ DB Port
		NULL,								// ���� �����̳� ������ �̸�
		CLIENT_FOUND_ROWS					// ���� �ɼ� flag
	))
	/// ���� ���н�
	{
		fprintf(stderr, "[C2DB Connect ����] DB ���� ���ῡ �����Ͽ����ϴ�.\n Error : %s\n", mysql_error(m_MYSQL));
		return false;
	}
	/// ���� ������
	else
	{
		// �ѱ� ���� ���� �ذ�.
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

		// �����˻�
		mysql_query(m_MYSQL, _Query.c_str());
		// ��������
		MYSQL_RES* Query_Result = mysql_store_result(m_MYSQL);

		// �ش絥���Ͱ� �������� �ʴ´ٸ�
		if (!Query_Result)
		{
			return false;
		}
		// �ش� ���̵� �����Ѵٸ�
		else
		{
			// �࿡ ���� �����͸� �޾ƿ�.
			MYSQL_ROW Row_Data;
			// ��ȸ�� �����Ϳ��� Į���� ����.
			unsigned int Fields_Count = mysql_num_fields(Query_Result);

			// �ش� �����͵��� ��� ��ȸ�Ѵ�.
			while (Row_Data = mysql_fetch_row(Query_Result))
			{
				for (int i = 0; i < Fields_Count; i++)
				{
					std::string _Data(Row_Data[i]);
					// �ش���̵� �����ϸ� true �� ��ȯ.
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

		// �����˻�
		mysql_query(m_MYSQL, _Query.c_str());
		// ��������
		MYSQL_RES* Query_Result = mysql_store_result(m_MYSQL);

		// �ش絥���Ͱ� �������� �ʴ´ٸ�
		if (!Query_Result)
		{
			return false;
		}
		// �ش� ���̵� ���� ��й�ȣ�� �޾ƿͼ�
		else
		{
			// �࿡ ���� �����͸� �޾ƿ�.
			MYSQL_ROW Row_Data;
			// ��ȸ�� �����Ϳ��� Į���� ����.
			unsigned int Fields_Count = mysql_num_fields(Query_Result);

			// �ش� �����͵��� ��� ��ȸ�Ѵ�.
			while (Row_Data = mysql_fetch_row(Query_Result))
			{
				for (int i = 0; i < Fields_Count; i++)
				{
					std::string _Data(Row_Data[i]);
					// �ش���̵� ���Ͽ� ��й�ȣ�� ��ġ�ϸ� true ��ȯ.
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
		
		// �������� ���н�
		if (mysql_query(m_MYSQL, _Query.c_str()))
		{
			return false;
		}
		// �������� ������
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
		// �̹� ģ�����(��û)�� �Ǿ��ִ��� �˻��Ѵ�.
		std::string Find_Query = SELECTSTRING + std::string("* FROM FRIEND WHERE USERID='") + _User_ID  + std::string("' AND FRIENDID ='") + _Friend_ID + std::string("'");
		// �����˻�
		mysql_query(m_MYSQL, Find_Query.c_str());
		MYSQL_RES* Query_Result = mysql_store_result(m_MYSQL);

		// �ش絥���Ͱ� �̹� �����Ѵٸ� �������� �ʴ´�.
		if (Query_Result->row_count != 0)
		{
			mysql_free_result(Query_Result);
			return true;
		}

		mysql_free_result(Query_Result);

		// ģ������� �Ǿ����� �ʴٸ� �ش� ģ����û�� ����Ѵ�.
		std::string _Query = INSERTSTRING + std::string("FRIEND (USERID,FRIENDID,ACCEPTFLAG) VALUES ('") + _User_ID + std::string("', '") + _Friend_ID + std::string("', 0)");

		// �������� ���н�
		if (mysql_query(m_MYSQL, _Query.c_str()))
		{
			return false;
		}
		else
		{
			_Query = INSERTSTRING + std::string("FRIEND (USERID,FRIENDID,ACCEPTFLAG) VALUES ('") + _Friend_ID + std::string("', '") + _User_ID + std::string("', 0)");

			if (mysql_query(m_MYSQL, _Query.c_str()))
			{
				// �־��� ������ ����
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
		// �̹� ģ����û�� �Ǿ��ִ��� �˻��Ѵ�.
		std::string Find_Query = SELECTSTRING + std::string("* FROM FRIEND WHERE USERID='") + _User_ID + std::string("' AND FRIENDID ='") + _Friend_ID + std::string("'");
		// �����˻�
		mysql_query(m_MYSQL, Find_Query.c_str());
		MYSQL_RES* Query_Result = mysql_store_result(m_MYSQL);

		// ���� ģ����û�� �������� ���� ��� false ����.
		if (Query_Result->row_count == 0)
		{
			mysql_free_result(Query_Result);
			return false;
		}

		mysql_free_result(Query_Result);

		// ģ����û�� ������ �ִٸ� �ش� ������ ģ������ ���� �ٲ��ش�.
		std::string _Query = UPDATESTRING + std::string("FRIEND SET ACCEPTFLAG=1 WHERE USERID='") + _User_ID + std::string("' AND FRIENDID ='") + _Friend_ID + std::string("'");

		// �������� ���н�
		if (mysql_query(m_MYSQL, _Query.c_str()))
		{
			return false;
		}
		else
		{
			_Query = UPDATESTRING + std::string("FRIEND SET ACCEPTFLAG=1 WHERE USERID='") + _Friend_ID + std::string("' AND FRIENDID ='") + _User_ID + std::string("'");

			if (mysql_query(m_MYSQL, _Query.c_str()))
			{
				// �־��� ������ ����
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

		// �����˻�
		mysql_query(m_MYSQL, _Query.c_str());
		// ��������
		MYSQL_RES* Query_Result = mysql_store_result(m_MYSQL);

		// �ش絥���Ͱ� �������� �ʴ´ٸ�
		if (!Query_Result)
		{
			mysql_free_result(Query_Result);
			return false;
		}
		// �ش� ���̵� ���� ģ������ �޾ƿ´�.
		else
		{
			// �࿡ ���� �����͸� �޾ƿ�.
			MYSQL_ROW Row_Data;
			// ��ȸ�� �����Ϳ��� Į���� ����.
			unsigned int Fields_Count = mysql_num_fields(Query_Result);

			// �ش� �����͵��� ��� ��ȸ�Ѵ�.
			while (Row_Data = mysql_fetch_row(Query_Result))
			{
				// ���� ���� ģ�����谡 �ƴ϶�� ��� ����..
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

		// �����˻�
		mysql_query(m_MYSQL, _Query.c_str());
		// ��������
		MYSQL_RES* Query_Result = mysql_store_result(m_MYSQL);

		// �ش絥���Ͱ� �������� �ʴ´ٸ�
		if (!Query_Result)
		{
			return false;
		}
		// �ش� ���̵� ���� ��й�ȣ�� �޾ƿͼ�
		else
		{
			// �࿡ ���� �����͸� �޾ƿ�.
			MYSQL_ROW Row_Data;
			// ��ȸ�� �����Ϳ��� Į���� ����.
			unsigned int Fields_Count = mysql_num_fields(Query_Result);

			// �ش� �����͵��� ��� ��ȸ�Ѵ�.
			while (Row_Data = mysql_fetch_row(Query_Result))
			{
				for (int i = 0; i < Fields_Count; i++)
				{
					std::string _Data_String(Row_Data[i]);
					unsigned int _Data = std::stoi(_Data_String);
					// �ش���̵� ���� �ĺ��� ��ȯ.
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
	/// �����Ͱ� ���� ���¿��� �������� ������ ��찡 �߻��ϸ�
	/// ������ ���� ������ �ϱ����� �ش� ������ ���� Ȯ�������� ���ľ� �ϴµ� �̰��� �׳� ������ ���� ���¿��� �������� �����ϴ� �� ���� ����� �����..
	/// ���� return ���� false�� true�� �����ʰ�, �����Ͱ� �ִ����� �������� �ѹ� �����Ű�°� �ӵ��� ����������?!
	if (Is_Connect)
	{
		// ������ ���� ���� ����.
		std::string _Query = DELETESTRING + std::string("LOGIN WHERE ID='") + _User_ID + std::string("'");
		mysql_query(m_MYSQL, _Query.c_str());

		_Query = DELETESTRING + std::string("FRIEND WHERE USERID='") + _User_ID + std::string("'");
		mysql_query(m_MYSQL, _Query.c_str());

		_Query = DELETESTRING + std::string("FRIEND WHERE FRIENDID='") + _User_ID + std::string("'");
		mysql_query(m_MYSQL, _Query.c_str());
	}
}
