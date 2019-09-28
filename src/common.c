#if defined (_WIN32)
#include <windows.h>
#endif

#include "common.h"
#include <zip/windll/structs.h>


static void checkfornil (lua_State *L, const char *key) {
  if (!lua_isnil(L,-1))
    luaL_argerror(L, 1, lua_pushfstring(L, "invalid type of field '%s'", key));
}


void optstring (lua_State *L, int pos, const char* key, const char **trg) {
  lua_getfield(L, pos, key);
  if (lua_isstring(L,-1))
    *trg = lua_tostring(L, -1);
  else
    checkfornil(L, key);
  lua_pop(L,1);
}


void optboolean (lua_State *L, int pos, const char* key, int *trg) {
  lua_getfield(L, pos, key);
  if (lua_type(L,-1) == LUA_TBOOLEAN)
    *trg = lua_toboolean(L, -1);
  else
    checkfornil(L, key);
  lua_pop(L,1);
}


void optinteger (lua_State *L, int pos, const char* key, int *trg) {
  lua_getfield(L, pos, key);
  if (lua_isnumber(L,-1))
    *trg = lua_tointeger(L, -1);
  else
    checkfornil(L, key);
  lua_pop(L,1);
}


void optfunction (lua_State *L, int pos, const char* key) {
  lua_getfield(L, pos, key);
  if (!lua_isfunction(L,-1))
    checkfornil(L, key);
  lua_setfield(L, ALG_ENVIRONINDEX, key);
}


/*
 * Creates an argarray from a Lua table.
 * Replaces '\\' to '/' in all items (unzip32.dll wouldn't find items with '\\')
 * Side effect: pushes a userdata and a table onto Lua stack (but don't remove
 * them manually from there!)
 */
void create_argarray (lua_State *L, int pos, int *argc, char ***argv) {
  int i;
  if (pos < 0)
    pos += lua_gettop(L) + 1;
  luaL_checktype(L, pos, LUA_TTABLE);
  *argc = lua_rawlen(L, pos);
  *argv = (char**) lua_newuserdata(L, (*argc+1) * sizeof(char*));
  lua_createtable(L, *argc, 0);
  for (i=0; i<*argc; i++, lua_pop(L,1)) {
    const char *item;
    lua_pushinteger(L, i+1);
    lua_gettable(L, pos);
    item = lua_tostring(L, -1);
    if (item == NULL)
      luaL_error(L, "arg #d: invalid type on position %d", pos, i+1);
    item = luaL_gsub(L, item, "\\", "/");
    (*argv)[i] = (char*) item;
    lua_rawseti(L, -3, i+1);
  }
  (*argv)[*argc] = NULL;
}


void optional_argarray (lua_State *L, int pos, int *argc, char ***argv) {
  if (lua_istable(L, pos))
    create_argarray(L, pos, argc, argv);
  else if (!lua_isnoneornil(L, pos))
    luaL_typerror(L, pos, "table or nil");
}


void set_string_field (lua_State *L, const char* key, const char* val) {
  lua_pushstring(L, val);
  lua_setfield(L, -2, key);
}


void set_boolean_field (lua_State *L, const char* key, int val) {
  lua_pushboolean(L, val);
  lua_setfield(L, -2, key);
}


void set_integer_field (lua_State *L, const char* key, int val) {
  lua_pushinteger(L, val);
  lua_setfield(L, -2, key);
}


void set_number_field (lua_State *L, const char* key, double val) {
  lua_pushnumber(L, val);
  lua_setfield(L, -2, key);
}


int callback_password (lua_State *L, LPSTR pwbuf, int bufsize, LPCSTR param,
  LPCSTR filename)
{
  int status = IZ_PW_ERROR;
  lua_getfield(L, ALG_ENVIRONINDEX, "password");
  if (!lua_isfunction(L,-1))
    status = IZ_PW_CANCELALL;
  else {
    lua_pushinteger(L, bufsize);
    lua_pushstring(L, param);
    lua_pushstring(L, filename);
    if (0 == lua_pcall(L, 3, 1, 0)) {
      if (lua_isstring(L,-1)) {
        const char *str = lua_tostring(L,-1);
        size_t len = lua_rawlen(L,-1);
        if (len < (size_t)bufsize && len == strlen(str)) {
          strcpy(pwbuf, str);
          status = IZ_PW_ENTERED;
        }
      }
      else if (lua_isnil(L,-1))
        status = IZ_PW_CANCEL;
    }
  }
  lua_pop(L,1);
  return status;
}


int callback_print (lua_State *L, char *buf, unsigned long size)
{
  int ret = (unsigned int) size;
  lua_getfield(L, ALG_ENVIRONINDEX, "print");
  if (lua_isfunction(L, -1)) {
    lua_pushlstring(L, buf, size);
    if (0 == lua_pcall(L, 1, 1, 0)) {
      if (lua_isnumber(L, -1)) {
        ret = lua_tointeger(L, -1);
      }
    }
  }
  lua_pop(L,1);
  return ret;
}

/* Get or set time of file's last modification. Time unit is second.
 * (Windows-specific)
 */
static int FileTime (const char *filename, double *time, int get)
{
  FILETIME t_write;
  LARGE_INTEGER LI;
  HANDLE handle;

  handle = CreateFile(filename, get ? GENERIC_READ:GENERIC_WRITE, 0, NULL,
    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (handle == INVALID_HANDLE_VALUE)
    return 0;

  if (get) {
    if (!GetFileTime(handle, NULL, NULL, &t_write)) {
      CloseHandle(handle);
      return 0;
    }
    memcpy(&LI, &t_write, sizeof(LARGE_INTEGER));
    *time = LI.QuadPart / 10000000;       /* convert 100 ns units to seconds */
    CloseHandle(handle);
    return 1;
  }

  LI.QuadPart = *time * 10000000;         /* convert seconds to 100 ns units */
  memcpy(&t_write, &LI, sizeof(FILETIME));
  if (!SetFileTime(handle, NULL, NULL, &t_write)) {
    CloseHandle(handle);
    return 0;
  }
  CloseHandle(handle);
  return 1;
}

int f_filetime (lua_State *L) {
  const char *filename = luaL_checkstring(L, 1);
  double time;
  if (lua_isnoneornil(L, 2)) {
    if (FileTime(filename, &time, 1))
      lua_pushnumber(L, time);
    else
      lua_pushnil(L);
    return 1;
  }
  time = luaL_checknumber(L, 2);
  lua_pushboolean (L, FileTime(filename, &time, 0));
  return 1;
}

#if LUA_VERSION_NUM > 501
int luaL_typerror (lua_State *L, int narg, const char *tname) {
  const char *msg = lua_pushfstring(L, "%s expected, got %s",
                                    tname, luaL_typename(L, narg));
  return luaL_argerror(L, narg, msg);
}
#endif
