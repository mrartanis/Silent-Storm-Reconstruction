#ifndef __INPUT_H__
#define __INPUT_H__
////////////////////////////////////////////////////////////////////////////////////////////////////
#include <cstdint>
class CDataStream;
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NInput
{
typedef std::uint32_t STime;
////////////////////////////////////////////////////////////////////////////////////////////////////
	enum EPOVAxis
	{
		PA_UNKNOWN,
		PA_X,
		PA_Y
	};
	enum EControlType
	{
		CT_KEY,
		CT_POV,
		CT_AXIS,
		CT_TIME,
		CT_LIMAXIS,
		CT_UNKNOWN,
		// Historical UI event names retained for compatibility. SDL text supplies
		// CT_WIN_CHAR; SDL key-down/repeat supplies CT_WIN_KEY in the UI's virtual-key
		// numbering. Physical CT_KEY edges remain separate from text and repeat.
		CT_WIN_CHAR,
		CT_WIN_KEY
	};
////////////////////////////////////////////////////////////////////////////////////////////////////
	struct SMessage
	{
		int nAction;
		EPOVAxis ePOVAxis;
		EControlType cType;

		int nParam;
		bool bState;
		STime tTime;
	};
////////////////////////////////////////////////////////////////////////////////////////////////////
	bool InitInput( bool bNonExclusiveMode = false, int nSampleBufferSize = -1 );
	bool DoneInput();

	void PumpMessages( bool bFocus );
	bool GetMessage( SMessage *pMsg );
	// Compatibility hook for synthesized UI events; the game uses SDL events.
	void AddWinMessage( EControlType cType, int nParam );
	bool GetKeyForMessage( const SMessage &mMsg, int *pnVirtualKey );
		
	int GetControlID( const string &sCommand );
	void GetControlInfo( int nAction, EControlType *pcType, float *pfGranularity );

	void StartSaveInput( CDataStream *pStream );
	void StopSaveInput();
	void StartEmulateInput( CDataStream *pStream );
	void StopEmulateInput();
////////////////////////////////////////////////////////////////////////////////////////////////////
};
////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
