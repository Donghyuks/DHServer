#include "C2DB.h"
#include <stdlib.h>
#include <stdio.h>


C2DB::C2DB()
{
	m_MYSQL = mysql_init(m_MYSQL);
	if (!m_MYSQL)
	{
		puts("[C2DB ������ ����] MYSQL ������ �����Ͽ����ϴ�.");
	}

	mysql_options(m_MYSQL, MYSQL_READ_DEFAULT_FILE, (void*)"./my.cnf");
}

C2DB::~C2DB()
{
	mysql_close(m_MYSQL);
}

bool C2DB::ConnectDB(std::string _Server_IP, std::string _User_ID, std::string _User_Password, std::string _DB_Name, unsigned int _Port)
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

bool C2DB::SearchID(std::string _User_ID)
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

bool C2DB::ComparePassword(std::string _User_ID, std::string _User_Password)
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

bool C2DB::CreateNewAccount(std::string _User_ID, std::string _User_Password)
{
	if (Is_Connect)
	{
		std::string _Query = INSERTSTRING + std::string("LOGIN VALUES ('") + _User_ID + std::string("', '") + _User_Password + std::string("')");
		
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

void C2DB::DeleteAccount(std::string _User_ID)
{
	/// �����Ͱ� ���� ���¿��� �������� ������ ��찡 �߻��ϸ�
	/// ������ ���� ������ �ϱ����� �ش� ������ ���� Ȯ�������� ���ľ� �ϴµ� �̰��� �׳� ������ ���� ���¿��� �������� �����ϴ� �� ���� ����� �����..
	/// ���� return ���� false�� true�� �����ʰ�, �����Ͱ� �ִ����� �������� �ѹ� �����Ű�°� �ӵ��� ����������?!
	if (Is_Connect)
	{
		// ������ ���� ���� ����.
		std::string _Query = DELETESTRING + std::string("LOGIN WHERE ID='") + _User_ID + std::string("'");
		mysql_query(m_MYSQL, _Query.c_str());
	}
}