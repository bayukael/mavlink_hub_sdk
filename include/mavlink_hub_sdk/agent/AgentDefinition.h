#pragma once

#include "mavlink_hub_sdk/agent/Agent.h"
#include "mavlink_hub_sdk/agent/AgentConfig.h"
#include "mavlink_hub_sdk/common/ParseResult.h"
#include "mavlink_hub_sdk/manager_resource_requester/IManagerResourceRequester.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace pendarlab::sdk::mavlink_hub
{
  using AgentConfigParseResult = ParseResult<AgentConfig>;
  class AgentDefinition
  {
  public:
    virtual AgentConfigParseResult parseConfig(const std::unordered_map<std::string, std::string>& cfg) const = 0;
    virtual std::unique_ptr<Agent> create(const AgentConfig& cfg, std::unique_ptr<IManagerResourceRequester> requester) const = 0;
    virtual ~AgentDefinition() = default;
  };
} // namespace pendarlab::sdk::mavlink_hub
