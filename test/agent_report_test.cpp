#include <gtest/gtest.h>

#include <mavlink_hub_sdk/agent/Agent.h>

#include <memory>
#include <string>

using pendarlab::sdk::mavlink_hub::Agent;
using pendarlab::sdk::mavlink_hub::AgentConfig;

namespace
{
  // Minimal agent that does NOT override report(); used to verify the default,
  // backward-compatible implementation.
  class DefaultReportAgent : public Agent
  {
  public:
    DefaultReportAgent() = default;
    ~DefaultReportAgent() override = default;

  protected:
    bool doStart() override { return true; }
    bool doStop() override { return true; }
    bool doConfigure(const AgentConfig&) override { return true; }
  };

  // Agent that overrides report() to return its own string.
  class CustomReportAgent : public DefaultReportAgent
  {
  public:
    CustomReportAgent() = default;
    ~CustomReportAgent() override = default;

    std::string report() const override { return "custom-agent-report-42"; }
  };
} // namespace

TEST(AgentReport, DefaultIndicatesNotImplemented)
{
  DefaultReportAgent agent;
  EXPECT_EQ(agent.report(), "agent does not implement a report");
}

TEST(AgentReport, OverrideReturnsItsOwnReport)
{
  CustomReportAgent agent;
  EXPECT_EQ(agent.report(), "custom-agent-report-42");
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
