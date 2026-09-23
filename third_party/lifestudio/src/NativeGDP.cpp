#include "LifeStudioHeadAPIGDP.h"
#include <Windows.h>
#include <objidl.h>
#include <algorithm>
#include <climits>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

namespace
{
std::wstring Wide(const char *name)
{
  if (!name || !*name) return {};
  const int count = MultiByteToWideChar(CP_ACP, 0, name, -1, nullptr, 0);
  if (count <= 1) return {};
  std::wstring result(count, L'\0');
  if (MultiByteToWideChar(CP_ACP, 0, name, -1, &result[0], count) != count)
    return {};
  result.pop_back();
  return result;
}

std::string Narrow(const wchar_t *name)
{
  if (!name || !*name) return {};
  const int count = WideCharToMultiByte(CP_ACP, 0, name, -1, nullptr, 0, nullptr, nullptr);
  if (count <= 1) return {};
  std::string result(count, '\0');
  if (WideCharToMultiByte(CP_ACP, 0, name, -1, &result[0], count, nullptr, nullptr) != count)
    return {};
  result.pop_back();
  return result;
}

std::vector<std::string> Children(IStorage *storage, DWORD type)
{
  std::vector<std::string> result;
  if (!storage) return result;
  IEnumSTATSTG *enumerator = nullptr;
  if (FAILED(storage->EnumElements(0, nullptr, 0, &enumerator))) return result;
  STATSTG item{};
  ULONG fetched = 0;
  while (enumerator->Next(1, &item, &fetched) == S_OK)
  {
    if (item.type == type) result.push_back(Narrow(item.pwcsName));
    CoTaskMemFree(item.pwcsName);
  }
  enumerator->Release();
  std::sort(result.begin(), result.end());
  return result;
}

bool StreamSize(IStorage *storage, const char *name, std::uint32_t *size)
{
  if (!storage || !size) return false;
  const std::wstring wide = Wide(name);
  if (wide.empty()) return false;
  IStream *stream = nullptr;
  if (FAILED(storage->OpenStream(wide.c_str(), nullptr,
             STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &stream))) return false;
  STATSTG state{};
  const HRESULT status = stream->Stat(&state, STATFLAG_NONAME);
  stream->Release();
  if (FAILED(status) || state.cbSize.QuadPart > UINT32_MAX) return false;
  *size = static_cast<std::uint32_t>(state.cbSize.QuadPart);
  return true;
}

bool ReadStream(IStorage *storage, const char *name, char *buffer, std::uint32_t size)
{
  if (!storage || !buffer) return false;
  const std::wstring wide = Wide(name);
  if (wide.empty()) return false;
  IStream *stream = nullptr;
  if (FAILED(storage->OpenStream(wide.c_str(), nullptr,
             STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &stream))) return false;
  ULONG read = 0;
  const HRESULT status = stream->Read(buffer, size, &read);
  stream->Release();
  return SUCCEEDED(status) && read == size;
}

std::vector<std::string> DataList(IStorage *storage)
{
  std::uint32_t length = 0;
  if (!StreamSize(storage, "Morph.txt", &length) || length == 0 || length > 1000000)
    return Children(storage, STGTY_STREAM);
  std::string morph(length, '\0');
  if (!ReadStream(storage, "Morph.txt", &morph[0], length)) return {};
  std::vector<std::string> result{"Morph.txt"};
  std::istringstream input(morph);
  std::string line;
  while (std::getline(input, line))
  {
    std::istringstream fields(line);
    std::string tag;
    fields >> tag;
    if (tag != "HEAD") continue;
    std::string token, stem;
    while (fields >> token) stem = token;
    if (stem.empty()) continue;
    const std::string animator = stem + "_A.mld";
    const std::string muscles = stem + "_M.mld";
    if (!StreamSize(storage, animator.c_str(), &length) ||
        !StreamSize(storage, muscles.c_str(), &length)) return {};
    result.push_back(animator);
    result.push_back(muscles);
  }
  if (StreamSize(storage, "Links.dat", &length)) result.push_back("Links.dat");
  return result;
}

class NativeGDPObject final : public LifeStudioHeadAPI::IGDPObject
{
  IStorage *storage;
  std::vector<std::string> data;
  std::vector<std::string> children;
public:
  explicit NativeGDPObject(IStorage *value): storage(value),
      data(DataList(value)), children(Children(value, STGTY_STORAGE)) {}
  ~NativeGDPObject() { if (storage) storage->Release(); }

  int Size(const char *name) override
  {
    std::uint32_t length = 0;
    return StreamSize(storage, name, &length) && length <= INT_MAX ? int(length) : 0;
  }
  bool Get(const char *name, char *buffer) override
  {
    std::uint32_t length = 0;
    return StreamSize(storage, name, &length) &&
           ReadStream(storage, name, buffer, length);
  }
  int MaterialsCount() const override { return 0; }
  bool Material(int, LifeStudioHeadAPI::ObjectMaterial &) const override { return false; }
  int TrianglesCount(int) const override { return 0; }
  const unsigned short *Triangulation(int) const override { return nullptr; }
  int VerticesCount() const override
  {
    char header[12]{};
    std::uint32_t size = 0;
    if (!StreamSize(storage, "object_A.mld", &size) || size < sizeof(header)) return 0;
    const std::wstring wide = Wide("object_A.mld");
    IStream *stream = nullptr;
    if (FAILED(storage->OpenStream(wide.c_str(), nullptr,
               STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &stream))) return 0;
    ULONG read = 0;
    const HRESULT status = stream->Read(header, sizeof(header), &read);
    stream->Release();
    if (FAILED(status) || read != sizeof(header)) return 0;
    std::uint32_t count;
    std::memcpy(&count, header + 8, 4);
    return count <= 1000000 ? int(count) : 0;
  }
  const float *UV() const override { return nullptr; }
  const float *UVNoChg() const override { return nullptr; }
  bool HasExtenedUVInfo() const override { return false; }
  int UVCount() const override { return 0; }
  int BaseTrianglesCount() const override { return 0; }
  const unsigned short *BaseTriangulation() const override { return nullptr; }
  const unsigned short *UV2VMap() const override { return nullptr; }
  int AdditionalNormalsDataSize() const override { return 0; }
  bool AdditionalNormalsData(char *) override { return false; }
  int PNGTextureSize(const char *) const override { return 0; }
  bool PNGTexture(const char *, char *) override { return false; }
  bool IsTransformable() const override
  {
    std::uint32_t size = 0;
    return StreamSize(storage, "Morph.txt", &size) && size > 0;
  }
  int DataListSize() const override { return int(data.size()); }
  const char *DataListItem(int index) const override
  {
    return index >= 0 && std::size_t(index) < data.size() ? data[index].c_str() : nullptr;
  }
  int DefaultAnimatorDataSize() const override
  {
    std::uint32_t size = 0;
    return StreamSize(storage, "object_A.mld", &size) && size <= INT_MAX ? int(size) : 0;
  }
  bool DefaultAnimatorData(char *buffer) override
  {
    std::uint32_t size = 0;
    return StreamSize(storage, "object_A.mld", &size) &&
           ReadStream(storage, "object_A.mld", buffer, size);
  }
  int SubObjectsCount() const override { return int(children.size()); }
  const char *SubObjectName(int index) const override
  {
    return index >= 0 && std::size_t(index) < children.size() ? children[index].c_str() : nullptr;
  }
  const char *SubObjectType(int) const override { return ""; }
  LifeStudioHeadAPI::IGDPObject *SubObject(int index) override
  {
    if (index < 0 || std::size_t(index) >= children.size()) return nullptr;
    const std::wstring wide = Wide(children[index].c_str());
    IStorage *child = nullptr;
    if (FAILED(storage->OpenStorage(wide.c_str(), nullptr,
               STGM_READ | STGM_SHARE_EXCLUSIVE, nullptr, 0, &child))) return nullptr;
    return new NativeGDPObject(child);
  }
  void Destroy() override { delete this; }
};

class NativeGDPFile final : public LifeStudioHeadAPI::IGDPFile
{
  IStorage *root;
  std::vector<std::string> objects;
public:
  explicit NativeGDPFile(IStorage *value): root(value), objects(Children(value, STGTY_STORAGE)) {}
  ~NativeGDPFile() { if (root) root->Release(); }
  int ObjectsCount() const override { return int(objects.size()); }
  const char *ObjectName(int index) const override
  {
    return index >= 0 && std::size_t(index) < objects.size() ? objects[index].c_str() : nullptr;
  }
  LifeStudioHeadAPI::IGDPObject *Object(int index) override
  {
    if (index < 0 || std::size_t(index) >= objects.size()) return nullptr;
    const std::wstring wide = Wide(objects[index].c_str());
    IStorage *storage = nullptr;
    if (FAILED(root->OpenStorage(wide.c_str(), nullptr,
               STGM_READ | STGM_SHARE_EXCLUSIVE, nullptr, 0, &storage))) return nullptr;
    return new NativeGDPObject(storage);
  }
  void Destroy() override { delete this; }
};
}

namespace LifeStudioHeadAPI
{
IGDPFile *__stdcall IGDPFile::Create(const char *filename)
{
  const std::wstring wide = Wide(filename);
  if (wide.empty()) return nullptr;
  IStorage *storage = nullptr;
  if (FAILED(StgOpenStorage(wide.c_str(), nullptr,
      STGM_READ | STGM_SHARE_DENY_WRITE, nullptr, 0, &storage))) return nullptr;
  return new NativeGDPFile(storage);
}
}
