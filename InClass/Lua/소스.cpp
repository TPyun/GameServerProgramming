#include <iostream>
extern "C" {
#include "include/lua.hpp"
#include "include/lauxlib.h"
#include "include/lualib.h"
}
#pragma comment (lib, "lua54.lib")
using namespace std;

int c_addnum(lua_State* L)
{
	int a = (int)lua_tonumber(L, -2);
	int b = (int)lua_tonumber(L, -1);
	lua_pop(L, 3);
	lua_pushnumber(L, a + b);
	return 1;
}
int main()
{
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);
	luaL_loadfile(L,"dragon.lua");
	int error = lua_pcall(L, 0, 0, 0);
	if (error) {
		cout << "Error:" << lua_tostring(L, -1);
		lua_pop(L, 1);
	}
	
	//루아 전체 실행
	/*lua_getglobal(L, "pos_x");
	lua_getglobal(L, "pos_y");
	int pos_x = (int)lua_tonumber(L, -2);
	int pos_y = (int)lua_tonumber(L, -1);
	lua_pop(L, 2);
	cout << "pos_x:" << pos_x << ", pos_y:" << pos_y << endl;*/

	//루아 함수 실행
	/*lua_getglobal(L, "plustwo");
	lua_pushnumber(L, 100);
	lua_pcall(L, 1, 1, 0);
	int result = (int)lua_tonumber(L, -1);
	cout << "result:" << result << endl;*/

	
	lua_register(L, "c_addnum", c_addnum);
	lua_getglobal(L, "c_addnum");
	lua_pushnumber(L, 100);
	lua_pushnumber(L, 20);
	lua_pcall(L, 2, 1, 0);
	int sum = lua_tonumber(L, -1);
	lua_pop(L, 1);
	cout << "sum:" << sum << endl;


	lua_close(L);
}