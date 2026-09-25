#include "../FileIO/StdAfx.h"
#include "../Misc/Basic2.h"
#include "../MiscDll/LogStream.h"

#include <iostream>

int main()
{
	consoleLines.clear();
	csAI << CC_RED << "path=" << 42 << L"\n";
	if ( consoleLines.size() != 1 )
	{
		std::cerr << "AI log did not emit one line\n";
		return 1;
	}
	const SConsoleLine &line = consoleLines.front();
	if ( line.eType != STREAM_AI || line.szText != L"<color=red>path=42" )
	{
		std::cerr << "AI log content or stream changed\n";
		return 1;
	}
	csSystem << true << L"\n";
	if ( consoleLines.size() != 2 ||
		consoleLines.front().szText != L"<green>true<white>" )
	{
		std::cerr << "System log boolean formatting changed\n";
		return 1;
	}
	return 0;
}
