/*
** $Id: ltm.h,v 1.18 2000/10/05 13:00:17 roberto Exp $
** Tag methods
** See Copyright Notice in lua.h
*/

#ifndef ltm_h
#define ltm_h


#include "lobject.h"
#include "lstate.h"

#include "lsaver.h"

/*
* WARNING: if you change the order of this enumeration,
* grep "ORDER TM"
*/
typedef enum {
  TM_GETTABLE = 0,
  TM_SETTABLE,
  TM_INDEX,
  TM_GETGLOBAL,
  TM_SETGLOBAL,
  TM_ADD,
  TM_SUB,
  TM_MUL,
  TM_DIV,
  TM_POW,
  TM_UNM,
  TM_LT,
  TM_CONCAT,
  TM_GC,
  TM_FUNCTION,
  TM_N		/* number of elements in the enum */
} TMS;

struct TMinfo
{
  int method[TM_N];
};

static_assert(TM_N == 15 && sizeof(TMinfo) == 60, "Lua tag methods wire size");
namespace S2FileIO {
template<>
struct StructureFieldCodec<TMinfo, void> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 60;
  static bool Decode(const std::uint8_t* source, std::size_t length,
                     TMinfo* value) {
    if (!value) return false;
    std::int32_t methods[TM_N];
    if (!DecodeLuaStateIntegers(source, length, methods, TM_N)) return false;
    for (int i = 0; i < TM_N; ++i) value->method[i] = methods[i];
    return true;
  }
  static bool Encode(const TMinfo& value, std::uint8_t* destination,
                     std::size_t length) {
    std::int32_t methods[TM_N];
    for (int i = 0; i < TM_N; ++i) methods[i] = value.method[i];
    return EncodeLuaStateIntegers(methods, TM_N, destination, length);
  }
};
} // namespace S2FileIO

struct TM {
	ZDATA
	TMinfo info;
  list<int> collected;  /* list of garbage-collected udata indices with this tag */
	ZEND int operator&( CStructureSaver &f ) { f.Add(2,&info); f.Add(3,&collected); return 0; }
};


int luaT_tag (lua_State *L, const TObject *o);

inline int& luaT_gettm( lua_State *L, int tag, int event ) { return L->TMtable[tag].info.method[event]; }
inline int luaT_gettmindex( lua_State *L, const TObject *o, int e ) { return luaT_gettm( L, luaT_tag( L, o ), e ); }
inline Closure *luaT_gettm( lua_State *L, const TObject *o, int e )  
{ 
	int nCL = luaT_gettmindex( L, o, e );
	if ( nCL == STK_NULL )
		return NULL;
	return L->closures[ nCL ];
}

inline bool IsValidTag( lua_State *L, int nTag ) 
{
	return nTag >= NUM_TAGS && nTag < L->TMtable.size();
}

extern const char *const luaT_eventname[];


void luaT_init (lua_State *L);
void luaT_realtag (lua_State *L, int tag);
int luaT_validevent (int t, int e);  /* used by compatibility module */


#endif
