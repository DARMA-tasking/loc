#include "dummy.h"

namespace loc::dummy {

  void Dummy::addDummy() {
    if(is_dummy_) {
      dummies_.insert(2);
    }
  }

  int Dummy::sum(int a, int b) const {
    return a + b;
  }

} // namespace loc::dummy
