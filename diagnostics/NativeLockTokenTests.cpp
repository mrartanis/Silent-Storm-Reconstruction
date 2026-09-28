#if defined(_WIN32)
#include "../Main/StdAfx.h"
#else
#include "../FileIO/StdAfx.h"
#include "../FileIO/BasicChunk1.h"
#endif
#include "../Main/Locks.h"

#include <cstdint>
#include <cstdio>

int main()
{
	if (!pSSClasses ||
		pSSClasses->GetTypeID(static_cast<CLockObject*>(nullptr)) != 0x23065400)
		return 1;
	CObj<CLockObject> owner = new CLockObject;
	CObj<CLockObject> other = new CLockObject;
	CLockable lock;
	if (lock.IsLocked(other)) return 2;
	CObjectBase *token = lock.Lock(owner);
	if (!token || token != lock.pLock.GetPtr() || lock.pLock->GetHandle() != owner)
		return 3;
	if (lock.Lock(owner) != token || lock.IsLocked(owner) ||
		!lock.IsLocked(other) || lock.Lock(other))
		return 4;

	CMemoryStream stream;
	try {
		CStructureSaver saver(stream, CStructureSaver::WRITE);
		saver.Add(1, &lock);
	} catch (...) { return 5; }
	std::uint64_t digest = UINT64_C(14695981039346656037);
	for (int i = 0; i < stream.GetSize(); ++i) {
		digest ^= stream.GetBuffer()[i];
		digest *= UINT64_C(1099511628211);
	}
	owner = nullptr;
	other = nullptr;
	lock.pLock = nullptr;
	stream.SetRMode();
	stream.Seek(0);
	try {
		CStructureSaver saver(stream, CStructureSaver::READ);
		saver.Add(1, &lock);
	} catch (...) { return 6; }
	if (!lock.pLock || !lock.pLock->GetHandle()) return 7;
	CObjectBase *restoredOwner = lock.pLock->GetHandle();
	CObj<CLockObject> newOther = new CLockObject;
	if (lock.IsLocked(restoredOwner) || !lock.IsLocked(newOther) ||
		lock.Lock(restoredOwner) != lock.pLock.GetPtr() || lock.Lock(newOther))
		return 8;
	if (stream.GetSize() != 70 || digest != UINT64_C(0x5A6C2F212363E2E7))
		return 9;
	std::printf("lock token id=23065400 bytes=%d fnv=%016llX\n",
		stream.GetSize(), static_cast<unsigned long long>(digest));
	return 0;
}
