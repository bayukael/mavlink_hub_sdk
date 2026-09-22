#pragma once

#include <functional>
#include <mavlink/common/mavlink.h>
#include <mavlink_endpoint/MavlinkEndpointPacket.h>
#include <mavlink_endpoint/MavlinkEndpointState.h>
#include <mavlink_endpoint/MavlinkEndpointToken.h>
#include <memory>
#include <optional>

namespace pendarlab::sdk::mavlink_hub
{
  class IMavlinkEndpointUser
  {
    using MavlinkEndpointToken = pendarlab::lib::comm::MavlinkEndpointToken;
    using MavlinkEndpointState = pendarlab::lib::comm::MavlinkEndpointState;
    using MavlinkEndpointPacket = pendarlab::lib::comm::MavlinkEndpointPacket;

  public:
    virtual ~IMavlinkEndpointUser() = default;
    virtual std::unique_ptr<MavlinkEndpointToken> createListener(std::function<void(const MavlinkEndpointPacket&)> listener_cb) = 0;
    virtual std::optional<int> getNumOfListener() = 0;
    virtual std::optional<int> writeMessage(const mavlink_message_t& msg) = 0;
    virtual std::optional<MavlinkEndpointState> getState() = 0;
  };

} // namespace pendarlab::sdk::mavlink_hub
