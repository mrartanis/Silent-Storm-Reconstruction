#ifndef __A5_SHOWOBJECTIVES_H_
#define __A5_SHOWOBJECTIVES_H_
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
////////////////////////////////////////////////////////////////////////////////////////////////////
// Retail "show objectives" modal (compiland iObjectivesmenu). The live mission UI
// uses these classes; CScenarioTracker supplies both clue-derived and script goals.
//
// Functions carried (VA = RVA + 0x400000):
//   NScenario::SGoalDescription::SGoalDescription(copy)              @0x220370
//   NUI::CShowObjectivesUI::CShowObjectivesUI                        @0x21e550
//   NUI::CShowObjectivesUI::ProcessMessage                          @0x21f3c0
//   NUI::CClueLine::CClueLine(SWindowInfo&,IMission*,SGoalDesc&,bool)@0x21eaf0
//   NUI::CClueLine::CClueLine()                                     @0x220540
//   NUI::CClueLine::ProcessMessage                                  @0x21eb90
//   NUI::CTaskLine::CTaskLine                                       @0x21ea80
//   NUI::CTaskLine::ProcessMessage                                  @0x21f020
//   NGame::CShowObjectivesInterface::CShowObjectivesInterface(def)   @0x21e5b0
//   NGame::CShowObjectivesInterface::CShowObjectivesInterface(copy)  @0x2201b0
//   NGame::CShowObjectivesInterface::OnGetFocus                      @0x21e380
//   NGame::CShowObjectivesInterface::ProcessEvent                    @0x21e450
//   NGame::CShowObjectivesInterface::RenderFrame                     @0x21e4c0
//   NGame::CShowObjectivesInterface::Step                           @0x21e4f0
//   NGame::CShowObjectivesInterface::Initialize                     @0x21e640
//   NGame::CICShowObjectives::CICShowObjectives(payload)            @0x21e9c0
//   NGame::CICShowObjectives::CICShowObjectives(copy)               @0x21ffa0
//   NGame::CICShowObjectives::Exec                                  @0x21ea00
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "iMain.h"
#include "scScenarioTracker.h"		// list/vector/CDBPtr/NDb::CString + forward CScenarioZone
#include "scFlowChartItems.h"		// NScenario::ETaskState + runtime CScenarioGoal/CScenarioTask/CScenarioZone
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NGame { class IMission; }
namespace NUI   { class CScreenShot; }
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NScenario
{
////////////////////////////////////////////////////////////////////////////////////////////////////
// Release task/goal completion state (parallels dev ETaskState; same numeric values).
////////////////////////////////////////////////////////////////////////////////////////////////////
enum EScenarioTaskState : int
{
	STS_UNKNOWN   = 0,
	STS_COMPLETED = 1,
	STS_FAILED    = 2,
};
////////////////////////////////////////////////////////////////////////////////////////////////////
inline EScenarioTaskState TaskStateToScenario( ETaskState eState )
{
	switch ( eState )
	{
	case TS_COMPLETED:	return STS_COMPLETED;
	case TS_FAILED:		return STS_FAILED;
	default:			return STS_UNKNOWN;
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// STaskDescription (release, size 8) -- a goal's sub-task value: localized string + state.
////////////////////////////////////////////////////////////////////////////////////////////////////
struct STaskDescription
{
	CDBPtr<NDb::CString>	pString;
	EScenarioTaskState		state;
	STaskDescription(): state( STS_UNKNOWN ) {}
};
////////////////////////////////////////////////////////////////////////////////////////////////////
// SGoalDescription (release, size 20) -- a goal value: localized string + state + its tasks.
////////////////////////////////////////////////////////////////////////////////////////////////////
struct SGoalDescription
{
	CDBPtr<NDb::CString>		pString;
	EScenarioTaskState			state;
	vector< STaskDescription >	tasks;
	SGoalDescription(): state( STS_UNKNOWN ) {}
	SGoalDescription( const SGoalDescription &src );	// @0x220370
};
////////////////////////////////////////////////////////////////////////////////////////////////////
} // namespace NScenario
////////////////////////////////////////////////////////////////////////////////////////////////////
namespace NGame
{
////////////////////////////////////////////////////////////////////////////////////////////////////
// CICShowObjectives -- queued main-loop command that builds + pushes a CShowObjectivesInterface modal
// for one scenario zone (carrying an optional frozen screenshot backdrop). Sibling of CICObjectives,
// but per-zone + screenshot like the retail command. Transient command object -- not serialized (matches
// the unregistered sibling CICObjectives), so no REGISTER_SAVELOAD_CLASS / ZDATA here.
////////////////////////////////////////////////////////////////////////////////////////////////////
class CICShowObjectives: public NMainLoop::CInterfaceCommand
{
	OBJECT_BASIC_METHODS(CICShowObjectives);
private:
	CPtr<IMission>					pMission;
	CPtr<NUI::CScreenShot>			pScreenShot;
	CPtr<NScenario::CScenarioZone>	pZone;

public:
	CICShowObjectives() {}
	CICShowObjectives( IMission *pMission, NScenario::CScenarioZone *pZone, NUI::CScreenShot *pScreenShot = 0 );	// @0x21e9c0
	CICShowObjectives( const CICShowObjectives &src );		// @0x21ffa0

	virtual void Exec();									// @0x21ea00
};
////////////////////////////////////////////////////////////////////////////////////////////////////
} // namespace NGame
////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
