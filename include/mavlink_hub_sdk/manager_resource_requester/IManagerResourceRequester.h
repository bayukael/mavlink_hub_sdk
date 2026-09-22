#pragma once

#include "mavlink_hub_sdk/mavlink_endpoint_user/IMavlinkEndpointUser.h"

#include <memory>
#include <string>

namespace pendarlab::sdk::mavlink_hub
{
  class IManagerResourceRequester
  {
  public:
    virtual ~IManagerResourceRequester() = default;
    virtual std::unique_ptr<IMavlinkEndpointUser> requestMavlinkEndpoint(const std::string& ep_name) = 0;
  };

} // namespace pendarlab::sdk::mavlink_hub
