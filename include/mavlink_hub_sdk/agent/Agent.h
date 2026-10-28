#pragma once

#include "mavlink_hub_sdk/agent/AgentConfig.h"
#include "mavlink_hub_sdk/agent/AgentState.h"
#include "mavlink_hub_sdk/manager_resource_requester/IManagerResourceRequester.h"

#include <memory>
#include <string>
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
    /// Returns the agent's report as an opaque string. The format is chosen by
    /// the agent. The default implementation returns an indication that the
    /// agent does not implement a report, so existing agents keep compiling and
    /// running unchanged until they override it.
    virtual std::string report() const;

  protected:
    virtual bool doStart() = 0;
    virtual bool doStop() = 0;
    virtual bool doConfigure(const AgentConfig& config) = 0;

  private:
    struct AgentImpl;
    std::unique_ptr<AgentImpl> d;
  };
} // namespace pendarlab::sdk::mavlink_hub
