#include <cstdio>
#include "SQL.h"
using namespace std;

SQL::SQL()
{
    wcout.imbue(locale("korean"));

    // Allocate environment handle  
    retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);   //handle วาด็

    // Set the ODBC version environment attribute  
    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
        retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

        // Allocate connection handle  
        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
            retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

            // Set login timeout to 5 seconds  
            if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
                SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

                // Connect to data source  
                retcode = SQLConnect(hdbc, (SQLWCHAR*)L"GameServer", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);

                // Allocate statement handle  
                if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
                    cout << "SQL Connect Success" << endl;
                }
                else cout << "connect false" << endl;
               
                SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
            }
        }
        SQLFreeHandle(SQL_HANDLE_ENV, henv);
    }
}

SQL::~SQL()
{
    SQLDisconnect(hdbc);

}

PI SQL::get_list(bool just_show = false)
{
    // Bind columns
    retcode = SQLBindCol(hstmt, 1, SQL_C_CHAR, szName, NAME_LEN, &cbName);
    retcode = SQLBindCol(hstmt, 2, SQL_INTEGER, &user_level, 12, &cbLevel);
    retcode = SQLBindCol(hstmt, 3, SQL_INTEGER, &user_exp, 12, &cbExp);
    retcode = SQLBindCol(hstmt, 4, SQL_INTEGER, &user_hp, 12, &cbHp);
    retcode = SQLBindCol(hstmt, 5, SQL_INTEGER, &user_max_hp, 12, &cbMaxHp);
    retcode = SQLBindCol(hstmt, 6, SQL_INTEGER, &user_pos_x, 12, &cbPosX);
    retcode = SQLBindCol(hstmt, 7, SQL_INTEGER, &user_pos_y, 12, &cbPosY);

    // Fetch and print each row of data. On an error, display a message and exit.  
    for (int i = 1; ; i++) {
        retcode = SQLFetch(hstmt);
        if (retcode == SQL_ERROR)
            cout << "Fetch Error" << endl;
        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
            if(just_show)
                cout << format("{:5d}: Name:{} Level:{:4d} Exp:{:4d} Hp:{:4d} MaxHp:{:4d} PosX:{:4d} PosY:{:4d}\n", i, reinterpret_cast<char*>(szName), user_level, user_exp, user_hp, user_max_hp, user_pos_x, user_pos_y);
            else
                return PI{ reinterpret_cast<char*>(szName), user_level, user_exp, user_hp, user_max_hp, user_pos_x, user_pos_y };
        }
        else return PI{};
    }
}

PI SQL::get_exp_over(int exp)
{
    // SQLAllocHandle
    retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {

        wstring sqlStatement = L"EXEC over_exp " + to_wstring(exp);
        retcode = SQLExecDirect(hstmt, reinterpret_cast<SQLWCHAR*>(sqlStatement.data()), SQL_NTS);
        
        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
            //cout << "get exp over success" << endl;
            return get_list();
        }
        else cout << "SQLExecDirect Error get exp over" << endl;
    }
    else cout << "SQL Statement Error" << endl;
    return PI{};
}

PI SQL::find_by_name(char* name)
{
	// SQLAllocHandle
	retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {

        string nameString(name);
        wstring sqlStatement = L"EXEC find_by_name '" + wstring(nameString.begin(), nameString.end()) + L"'";
		retcode = SQLExecDirect(hstmt, reinterpret_cast<SQLWCHAR*>(sqlStatement.data()), SQL_NTS);
        
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
            //cout << "find by name success" << endl;
            return get_list();
		}
		else cout << "SQLExecDirect Error find by name" << endl;
	}
	else cout << "SQL Statement Error" << endl;
    return PI{};
}

void SQL::insert_new_account(char* name, int level, int exp, int hp, int max_hp, int pos_x, int pos_y)
{
	// SQLAllocHandle
	retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {

		string nameString(name);
		wstring sqlStatement = L"EXEC insert_new_account '" + wstring(nameString.begin(), nameString.end()) + L"'" + L"," + to_wstring(level) + L"," + to_wstring(exp) + L"," + to_wstring(hp) + L"," + to_wstring(max_hp) + L"," + to_wstring(pos_x) + L"," + to_wstring(pos_y);
        retcode = SQLExecDirect(hstmt, reinterpret_cast<SQLWCHAR*>(sqlStatement.data()), SQL_NTS);
        
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			//cout << "insert new account success" << endl;
		}
		else cout << "SQLExecDirect Error insert new account" << endl;
	}
	else cout << "SQL Statement Error" << endl;
}

void SQL::save_info(char* name, int level, int exp, int hp, int max_hp, int pos_x, int pos_y)
{
	// SQLAllocHandle
	retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
        
		string nameString(name);
		wstring sqlStatement = L"EXEC save_info '" + wstring(nameString.begin(), nameString.end()) + L"'" + L"," + to_wstring(level) + L"," + to_wstring(exp) + L"," + to_wstring(hp) + L"," + to_wstring(max_hp) + L"," + to_wstring(pos_x) + L"," + to_wstring(pos_y);
        retcode = SQLExecDirect(hstmt, reinterpret_cast<SQLWCHAR*>(sqlStatement.data()), SQL_NTS);

		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			cout << "save info success" << endl;
		}
		else cout << "SQLExecDirect Error save info" << endl;
	}
	else cout << "SQL Statement Error" << endl;
}

void SQL::delete_all()
{
    retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {

		wstring sqlStatement = L"EXEC delete_all";
        retcode = SQLExecDirect(hstmt, reinterpret_cast<SQLWCHAR*>(sqlStatement.data()), SQL_NTS);
        
        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
            //cout << "delete all success" << endl;
        }
        else cout << "SQLExecDirect Error delete all" << endl;
    }
    else cout << "SQL Statement Error" << endl;
}

void SQL::show_all()
{
    retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {

        wstring sqlStatement = L"EXEC show_all";
        retcode = SQLExecDirect(hstmt, reinterpret_cast<SQLWCHAR*>(sqlStatement.data()), SQL_NTS);

        if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
            //cout << "show all success" << endl;
            get_list(true);
        }
        else cout << "SQLExecDirect Error show all" << endl;
    }
    else cout << "SQL Statement Error" << endl;
}
