#include<iostream>
#include<Windows.h>
#include <sqlext.h>  
#include <format>
#include <string>
using namespace std;

#define NAME_LEN 20

int main() {
    SQLHENV henv;
    SQLHDBC hdbc;
    SQLHSTMT hstmt;
    SQLRETURN retcode;
    SQLWCHAR szName[NAME_LEN];
    SQLINTEGER user_id, user_exp;
	SQLLEN cbName = 0, cbId = 0, cbExp = 0;

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
                    cout << "Connect Success" << endl;
                    retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
                    
					// Execute a SQL statement directly
                    retcode = SQLExecDirect(hstmt, (SQLWCHAR*)L"EXEC over_exp 1", SQL_NTS);
                    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {

                        // Bind columns 1, 2, and 3  
                        retcode = SQLBindCol(hstmt, 1, SQL_INTEGER, &user_id, 12, &cbId);
                        retcode = SQLBindCol(hstmt, 2, SQL_C_CHAR, szName, NAME_LEN, &cbName);
                        retcode = SQLBindCol(hstmt, 3, SQL_INTEGER, &user_exp, 12, &cbExp);

                        // Fetch and print each row of data. On an error, display a message and exit.  
                        for (int i = 1; ; i++) {
                            retcode = SQLFetch(hstmt);
                            if (retcode == SQL_ERROR)
                                cout << "Fetch Error" << endl;
                            if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
                                cout << format("{}: {} {} {}\n", i, user_id, reinterpret_cast<char*>(szName), user_exp);
                            }
                            else
                                break;
                        }
                    }
                    else {
						cout << "SQLExecDirect Error" << endl;
                    }
                    

                    // Process data  
                    if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
                        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
                    }

                    SQLDisconnect(hdbc);
                }
                else
                {
                    cout << "connect false" << endl;
                }
                SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
            }
        }
        SQLFreeHandle(SQL_HANDLE_ENV, henv);
    }
}