#include <cstdint>
#include <cstdlib>
#include <string>
#include <utility>

#include "goreecloud/browser/private_permission_decision_store.hpp"

using goreecloud::browser::EphemeralPermissionDecision;
using goreecloud::browser::PermissionBroker;
using goreecloud::browser::PermissionDecision;
using goreecloud::browser::PermissionDecisionSource;
using goreecloud::browser::PermissionLifecycleState;
using goreecloud::browser::PermissionRequest;
using goreecloud::browser::PermissionResource;
using goreecloud::browser::PrivatePermissionContext;
using goreecloud::browser::PrivatePermissionDecisionStore;
using goreecloud::browser::PrivatePermissionLifecycleCoordinator;
using goreecloud::browser::PrivacyContext;

namespace {

void require(bool condition) {
  if (!condition) std::abort();
}

EphemeralPermissionDecision decision(
    std::string profile_id,
    std::string context_id,
    PrivacyContext context,
    PermissionResource resource = PermissionResource::camera,
    PermissionDecision value = PermissionDecision::allow_session,
    std::int64_t created_at = 1000,
    std::int64_t expires_at = 0) {
  return EphemeralPermissionDecision{
      .profile_id = std::move(profile_id),
      .privacy_context_id = std::move(context_id),
      .privacy_context = context,
      .origin = "https://example.test",
      .resource = resource,
      .decision = value,
      .source = PermissionDecisionSource::user,
      .created_at_millis = created_at,
      .expires_at_millis = expires_at,
      .revoked_at_millis = 0,
      .contract_version = 1,
  };
}

PermissionRequest request(std::string request_id,
                          std::string profile_id,
                          std::string context_id,
                          PrivacyContext context) {
  return PermissionRequest{
      .request_id = std::move(request_id),
      .profile_id = std::move(profile_id),
      .privacy_context_id = std::move(context_id),
      .privacy_context = context,
      .tab_owner_id = "tab-owner",
      .top_level_origin = "https://example.test",
      .requesting_origin = "https://example.test",
      .resources = {PermissionResource::camera},
      .user_gesture = true,
      .created_at_millis = 1000,
      .expires_at_millis = 5000,
      .contract_version = 1,
  };
}

}  // namespace

int main() {
  PermissionBroker broker;
  PrivatePermissionDecisionStore store;
  PrivatePermissionLifecycleCoordinator lifecycle(broker, store);

  require(store.open_context(PrivatePermissionContext{
      .profile_id = "profile-personal",
      .privacy_context_id = "private-shared",
      .privacy_context = PrivacyContext::private_browsing,
      .contract_version = 1,
  }));
  require(store.open_context(PrivatePermissionContext{
      .profile_id = "profile-personal",
      .privacy_context_id = "isolated-one",
      .privacy_context = PrivacyContext::isolated_private,
      .contract_version = 1,
  }));
  require(store.open_context(PrivatePermissionContext{
      .profile_id = "profile-work",
      .privacy_context_id = "private-shared",
      .privacy_context = PrivacyContext::private_browsing,
      .contract_version = 1,
  }));
  require(!store.open_context(PrivatePermissionContext{
      .profile_id = "profile-personal",
      .privacy_context_id = "normal-one",
      .privacy_context = PrivacyContext::normal,
      .contract_version = 1,
  }));

  auto camera = decision("profile-personal", "private-shared",
                         PrivacyContext::private_browsing);
  require(store.upsert(camera));
  require(store.upsert(camera));
  require(store.lookup("profile-personal", "private-shared",
                       PrivacyContext::private_browsing,
                       "https://example.test", PermissionResource::camera,
                       1200)
              .has_value());

  auto persistent = decision(
      "profile-personal", "private-shared", PrivacyContext::private_browsing,
      PermissionResource::microphone, PermissionDecision::allow_persistent);
  require(!store.upsert(persistent));

  auto once = decision(
      "profile-personal", "private-shared", PrivacyContext::private_browsing,
      PermissionResource::microphone, PermissionDecision::deny_once);
  require(!store.upsert(once));

  auto invalid_resource =
      decision("profile-personal", "private-shared",
               PrivacyContext::private_browsing);
  invalid_resource.resource = static_cast<PermissionResource>(255);
  require(!store.upsert(invalid_resource));

  auto wrong_context_type =
      decision("profile-personal", "private-shared",
               PrivacyContext::isolated_private);
  require(!store.upsert(wrong_context_type));

  auto expiring = decision(
      "profile-personal", "private-shared", PrivacyContext::private_browsing,
      PermissionResource::geolocation, PermissionDecision::deny_session, 1100,
      2000);
  require(store.upsert(expiring));
  require(store.lookup("profile-personal", "private-shared",
                       PrivacyContext::private_browsing,
                       "https://example.test", PermissionResource::geolocation,
                       1999)
              .has_value());
  require(!store.lookup("profile-personal", "private-shared",
                        PrivacyContext::private_browsing,
                        "https://example.test", PermissionResource::geolocation,
                        2000)
               .has_value());

  require(store.revoke("profile-personal", "private-shared",
                       PrivacyContext::private_browsing,
                       "https://example.test", PermissionResource::camera,
                       1500));
  require(!store.lookup("profile-personal", "private-shared",
                        PrivacyContext::private_browsing,
                        "https://example.test", PermissionResource::camera,
                        1500)
               .has_value());

  auto stale = decision(
      "profile-personal", "private-shared", PrivacyContext::private_browsing,
      PermissionResource::camera, PermissionDecision::deny_session, 1499);
  require(!store.upsert(stale));
  auto fresh = decision(
      "profile-personal", "private-shared", PrivacyContext::private_browsing,
      PermissionResource::camera, PermissionDecision::deny_session, 1501);
  require(store.upsert(fresh));

  auto work = decision("profile-work", "private-shared",
                       PrivacyContext::private_browsing,
                       PermissionResource::microphone,
                       PermissionDecision::allow_session);
  require(store.upsert(work));
  require(!store.lookup("profile-personal", "private-shared",
                        PrivacyContext::private_browsing,
                        "https://example.test", PermissionResource::microphone,
                        1600)
               .has_value());
  require(store.lookup("profile-work", "private-shared",
                       PrivacyContext::private_browsing,
                       "https://example.test", PermissionResource::microphone,
                       1600)
              .has_value());

  auto isolated = decision("profile-personal", "isolated-one",
                           PrivacyContext::isolated_private,
                           PermissionResource::protected_media,
                           PermissionDecision::deny_session);
  require(store.upsert(isolated));
  require(store.lookup("profile-personal", "isolated-one",
                       PrivacyContext::isolated_private,
                       "https://example.test",
                       PermissionResource::protected_media, 1600)
              .has_value());
  require(!store.lookup("profile-personal", "isolated-one",
                        PrivacyContext::private_browsing,
                        "https://example.test",
                        PermissionResource::protected_media, 1600)
               .has_value());

  require(store.reset_origin("profile-personal", "private-shared",
                             PrivacyContext::private_browsing,
                             "https://example.test", 1700) == 2);
  require(!store.lookup("profile-personal", "private-shared",
                        PrivacyContext::private_browsing,
                        "https://example.test", PermissionResource::camera,
                        1700)
               .has_value());

  auto invalid_request =
      request("request-invalid-resource", "profile-personal",
              "private-shared", PrivacyContext::private_browsing);
  invalid_request.resources = {static_cast<PermissionResource>(255)};
  require(!broker.receive(std::move(invalid_request)));

  require(broker.receive(request("request-personal-private",
                                 "profile-personal", "private-shared",
                                 PrivacyContext::private_browsing)));
  require(broker.receive(request("request-work-private", "profile-work",
                                 "private-shared",
                                 PrivacyContext::private_browsing)));
  require(broker.receive(request("request-personal-isolated",
                                 "profile-personal", "isolated-one",
                                 PrivacyContext::isolated_private)));

  const auto closed = lifecycle.close_and_forget(
      "profile-personal", "private-shared",
      PrivacyContext::private_browsing);
  require(closed.accepted);
  require(closed.first_close);
  require(closed.decisions_destroyed == 2);
  require(closed.requests_cancelled == 1);
  require(store.is_context_closed("profile-personal", "private-shared",
                                  PrivacyContext::private_browsing));
  require(broker.state("request-personal-private") ==
          PermissionLifecycleState::completed);
  require(broker.state("request-work-private") ==
          PermissionLifecycleState::received);
  require(broker.state("request-personal-isolated") ==
          PermissionLifecycleState::received);

  require(!store.upsert(fresh));
  require(!store.open_context(PrivatePermissionContext{
      .profile_id = "profile-personal",
      .privacy_context_id = "private-shared",
      .privacy_context = PrivacyContext::private_browsing,
      .contract_version = 1,
  }));

  require(store.lookup("profile-work", "private-shared",
                       PrivacyContext::private_browsing,
                       "https://example.test", PermissionResource::microphone,
                       1800)
              .has_value());
  require(store.lookup("profile-personal", "isolated-one",
                       PrivacyContext::isolated_private,
                       "https://example.test",
                       PermissionResource::protected_media, 1800)
              .has_value());

  const auto repeated_close = lifecycle.close_and_forget(
      "profile-personal", "private-shared",
      PrivacyContext::private_browsing);
  require(repeated_close.accepted);
  require(!repeated_close.first_close);
  require(repeated_close.decisions_destroyed == 0);
  require(repeated_close.requests_cancelled == 0);

  const auto mismatched_close = lifecycle.close_and_forget(
      "profile-personal", "isolated-one", PrivacyContext::private_browsing);
  require(!mismatched_close.accepted);
  require(broker.state("request-personal-isolated") ==
          PermissionLifecycleState::received);

  const auto isolated_close = lifecycle.close_and_forget(
      "profile-personal", "isolated-one",
      PrivacyContext::isolated_private);
  require(isolated_close.accepted);
  require(isolated_close.first_close);
  require(isolated_close.decisions_destroyed == 1);
  require(isolated_close.requests_cancelled == 1);

  const auto work_close = lifecycle.close_and_forget(
      "profile-work", "private-shared",
      PrivacyContext::private_browsing);
  require(work_close.accepted);
  require(work_close.first_close);
  require(work_close.decisions_destroyed == 1);
  require(work_close.requests_cancelled == 1);
  require(store.decision_count() == 0);

  {
    PrivatePermissionDecisionStore bounded_contexts;
    for (std::size_t index = 0;
         index < goreecloud::browser::kPrivatePermissionMaxContexts; ++index) {
      require(bounded_contexts.open_context(PrivatePermissionContext{
          .profile_id = "profile-bounded",
          .privacy_context_id = "private-active-" + std::to_string(index),
          .privacy_context = PrivacyContext::private_browsing,
          .contract_version = 1,
      }));
    }
    require(!bounded_contexts.open_context(PrivatePermissionContext{
        .profile_id = "profile-bounded",
        .privacy_context_id = "private-active-overflow",
        .privacy_context = PrivacyContext::private_browsing,
        .contract_version = 1,
    }));
  }

  {
    PrivatePermissionDecisionStore bounded_decisions;
    require(bounded_decisions.open_context(PrivatePermissionContext{
        .profile_id = "profile-bounded",
        .privacy_context_id = "private-decisions",
        .privacy_context = PrivacyContext::private_browsing,
        .contract_version = 1,
    }));
    for (std::size_t index = 0;
         index < goreecloud::browser::kPrivatePermissionMaxDecisionsPerContext;
         ++index) {
      auto record = decision("profile-bounded", "private-decisions",
                             PrivacyContext::private_browsing);
      record.origin = "https://site" + std::to_string(index) + ".test";
      require(bounded_decisions.upsert(std::move(record)));
    }
    auto overflow = decision("profile-bounded", "private-decisions",
                             PrivacyContext::private_browsing);
    overflow.origin = "https://overflow.test";
    require(!bounded_decisions.upsert(std::move(overflow)));
  }

  {
    PrivatePermissionDecisionStore bounded_lifetimes;
    for (std::size_t index = 0;
         index < goreecloud::browser::kPrivatePermissionMaxContextLifetimes;
         ++index) {
      const auto context_id = "private-lifetime-" + std::to_string(index);
      require(bounded_lifetimes.open_context(PrivatePermissionContext{
          .profile_id = "profile-bounded",
          .privacy_context_id = context_id,
          .privacy_context = PrivacyContext::private_browsing,
          .contract_version = 1,
      }));
      const auto close = bounded_lifetimes.close_context(
          "profile-bounded", context_id, PrivacyContext::private_browsing);
      require(close.accepted);
      require(close.first_close);
    }
    require(!bounded_lifetimes.open_context(PrivatePermissionContext{
        .profile_id = "profile-bounded",
        .privacy_context_id = "private-lifetime-overflow",
        .privacy_context = PrivacyContext::private_browsing,
        .contract_version = 1,
    }));
  }

  return 0;
}
