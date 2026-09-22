#pragma once

#include "mavlink_hub_sdk/agent/AgentConfig.h"
#include "mavlink_hub_sdk/agent/AgentState.h"
#include "mavlink_hub_sdk/manager_resource_requester/IManagerResourceRequester.h"

#include <memory>
#include <unordered_map>

namespace pendarlab::sdk::mavlink_hub
{
  class Agent
  {
  public:
    Agent();
    virtual ~Agent();
    Agent(Agent&&) noexcept;
    Agent& operator=(Agent&&) noexcept;

    bool start();
    bool stop();
    bool configure(const AgentConfig& config);
    AgentState getState() const;

  protected:
    virtual bool doStart() = 0;
    virtual bool doStop() = 0;
    virtual bool doConfigure(const AgentConfig& config) = 0;

  private:
    struct AgentImpl;
    std::unique_ptr<AgentImpl> d;
  };
} // namespace pendarlab::sdk::mavlink_hub
