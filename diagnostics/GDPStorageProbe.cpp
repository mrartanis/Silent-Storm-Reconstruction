// Read-only inventory of an original GDP OLE compound file. No extraction.
#include <Windows.h>
#include <objidl.h>
#include <cstdio>
#include <cwchar>

static bool List(IStorage *storage, int depth)
{
  IEnumSTATSTG *enumerator = nullptr;
  if (FAILED(storage->EnumElements(0, nullptr, 0, &enumerator))) return false;
  STATSTG item{};
  ULONG fetched = 0;
  while (enumerator->Next(1, &item, &fetched) == S_OK)
  {
    std::wprintf(L"%*ls%ls type=%lu size=%llu\n", depth * 2, L"", item.pwcsName,
                 item.type, static_cast<unsigned long long>(item.cbSize.QuadPart));
    if (item.type == STGTY_STORAGE)
    {
      IStorage *child = nullptr;
      const HRESULT opened = storage->OpenStorage(item.pwcsName, nullptr,
              STGM_READ | STGM_SHARE_EXCLUSIVE, nullptr, 0, &child);
      if (FAILED(opened) || !List(child, depth + 1))
      {
        std::fwprintf(stderr, L"cannot open child %ls: HRESULT %08lx\n",
                      item.pwcsName, opened);
        if (child) child->Release();
        CoTaskMemFree(item.pwcsName);
        enumerator->Release();
        return false;
      }
      child->Release();
    }
    CoTaskMemFree(item.pwcsName);
  }
  enumerator->Release();
  return true;
}

int wmain(int argc, wchar_t **argv)
{
  if (argc != 2)
  {
    std::fwprintf(stderr, L"usage: GDPStorageProbe file.gdp\n");
    return 2;
  }
  const HRESULT init = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  if (FAILED(init)) return 3;
  IStorage *root = nullptr;
  const HRESULT open = StgOpenStorage(argv[1], nullptr,
      STGM_READ | STGM_SHARE_DENY_WRITE, nullptr, 0, &root);
  if (FAILED(open))
  {
    std::fwprintf(stderr, L"cannot open storage: HRESULT %08lx\n", open);
    CoUninitialize();
    return 3;
  }
  const bool ok = List(root, 0);
  root->Release();
  CoUninitialize();
  return ok ? 0 : 3;
}
