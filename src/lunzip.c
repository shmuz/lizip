/*
  started    : 2009-06-03
  written by : Shmuel Zeigerman
*/

#include <windows.h>
#include <unzip/windll/structs.h>
#include <unzip/windll/decs.h>
#include "common.h"


/* Must use a global, since Info-ZIP callbacks don't provide
 * for an object pointer
 * **********************************************************/
static lua_State *GL;


/* Password entry routine - see password.c in the wiz directory for how
   this is actually implemented in WiZ.
 */
static int WINAPI CallbackPassword (LPSTR pwbuf, int bufsize, LPCSTR prompt,
    LPCSTR filename)
{
  return callback_password(GL, pwbuf, bufsize, prompt, filename);
}


static int WINAPI CallbackPrint (LPSTR buf, unsigned long size)
{
  return callback_print(GL, buf, size);
}


static void WINAPI CallbackSound ()
{
  lua_getfield(GL, ALG_ENVIRONINDEX, "sound");
  if (lua_isfunction(GL, -1)) {
    if (0 == lua_pcall(GL, 0, 0, 0)) {
      /* */
    }
  }
  lua_pop(GL,1);
}


static int WINAPI CallbackReplace (LPSTR efnam, unsigned efbufsiz) {
  /* This is where you will decide if you want to replace, rename etc existing
     files.
   */
  int ret = IDM_REPLACE_NO;
  lua_getfield(GL, ALG_ENVIRONINDEX, "replace");
  if (lua_isfunction(GL, -1)) {
    lua_pushstring(GL, efnam);
    lua_pushinteger(GL, efbufsiz);
    if (0 == lua_pcall(GL, 2, 2, 0)) {
      const char* str = lua_tostring(GL, -2);
      if (str) {
        if (!strcmp(str, "no")) ret = IDM_REPLACE_NO;
        else if (!strcmp(str, "yes")) ret = IDM_REPLACE_YES;
        else if (!strcmp(str, "all")) ret = IDM_REPLACE_ALL;
        else if (!strcmp(str, "none")) ret = IDM_REPLACE_NONE;
        else if (!strcmp(str, "rename") && lua_isstring(GL, -1)) {
          ret = IDM_REPLACE_RENAME;
          strncpy(efnam, lua_tostring(GL, -1), efbufsiz - 1);
          efnam[efbufsiz - 1] = '\0';
        }
      }
      lua_pop(GL,1);
    }
  }
  lua_pop(GL,1);
  return ret;
}


/* Essentially what this function is for is to do a listing of an archive
   contents.
 */
static void WINAPI CallbackSendApplicationMessage (
    z_uint8 ucsize,
    z_uint8 csiz,
    unsigned cfactor,
    unsigned mo, unsigned dy, unsigned yr, unsigned hh, unsigned mm,
    char c, LPCSTR filename, LPCSTR methbuf, unsigned long crc, char fCrypt)
{
  lua_getfield(GL, ALG_ENVIRONINDEX, "SendApplicationMessage");
  if (lua_isfunction(GL, -1)) {
    lua_pushnumber(GL, ucsize);
    lua_pushnumber(GL, csiz);
    lua_pushnumber(GL, cfactor);
    lua_pushnumber(GL, mo);
    lua_pushnumber(GL, dy);
    lua_pushnumber(GL, yr);
    lua_pushnumber(GL, hh);
    lua_pushnumber(GL, mm);
    lua_pushlstring(GL, &c, 1);
    lua_pushstring(GL, filename);
    lua_pushstring(GL, methbuf);
    lua_pushnumber(GL, crc);
    lua_pushlstring(GL, &fCrypt, 1);
    if (0 == lua_pcall(GL, 13, 0, 0)) {
      /* */
    }
  }
  lua_pop(GL,1);
}


static int WINAPI CallbackServCallBk (const char* filename, z_uint8 size)
{
  int ret = 0;
  lua_getfield(GL, ALG_ENVIRONINDEX, "ServCallBk");
  if (lua_isfunction(GL, -1)) {
    lua_pushstring(GL, filename);
    lua_pushnumber(GL, size);
    if (0 == lua_pcall(GL, 2, 1, 0))
      ret = lua_toboolean(GL, -1);
  }
  lua_pop(GL,1);
  return ret;
}


static USERFUNCTIONS get_user_functions () {
  USERFUNCTIONS uf;
  memset(&uf, 0, sizeof(USERFUNCTIONS));
  uf.password = CallbackPassword;
  uf.print = CallbackPrint;
  uf.sound = CallbackSound;
  uf.replace = CallbackReplace;
  uf.SendApplicationMessage = CallbackSendApplicationMessage;
  uf.ServCallBk = CallbackServCallBk;
  return uf;
}


static int f_noprinting (lua_State *L) {
  luaL_checkany(L, 1);
  Wiz_NoPrinting(lua_toboolean(L, 1));
  return 0;
}


static int f_validate (lua_State *L) {
  const char *archive = luaL_checkstring(L, 1);
  lua_pushinteger(L, Wiz_Validate((char*)archive, TRUE));
  return 1;
}


static int f_version (lua_State *L) {
  const UzpVer *ver = UzpVersion();

  lua_createtable(L, 0, 8);
  /* ---------------------------------------------------------------- */
  set_boolean_field(L, "is_beta", ver->flag & 0x01);
  set_boolean_field(L, "uses_zlib", ver->flag & 0x02);
  set_string_field(L, "betalevel", ver->betalevel);
  set_string_field(L, "date", ver->date);
  set_string_field(L, "zlib_version", ver->zlib_version);
  /* ---------------------------------------------------------------- */
  lua_pushfstring(L, "%d.%d.%d", (int)ver->unzip.major, (int)ver->unzip.minor,
                                 (int)ver->unzip.patchlevel);
  lua_setfield(L, -2, "unzip");
  /* ---------------------------------------------------------------- */
  lua_pushfstring(L, "%d.%d.%d", (int)ver->os2dll.major, (int)ver->os2dll.minor,
                                 (int)ver->os2dll.patchlevel);
  lua_setfield(L, -2, "os2dll");
  /* ---------------------------------------------------------------- */
  lua_pushfstring(L, "%d.%d.%d", (int)ver->windll.major, (int)ver->windll.minor,
                                 (int)ver->windll.patchlevel);
  lua_setfield(L, -2, "windll");
  /* ---------------------------------------------------------------- */
  return 1;
}


static void get_options_from_table (lua_State *L, int pos, DCL *dcl)
{
  optboolean(L, pos, "ExtractOnlyNewer", &dcl->ExtractOnlyNewer);
  optboolean(L, pos, "SpaceToUnderscore", &dcl->SpaceToUnderscore);
  optboolean(L, pos, "PromptToOverwrite", &dcl->PromptToOverwrite);

  optinteger(L, pos, "fQuiet", &dcl->fQuiet);

  optboolean(L, pos, "ncflag", &dcl->ncflag);
  optboolean(L, pos, "ntflag", &dcl->ntflag);
  optboolean(L, pos, "nvflag", &dcl->nvflag);
  optboolean(L, pos, "nfflag", &dcl->nfflag);
  optboolean(L, pos, "nzflag", &dcl->nzflag);

  optinteger(L, pos, "ndflag", &dcl->ndflag);

  optboolean(L, pos, "noflag", &dcl->noflag);
  optboolean(L, pos, "naflag", &dcl->naflag);
  optboolean(L, pos, "nZIflag", &dcl->nZIflag);
  optboolean(L, pos, "B_flag", &dcl->B_flag);
  optboolean(L, pos, "C_flag", &dcl->C_flag);

  optinteger(L, pos, "D_flag", &dcl->D_flag);
  optinteger(L, pos, "U_flag", &dcl->U_flag);
  optinteger(L, pos, "fPrivilege", &dcl->fPrivilege);

  /* optstring(L, pos, "lpszZipFN", (const char **) &dcl->lpszZipFN); */
  /* optstring(L, pos, "lpszExtractDir", (const char **) &dcl->lpszExtractDir); */
}


static void get_options_from_string (lua_State *L, int pos, DCL *dcl)
{
  const char *s = lua_tostring(L, pos);

  /* process 'freshen' and 'update' */
  if (strchr(s, 'f'))
    dcl->nfflag = 1;
  if (strchr(s, 'u')) {
    if (dcl->nfflag)
      luaL_argerror(L, pos, "'f' and 'u' options are mutually exclusive");
    dcl->ExtractOnlyNewer = 1;
  }

  /* process 'overwrite' and 'never overwrite' */
  if (strchr(s, 'o'))
    dcl->noflag = 1;
  if (strchr(s, 'n')) {
    if (dcl->noflag)
      luaL_argerror(L, pos, "'n' and 'o' options are mutually exclusive");
  }
  else if (!dcl->noflag)
    dcl->PromptToOverwrite = 1;
}


static int unzip_common (lua_State *L, int action) {
  const char *ZipFN;
  int ifnc = 0, xfnc = 0;
  char **ifnv = 0, **xfnv = 0;
  DCL dcl;
  int pos = 1;
  int result;
  USERFUNCTIONS uf = get_user_functions();

  GL = L;
  lua_settop(L, 5);

  ZipFN = luaL_checkstring(L, pos++);

  memset(&dcl, 0, sizeof(DCL));
  if (action == 'u') {
    const char *ExtractDir = luaL_optstring(L, pos++, NULL);

    if (lua_istable(L, pos)) {
      get_options_from_table(L, pos, &dcl);
      dcl.nvflag = dcl.ntflag = 0;
    }
    else if (lua_isstring(L, pos))
      get_options_from_string(L, pos, &dcl);
    else if (!lua_isnil(L, pos))
      return luaL_typerror(L, pos, "table or nil");

    dcl.lpszExtractDir = (char*) ExtractDir;
    pos++;
  }
  else if (action == 'l')
    dcl.nvflag = 1;
  else if (action == 't')
    dcl.ntflag = 1;

  dcl.StructVersID = UZ_DCL_STRUCTVER;
  dcl.lpszZipFN = (char*) ZipFN;

  optional_argarray(L, pos++, &ifnc, &ifnv);
  optional_argarray(L, pos++, &xfnc, &xfnv);

  result = Wiz_SingleEntryUnzip(ifnc, ifnv, xfnc, xfnv, &dcl, &uf);
  lua_pushinteger(L, result);

  if (result == PK_OK && dcl.nvflag) {
    lua_createtable(L, 0, 5);
    set_number_field (L, "TotalSizeComp", uf.TotalSizeComp);
    set_number_field (L, "TotalSize",     uf.TotalSize);
    set_number_field (L, "CompFactor",    uf.CompFactor);
    set_number_field (L, "NumMembers",    uf.NumMembers);
    set_number_field (L, "cchComment",    uf.cchComment);
    return 2;
  }
  return 1;
}


static int f_unzip (lua_State *L)   { return unzip_common(L, 'u'); }
static int f_list  (lua_State *L)   { return unzip_common(L, 'l'); }
static int f_test  (lua_State *L)   { return unzip_common(L, 't'); }


static int f_unziptomemory (lua_State *L) {
  UzpBuffer buff;
  const char *zip, *file;
  int ret;
  USERFUNCTIONS uf = get_user_functions();

  GL = L;
  zip = luaL_checkstring(L, 1);
  file = luaL_checkstring(L, 2);
  file = luaL_gsub(L, file, "\\", "/");   /* workaround: bug in unzip32.dll */
  ret = Wiz_UnzipToMemory((char*)zip, (char*)file, &uf, &buff);
  if (ret == 1) {
    lua_pushlstring(L, buff.strptr, buff.strlength);
    UzpFreeMemBuffer(&buff);
  }
  else
    lua_pushnil(L);
  return 1;
}


static int f_init (lua_State *L) {
  if (lua_isnoneornil(L, 1)) {
    lua_createtable(L, 0, 0);
    lua_insert(L, 1);
  }
  luaL_checktype(L, 1, LUA_TTABLE);
  optfunction(L, 1, "print");
  optfunction(L, 1, "sound");
  optfunction(L, 1, "replace");
  optfunction(L, 1, "password");
  optfunction(L, 1, "SendApplicationMessage");
  optfunction(L, 1, "ServCallBk");
  return 0;
}


static const luaL_Reg lib[] = {
  {"version",       f_version},
  {"init",          f_init},
  {"unzip",         f_unzip},
  {"list",          f_list},
  {"test",          f_test},
  {"unziptomemory", f_unziptomemory},
  {"validate",      f_validate},
  {"noprinting",    f_noprinting},
  {"filetime",      f_filetime},
  {NULL,           NULL}
};


int luaopen_lunzip (lua_State *L) {
#if LUA_VERSION_NUM == 501
  lua_newtable(L);
  lua_replace(L, LUA_ENVIRONINDEX); /* used in callbacks */
  luaL_register(L, "unzip", lib);
#else
  lua_newtable(L);
  lua_newtable(L);
  luaL_setfuncs(L, lib, 1);
  lua_setglobal(L, "unzip");
#endif
  return 0;
}
