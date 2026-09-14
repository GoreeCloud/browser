#include <cassert>
#include <stdexcept>
#include <string>

#include "goreecloud/browser/configured_search_router.hpp"

namespace {

bool throws_for_search(const goreecloud::browser::ConfiguredGoreeCloudSearchRouter& router) {
  try {
    (void)router.search_url("test");
  } catch (const std::runtime_error&) {
    return true;
  }
  return false;
}

}  // namespace

int main() {
  using goreecloud::browser::ConfiguredGoreeCloudSearchRouter;

  const ConfiguredGoreeCloudSearchRouter secure("https://search.goreecloud.test/search");
  assert(secure.configured());
  assert(secure.search_url("privacy shield") ==
         "https://search.goreecloud.test/search?q=privacy%20shield");
  assert(secure.search_url("a+b&c") ==
         "https://search.goreecloud.test/search?q=a%2Bb%26c");

  const ConfiguredGoreeCloudSearchRouter fixed_query(
      "https://search.goreecloud.test/search?source=browser");
  assert(fixed_query.configured());
  assert(fixed_query.search_url("Glaze UI") ==
         "https://search.goreecloud.test/search?source=browser&q=Glaze%20UI");

  const ConfiguredGoreeCloudSearchRouter query_delimiter(
      "https://search.goreecloud.test/search?");
  assert(query_delimiter.search_url("index") ==
         "https://search.goreecloud.test/search?q=index");

  const ConfiguredGoreeCloudSearchRouter local("http://localhost:8080/search");
  assert(local.configured());

  const ConfiguredGoreeCloudSearchRouter missing("");
  const ConfiguredGoreeCloudSearchRouter whitespace(" https://search.goreecloud.test/search");
  const ConfiguredGoreeCloudSearchRouter script("javascript:alert(1)");
  const ConfiguredGoreeCloudSearchRouter fragment(
      "https://search.goreecloud.test/search#unsafe-fragment");
  const ConfiguredGoreeCloudSearchRouter credentials(
      "https://user:secret@search.goreecloud.test/search");
  const ConfiguredGoreeCloudSearchRouter missing_authority("https:///search");

  assert(!missing.configured());
  assert(!whitespace.configured());
  assert(!script.configured());
  assert(!fragment.configured());
  assert(!credentials.configured());
  assert(!missing_authority.configured());
  assert(throws_for_search(missing));
  assert(throws_for_search(whitespace));
  assert(throws_for_search(script));
  assert(throws_for_search(fragment));
  assert(throws_for_search(credentials));
  assert(throws_for_search(missing_authority));

  return 0;
}
