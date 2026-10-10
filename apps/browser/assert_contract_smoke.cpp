#include <cassert>
#include <cstdlib>

#ifdef NDEBUG
#error "Browser smoke targets must keep assertions active in Release builds"
#endif

int main() {
  bool assertion_expression_evaluated = false;
  assert((assertion_expression_evaluated = true));
  return assertion_expression_evaluated ? EXIT_SUCCESS : EXIT_FAILURE;
}
