#include "mavlink_hub_sdk/agent/Agent.h"

#include "mavlink_hub_sdk/agent/AgentState.h"

#include <mutex>
#include <memory>

namespace pendarlab::sdk::mavlink_hub
{
  struct Agent::AgentImpl {
    std::mutex mtx_;
    AgentState state_;
  };

  Agent::Agent() : d(std::make_unique<AgentImpl>())
  {
  }
  Agent::~Agent()
  {
  }
  Agent::Agent(Agent&&) noexcept = default;
  Agent& Agent::operator=(Agent&&) noexcept = default;

  bool Agent::start()
  {
    AgentState state;
    {
      std::lock_guard lock(d->mtx_);
      state = d->state_;
    }

    if (state != AgentState::IDLE) {
      return false;
    }

    {
      std::lock_guard lock(d->mtx_);
      d->state_ = AgentState::STARTING;
    }

    if (!doStart()) {
      {
        std::lock_guard lock(d->mtx_);
        d->state_ = AgentState::IDLE;
        return false;
      }
    }

    {
      std::lock_guard lock(d->mtx_);
      d->state_ = AgentState::ACTIVE;
    }
    return true;
  }

  bool Agent::stop()
  {
    AgentState state;
    {
      std::lock_guard lock(d->mtx_);
      state = d->state_;
    }

    if (state == AgentState::STARTING) {
      return false;
    }

    if (state == AgentState::ACTIVE) {
      {
        std::lock_guard lock(d->mtx_);
        d->state_ = AgentState::STOPPING;
      }
      doStop();
      {
        std::lock_guard lock(d->mtx_);
        d->state_ = AgentState::IDLE;
      }
    }

    return true;
  }

  bool Agent::configure(const AgentConfig& config)
  {
    AgentState state;
    {
      std::lock_guard lock(d->mtx_);
      state = d->state_;
    }

    if (state != AgentState::IDLE) {
      return false;
    }

    return doConfigure(config);
  }

  AgentState Agent::getState() const
  {
    return d->state_;
  }
} // namespace pendarlab::sdk::mavlink_hub