// Backward-compatibility compile & link check.
//
// Mirrors how an existing agent (e.g. mavlink_hub_gz_agent's GzAgent) subclasses
// the SDK Agent base: it overrides only the lifecycle virtuals (doStart/doStop/
// doConfigure), declares move semantics and a pimpl, and does NOT touch the new
// report() method. This translation unit compiles and links as an executable
// against the SDK, demonstrating that existing agents build unchanged against
// the new base (acceptance criterion: "existing agents compile unchanged").
// The real gazebo agent cannot be built here (gz-msgs/gz-transport/sdformat are
// not installed), so this faithful stand-in reproduces its subclass shape and
// verifies the backward-compatible interface.

#include <mavlink_hub_sdk/agent/Agent.h>
#include <mavlink_hub_sdk/agent/AgentConfig.h>
#include <mavlink_hub_sdk/manager_resource_requester/IManagerResourceRequester.h>

#include <memory>

namespace pendarlab::sdk::mavlink_hub
{
  class StandInGzAgent : public Agent
  {
  public:
    StandInGzAgent(std::unique_ptr<IManagerResourceRequester> requester);
    virtual ~StandInGzAgent();
    StandInGzAgent(StandInGzAgent&&) noexcept;
    StandInGzAgent& operator=(StandInGzAgent&&) noexcept;

  protected:
    virtual bool doStart() override;
    virtual bool doStop() override;
    virtual bool doConfigure(const AgentConfig& config) override;

  private:
    struct StandInGzAgentImpl;
    std::unique_ptr<StandInGzAgentImpl> d;
  };

  struct StandInGzAgent::StandInGzAgentImpl {};

  StandInGzAgent::StandInGzAgent(std::unique_ptr<IManagerResourceRequester>)
    : d(std::make_unique<StandInGzAgentImpl>())
  {
  }
  StandInGzAgent::~StandInGzAgent() = default;
  StandInGzAgent::StandInGzAgent(StandInGzAgent&&) noexcept = default;
  StandInGzAgent& StandInGzAgent::operator=(StandInGzAgent&&) noexcept = default;

  bool StandInGzAgent::doStart() { return true; }
  bool StandInGzAgent::doStop() { return true; }
  bool StandInGzAgent::doConfigure(const AgentConfig&) { return true; }
} // namespace pendarlab::sdk::mavlink_hub

int main()
{
  using pendarlab::sdk::mavlink_hub::StandInGzAgent;
  // Constructing via an existing (unchanged) agent shape exercises the Agent
  // base constructor/lifecycle and links against the SDK library.
  StandInGzAgent agent(nullptr);
  return 0;
}
