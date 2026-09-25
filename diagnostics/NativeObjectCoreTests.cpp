#include <cstddef>
#include <cstdio>
#include <new>

#include <cassert>
#define ASSERT(value) assert(value)
#include "../Misc/Basic2.h"
#include "../Misc/EventsBase.h"

#ifdef _WIN32
// The game defines this during save teardown; this standalone test never enters it.
bool g_bSaveLoadTeardown = false;
#endif

namespace {
int live = 0;
int destroyed = 0;

struct Lifetime {
  Lifetime() { ++live; }
  Lifetime(const Lifetime&) { ++live; }
  ~Lifetime() { --live; ++destroyed; }
};

struct Interface {
  virtual int Value() const = 0;
  virtual ~Interface() = default;
};

class TestObject : public CObjectBase, public Interface {
  OBJECT_BASIC_METHODS(TestObject);
 public:
  explicit TestObject(int number = 0) : number_(number) {}
  int Value() const override { return number_; }
  void OnEvent(const int& delta) { sum_ += delta; }
  int Sum() const { return sum_; }
 private:
  int number_;
  int sum_ = 0;
  Lifetime lifetime_;
};
} // namespace

int main() {
  {
    CObj<TestObject> first = new TestObject(17);
    CObj<TestObject> second = first;
    CPtr<TestObject> observer = first.GetPtr();
    Interface* interfacePtr = first.GetPtr();
    if (live != 1 || destroyed != 0 || !IsValid(observer) ||
        CDynamicCast<TestObject>(interfacePtr).GetPtr() != first.GetPtr() ||
        observer->Value() != 17) return 1;
    {
      NGlobal::CEventRegister<TestObject, int> registration(first.GetPtr(),
          &TestObject::OnEvent);
      NGlobal::ThrowEvent(5);
      if (first->Sum() != 5) return 8;
      first = static_cast<TestObject*>(nullptr);
      NGlobal::ThrowEvent(7);
      if (observer->Sum() != 12) return 9;
      if (!IsValid(observer) || live != 1 || destroyed != 0) return 2;
      second = static_cast<TestObject*>(nullptr);
      NGlobal::ThrowEvent(11);
      if (IsValid(observer) || observer->Sum() != 0 || live != 1 ||
          destroyed != 1) return 3;
    }
    observer = static_cast<TestObject*>(nullptr);
    if (live != 0 || destroyed != 2) return 4;
  }
  {
    int* active = nullptr;
    int value = 23;
    {
      CFWContext context(&active, &value);
      if (active != &value || *active != 23) return 5;
    }
    if (active != nullptr || live != 0) return 6;
  }
  if (live != 0 || destroyed != 2) return 7;
  std::printf("native-object-core lifetimes %d\n", destroyed);
  return 0;
}
