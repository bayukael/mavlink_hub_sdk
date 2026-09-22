#pragma once

namespace pendarlab::sdk::mavlink_hub
{
  enum class AgentState{
    IDLE,
    STARTING,
    ACTIVE,
    STOPPING
  };
} // namespace pendarlab::sdk::mavlink_hub
