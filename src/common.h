#ifndef LIZIP_COMMON_H
#define LIZIP_COMMON_H

#include <lua.h>
#include <lauxlib.h>

void optinteger  (lua_State *L, int pos, const char* key, int *trg);
void optstring   (lua_State *L, int pos, const char* key, const char **trg);
void optboolean  (lua_State *L, int pos, const char* key, int *trg);
void optfunction (lua_State *L, int pos, const char* key);
void create_argarray   (lua_State *L, int pos, int *argc, char ***argv);
void optional_argarray (lua_State *L, int pos, int *argc, char ***argv);
void set_string_field  (lua_State *L, const char* key, const char* val);
void set_boolean_field (lua_State *L, const char* key, int val);
void set_integer_field (lua_State *L, const char* key, int val);
void set_number_field  (lua_State *L, const char* key, double val);
int  callback_password (lua_State *L, char* pwbuf, int bufsize,
                        const char* param, const char* filename);
int  callback_print    (lua_State *L, char *buf, unsigned long size);
int  f_filetime        (lua_State *L);

#endif

