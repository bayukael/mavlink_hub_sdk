#pragma once

#include <memory>
#include <vector>
#include <string>

#include "mavlink_hub_sdk/agent/AgentConfig.h"

namespace pendarlab::sdk::mavlink_hub
{
  struct AgentConfigParseResult{
    std::unique_ptr<AgentConfig> parsed;
    std::vector<std::string> messages;

    bool ok() const {return parsed != nullptr;}
  };
} // namespace pendarlab::sdk::mavlink_hub
