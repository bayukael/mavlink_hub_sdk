#pragma once

#include "mavlink_hub_sdk/mavlink_endpoint_user/IMavlinkEndpointUser.h"

#include <memory>
#include <string>
#include <vector>

namespace pendarlab::sdk::mavlink_hub
{
  class IManagerResourceRequester
  {
  public:
    virtual ~IManagerResourceRequester() = default;
    virtual std::unique_ptr<IMavlinkEndpointUser> requestMavlinkEndpoint(const std::string& ep_name) = 0;

    /// @return the names of all currently-known MAVLink endpoints.
    ///
    /// The returned list is the full set of REGISTERED endpoints; it is not filtered by
    /// connection state, so endpoints that are currently disconnected may still appear.
    /// This lets an agent treat the returned set as the complete set of endpoints it may
    /// target (e.g. for "all except" or "every endpoint" targeting) rather than only the
    /// currently-connected subset.
    virtual std::vector<std::string> getMavlinkEndpointList() = 0;
  };

} // namespace pendarlab::sdk::mavlink_hub
