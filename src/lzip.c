/*
  started    : 2009-06-02
  written by : Shmuel Zeigerman
*/

#include <zip/windll/structs.h>
#include "common.h"


extern int luaopen_lunzip (lua_State *);


/* Must use a global, since Info-ZIP callbacks don't provide
 * for an object pointer
 * **********************************************************/
static lua_State *GL;


/* Password entry routine - see password.c in the wiz directory for how
   this is actually implemented in Wiz.
 */
static int WINAPI CallbackPassword
    (LPSTR pwbuf, int bufsize, LPCSTR mode, LPCSTR name)
{
  return callback_password(GL, pwbuf, bufsize, mode, name);
}


/* Callback "print" routine */
static int WINAPI CallbackPrint(char *buf, unsigned long size)
{
  return callback_print(GL, buf, size);
}


/* Callback "comment" routine. See comment.c in the wiz directory for how
   this is actually implemented in Wiz.
 */
static int WINAPI CallbackComment (char *szBuf)
{
  szBuf[0] = '\0';
  lua_getfield(GL, ALG_ENVIRONINDEX, "comment");
  if (lua_isfunction(GL, -1)) {
    lua_pushstring(GL, szBuf);
    if (0 == lua_pcall(GL, 1, 1, 0)) {
      if (lua_isstring(GL, -1)) {
        strncpy(szBuf, lua_tostring(GL,-1), 0xFFF0);
        szBuf[0xFFF0] = '\0';
      }
    }
  }
  lua_pop(GL,1);
  return TRUE;
}


static int WINAPI CallbackServiceApplication
    (LPCSTR name, unsigned __int64 size)
{
  int ret = 0;
  lua_getfield(GL, ALG_ENVIRONINDEX, "ServiceApplication");
  if (lua_isfunction(GL, -1)) {
    lua_pushstring(GL, name);
    lua_pushnumber(GL, size);
    if (0 == lua_pcall(GL, 2, 1, 0))
      ret = lua_toboolean(GL,-1);
  }
  lua_pop(GL,1);
  return ret;
}


static ZIPUSERFUNCTIONS ZipUserFunctions = {
  CallbackPrint, CallbackComment, CallbackPassword, NULL,
  CallbackServiceApplication
};


static int f_version (lua_State *L) {
  ZpVer ver;

  GL = L;
  memset(&ver, 0, sizeof(ZpVer));
  ver.structlen = sizeof(ZpVer);
  ZpVersion(&ver);

  lua_createtable(L, 0, 9);
  /* ---------------------------------------------------------------- */
  set_boolean_field(L, "is_beta", ver.flag & 0x01);
  set_boolean_field(L, "uses_zlib", ver.flag & 0x02);
  set_string_field(L, "betalevel", ver.betalevel);
  set_string_field(L, "date", ver.date);
  set_string_field(L, "zlib_version", ver.zlib_version);
  set_boolean_field(L, "fEncryption", ver.fEncryption);
  /* ---------------------------------------------------------------- */
  lua_pushfstring(L, "%d.%d.%d", (int)ver.zip.major, (int)ver.zip.minor,
                                 (int)ver.zip.patchlevel);
  lua_setfield(L, -2, "zip");
  /* ---------------------------------------------------------------- */
  lua_pushfstring(L, "%d.%d.%d", (int)ver.os2dll.major, (int)ver.os2dll.minor,
                                 (int)ver.os2dll.patchlevel);
  lua_setfield(L, -2, "os2dll");
  /* ---------------------------------------------------------------- */
  lua_pushfstring(L, "%d.%d.%d", (int)ver.windll.major, (int)ver.windll.minor,
                                 (int)ver.windll.patchlevel);
  lua_setfield(L, -2, "windll");
  /* ---------------------------------------------------------------- */
  return 1;
}


static int f_init (lua_State *L) {
  if (lua_isnoneornil(L, 1)) {
    lua_createtable(L, 0, 0);
    lua_insert(L, 1);
  }
  luaL_checktype(L, 1, LUA_TTABLE);
  optfunction(L, 1, "print");
  optfunction(L, 1, "comment");
  optfunction(L, 1, "password");
  optfunction(L, 1, "ServiceApplication");
  lua_pushboolean(L, ZpInit(&ZipUserFunctions));
  return 1;
}


static void setoptions (lua_State *L, int pos, ZPOPT *opt) {
  int fLevel;
  luaL_checktype(L, pos, LUA_TTABLE);

  optstring(L, pos, "Date", (const char**)&opt->Date);
  optstring(L, pos, "szRootDir", (const char**)&opt->szRootDir);
  optstring(L, pos, "szTempDir", (const char**)&opt->szTempDir);
  optboolean(L, pos, "fTemp", &opt->fTemp);
  optboolean(L, pos, "fSuffix", &opt->fSuffix);
  optboolean(L, pos, "fEncrypt", &opt->fEncrypt);
  optboolean(L, pos, "fSystem", &opt->fSystem);
  optboolean(L, pos, "fVolume", &opt->fVolume);
  optboolean(L, pos, "fExtra", &opt->fExtra);
  optboolean(L, pos, "fNoDirEntries", &opt->fNoDirEntries);
  optboolean(L, pos, "fExcludeDate", &opt->fExcludeDate);
  optboolean(L, pos, "fIncludeDate", &opt->fIncludeDate);
  optboolean(L, pos, "fVerbose", &opt->fVerbose);
  optboolean(L, pos, "fQuiet", &opt->fQuiet);
  optboolean(L, pos, "fCRLF_LF", &opt->fCRLF_LF);
  optboolean(L, pos, "fLF_CRLF", &opt->fLF_CRLF);
  optboolean(L, pos, "fJunkDir", &opt->fJunkDir);
  optboolean(L, pos, "fGrow", &opt->fGrow);
  optboolean(L, pos, "fForce", &opt->fForce);
  optboolean(L, pos, "fMove", &opt->fMove);
#if 0  /* actions */
  optboolean(L, pos, "fDeleteEntries", &opt->fDeleteEntries);
  optboolean(L, pos, "fUpdate", &opt->fUpdate);
  optboolean(L, pos, "fFreshen", &opt->fFreshen);
#endif
  optboolean(L, pos, "fJunkSFX", &opt->fJunkSFX);
  optboolean(L, pos, "fLatestTime", &opt->fLatestTime);
  optboolean(L, pos, "fComment", &opt->fComment);
  optboolean(L, pos, "fOffsets", &opt->fOffsets);
  optboolean(L, pos, "fPrivilege", &opt->fPrivilege);
  /* optboolean(L, pos, "fEncryption", &opt->fEncryption); */ /* read-only flag */
  optstring (L, pos, "szSplitSize", (const char**)&opt->szSplitSize);
  optinteger(L, pos, "fRecurse", &opt->fRecurse);
  optinteger(L, pos, "fRepair", &opt->fRepair);

  fLevel = -1;
  optinteger(L, pos, "fLevel", &fLevel);
  if (fLevel >= 0 && fLevel <= 9)
    opt->fLevel = fLevel + '0';
}


static int archive (lua_State *L, int action) {
  ZCL zcl;
  ZPOPT opt;
  int result;
  const char *argv[2];

  GL = L;
  lua_settop(L, 3);

  /* 1-st argument */
  memset(&zcl, 0, sizeof(ZCL));
  zcl.lpszZipFN = (char*) luaL_checkstring(L, 1);

  /* 2-nd argument */
  if (lua_isstring(L, 2)) {
    argv[0] = lua_tostring(L, 2);
    argv[1] = 0;
    zcl.argc = 1;
    zcl.FNV = (char**)argv;
  }
  else if (lua_istable(L, 2))
    create_argarray(L, 2, &zcl.argc, &zcl.FNV);
  else
    return luaL_typerror(L, 2, "string or table");

  /* 3-rd argument */
  memset(&opt, 0, sizeof(ZPOPT));
  opt.fLevel = '6';
  if (!lua_isnil(L, 3))
    setoptions(L, 3, &opt);

  opt.fDeleteEntries = opt.fUpdate = opt.fFreshen = 0;
  if (action == 'u') opt.fUpdate = 1;
  else if (action == 'f') opt.fFreshen = 1;
  else if (action == 'd') opt.fDeleteEntries = 1;

  result = ZpArchive(zcl, &opt);

  lua_pushinteger(L, result);
  if (result >= 0 && result <= ZE_MAXERR)
    lua_pushstring(L, ziperrors[result].string);
  else
    lua_pushstring(L, "unknown error code");
  return 2;
}


static int f_add     (lua_State *L)  { return archive(L, 'a'); }
static int f_update  (lua_State *L)  { return archive(L, 'u'); }
static int f_freshen (lua_State *L)  { return archive(L, 'f'); }
static int f_delete  (lua_State *L)  { return archive(L, 'd'); }


static const luaL_Reg lib[] = {
  {"init",         f_init},
  {"version",      f_version},

  {"add",          f_add},
  {"update",       f_update},
  {"freshen",      f_freshen},
  {"delete",       f_delete},

  {"filetime",     f_filetime},

  {NULL,           NULL}
};


int luaopen_lizip (lua_State *L)
{
  ZpInit(&ZipUserFunctions); /* prevents crash */
#if LUA_VERSION_NUM == 501
  lua_newtable(L);
  lua_replace(L, LUA_ENVIRONINDEX); /* used in callbacks */
  luaL_register(L, "zip", lib);
#else
  lua_newtable(L);
  lua_newtable(L);
  luaL_setfuncs(L, lib, 1);
  lua_setglobal(L, "zip");
#endif
  luaopen_lunzip(L);
  return 0;
}
