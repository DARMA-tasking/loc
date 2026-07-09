#include <set>

namespace loc::dummy {

  struct Dummy {
    void addDummy();
    int sum(int a, int b) const;

    private:
      bool is_dummy_ = true;
      std::set<int> dummies_ = {};
  };

} // namespace loc::dummy
