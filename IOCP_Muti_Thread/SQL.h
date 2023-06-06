#pragma once
#include<iostream>
#include<Windows.h>
#include <sqlext.h>  
#include <format>
#include <string>
#define NAME_LEN 20

typedef struct PlayerInfo
{
	//int id = -1;
	char* name;
	int level = -1;
	int exp = -1;
	int hp = -1;
	int max_hp = -1;
	int x = -1;
	int y = -1;
}PI;

class SQL
{
public:
    SQLHENV henv;
    SQLHDBC hdbc;
    SQLHSTMT hstmt;
    SQLRETURN retcode;
    SQLWCHAR szName[NAME_LEN];
    SQLINTEGER user_id, user_level, user_exp, user_hp, user_max_hp, user_pos_x, user_pos_y;
	SQLLEN cbName = 0, cbId = 0, cbExp = 0, cbLevel = 0, cbHp = 0, cbMaxHp = 0, cbPosX = 0, cbPosY = 0;

public:
	SQL();
	~SQL();
    PI get_list();
	PI get_exp_over(int exp);
    
    PI find_by_name(char name[NAME_LEN]);
	void insert_new_account(char name[NAME_LEN], int level, int exp, int hp, int max_hp, int pos_x, int pos_y);
};
