#ifndef __FONTFORMAT_H__
#define __FONTFORMAT_H__
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "../FileIO/PortableFontWire.h"
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma pack( 4 )
// complete necessary one letter description
struct STFCharacter
{
  int x1, y1, x2, y2;                 // rect in texture's coords [0..1]
  int nA;                             // character's pre-space
  int nBC;                            // character's B + C = distance to the next character
  int nWidth;                         // lone character's width (B + (C > 0 ? C : 0))
};
#pragma pack()
static_assert(sizeof(STFCharacter) == 28 && sizeof(int) == 4,
              "game font character wire size");
namespace S2FileIO {
template<>
struct StructureFieldCodec<STFCharacter, void> {
  static constexpr bool kPortable = true;
  static constexpr std::size_t kWireSize = 28;
  static bool Decode(const std::uint8_t* source, std::size_t length,
                     STFCharacter* value) {
    if (!value) return false;
    FontCharacterFields fields{};
    if (!DecodeFontCharacter(source, length, &fields)) return false;
    value->x1 = fields.x1; value->y1 = fields.y1;
    value->x2 = fields.x2; value->y2 = fields.y2;
    value->nA = fields.advanceA; value->nBC = fields.advanceBC;
    value->nWidth = fields.width;
    return true;
  }
  static bool Encode(const STFCharacter& value, std::uint8_t* destination,
                     std::size_t length) {
    return EncodeFontCharacter(FontCharacterFields{
        value.x1, value.y1, value.x2, value.y2,
        value.nA, value.nBC, value.nWidth}, destination, length);
  }
};
} // namespace S2FileIO
////////////////////////////////////////////////////////////////////////////////////////////////////
class CFontFormatInfo: public CObjectBase
{
	OBJECT_BASIC_METHODS( CFontFormatInfo );
	typedef std::unordered_map<WORD, STFCharacter> CCharacterMap;
	typedef std::unordered_map<DWORD, int> CKernMap;
	//
  CCharacterMap chars;                  // all available characters map
  CKernMap kerns;                       // kerning pairs for the characters in the font.
	//
	int nHeight;													// native height of this font (in native pixels!)
	int nExternalLeading;									// extra leading (space) that the application adds between rows
  int nAveCharWidth;										// average width of characters in the font (generally defined as the width of the letter x).
  int nMaxCharWidth;										// width of the widest character in the font
  BYTE cCharSet;                        // character set of the font
	WORD wDefaultChar;										// alue of the character to be substituted for characters not in the font

public:
	// retrieve character description
  const STFCharacter& GetChar( const WORD c ) const
	{
		CCharacterMap::const_iterator pos = chars.find( c );
		if ( pos == chars.end() )
		{
			pos = chars.find( wDefaultChar );
			if ( pos == chars.end() )
			{
				ASSERT( 0 );
				return chars.begin()->second;
			}
		}
		return pos->second;
	}
	// retrieve kerning pair width
	int GetKern( WORD wChar, WORD wLastChar ) const
	{
    CKernMap::const_iterator pos = kerns.find( (DWORD(wLastChar) << 16) | DWORD(wChar) );
		return pos != kerns.end() ? pos->second : 0;
	}
	//
	int GetHeight() const { return nHeight; }
	int GetLineSpace() const { return nHeight + nExternalLeading; }
	int GetAveCharWidth() const { return nAveCharWidth; }
	int GetMaxCharWidth() const { return nMaxCharWidth; }
	//
	int operator&( CStructureSaver &f );
	friend class CFontGen;
};
////////////////////////////////////////////////////////////////////////////////////////////////////
#endif // __FONTFORMAT_H__
