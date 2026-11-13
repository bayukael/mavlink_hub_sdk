# mavlink_hub_sdk

`mavlink_hub_sdk` is the SDK for [mavlink_hub](https://github.com/bayukael/mavlink_hub). It defines the **Agent interface** that the hub programs against, plus the supporting types a plugin author needs to implement their own Agent. All public types live in the `pendarlab::sdk::mavlink_hub` namespace.

## What it is / what it is for

`mavlink_hub` is an application that acts as a hub for communicating with multiple MAVLink endpoints through a set of runtime-loaded *agents*. `mavlink_hub_sdk` sits in the middle of that design: it is the contract that lets the hub talk to any agent, and that lets any plugin author implement an agent the hub will understand.

The SDK has **two audiences**, and it serves both through the same types:

- **`mavlink_hub` (the consumer).** The hub loads agent plugins at runtime and drives them through the SDK's `Agent` / `AgentDefinition` interfaces. It hands each created agent an `IManagerResourceRequester` (so the agent can ask for a MAVLink endpoint) and, in return, receives an `IMavlinkEndpointUser` handle to talk to.
- **Plugin authors (implementers).** A plugin author implements their own Agent by subclassing the SDK's `Agent` and `AgentDefinition` types and shipping them as a plugin library that `mavlink_hub` loads (see `docs/ARCHITECTURE.md` for a step-by-step guide). The existing plugins [mavlink_hub_gz_agent](https://github.com/bayukael/mavlink_hub_gz_agent) and [mavlink_hub_heartbeat_generator](https://github.com/bayukael/mavlink_hub_heartbeat_generator) are examples of exactly this.

Because both audiences build against the same headers, the SDK is the single point of compatibility between the hub and its plugins: as long as the SDK ABI/API is stable, any agent written against it works with any hub state that links it.

## What the SDK provides

The SDK publishes the Agent interface and its supporting types:

| Type | Header | Role |
| --- | --- | --- |
| `Agent` | `mavlink_hub_sdk/agent/Agent.h` | The lifecycle of one agent instance: `start()` / `stop()` / `configure()` / `getState()`, plus a `report()`. Implementers override the protected `doStart` / `doStop` / `doConfigure` hooks. |
| `AgentConfig` | `mavlink_hub_sdk/agent/AgentConfig.h` | Base class an agent's concrete config type derives from. |
| `AgentDefinition` | `mavlink_hub_sdk/agent/AgentDefinition.h` | The plugin factory: `parseConfig()` turns raw string options into a config, `create()` builds an `Agent` from it. |
| `AgentState` | `mavlink_hub_sdk/agent/AgentState.h` | `enum class` describing an agent's runtime state (`IDLE`, `STARTING`, `ACTIVE`, `STOPPING`). |
| `AgentConfigParseResult` | `mavlink_hub_sdk/common/AgentConfigParseResult.h` | Output of `parseConfig()`: the parsed `AgentConfig` plus any messages. |
| `IManagerResourceRequester` | `mavlink_hub_sdk/manager_resource_requester/IManagerResourceRequester.h` | What the hub injects into an agent so it can request resources (MAVLink endpoints) from the Manager. |
| `IMavlinkEndpointUser` | `mavlink_hub_sdk/mavlink_endpoint_user/IMavlinkEndpointUser.h` | The endpoint handle an agent receives; it can listen for and write MAVLink messages. |

For the full description of each type and how to implement and register an Agent, see [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md).

## Quick start (build)

Requirements:

- CMake >= 3.13 and a C++17 compiler.
- The SDK's dependency `PendarlabMavlinkEndpoint` (`mavlink_endpoint`) installed and discoverable by CMake. The SDK's CMake config pulls in its transitive dependencies, but it cannot resolve them by itself, so you must point `CMAKE_PREFIX_PATH` at the install prefix(es) of the SDK's dependencies (e.g. `mavlink_endpoint`, and transitively `byte_transport`).
- GoogleTest (`find_package(GTest CONFIG REQUIRED)`) — needed because the build always defines the `MavlinkHubSdkAgentReportTest` test executable.

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_PREFIX_PATH="<prefix(es) of mavlink_endpoint and its dependencies>"
cmake --build build
```

Build artifacts (in `build/`):

- `libmavlink_hub_sdk.so` — the SDK shared library.
- `mavlink_hub_sdk_header` — the `header_check` smoke test (compiles every public header).
- `mavlink_hub_sdk_agent_report_test` — GoogleTest runner for the `report()` behaviour.

Run the checks:

```bash
./build/mavlink_hub_sdk_header         # header_check: compiles the public headers
./build/mavlink_hub_sdk_agent_report_test   # GoogleTest runner
```

There are no lint/format targets; building the `header_check` executable and running the tests is the only verification.

## Install

Install the SDK into a prefix:

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_PREFIX_PATH="<prefix(es) of mavlink_endpoint and its dependencies>" \
  -DCMAKE_INSTALL_PREFIX=<prefix>
cmake --build build
cmake --install build
```

This installs with this layout:

```
<prefix>/
  include/mavlink_hub_sdk/
    agent/Agent.h
    agent/AgentConfig.h
    agent/AgentDefinition.h
    agent/AgentState.h
    common/AgentConfigParseResult.h
    manager_resource_requester/IManagerResourceRequester.h
    mavlink_endpoint_user/IMavlinkEndpointUser.h
    Version.h
  lib/libmavlink_hub_sdk.so
  lib/cmake/PendarlabMavlinkHubSdk   # CMake package config
```

## Using the SDK from a plugin

The SDK installs as the CMake package `PendarlabMavlinkHubSdk` (exported target `pendarlab::MavlinkHubSdk`). A plugin points `CMAKE_PREFIX_PATH` at the SDK's install prefix (and at the SDK's own dependencies), then requests it from `find_package`:

```bash
cmake -S my_plugin -B build -DCMAKE_PREFIX_PATH="<sdk-prefix>;<sdk-dependency-prefixes>"
```

```cmake
find_package(PendarlabMavlinkHubSdk 1.0.0 REQUIRED)

add_library(my_agent_plugin SHARED ...)
target_link_libraries(my_agent_plugin PRIVATE pendarlab::MavlinkHubSdk)
```

Because the SDK's CMake config `find_dependency`-pulls `PendarlabMavlinkEndpoint`, the endpoint (and its transitive `byte_transport`) prefixes must also be reachable via `CMAKE_PREFIX_PATH` at configure time.

Example usage from a plugin source file:

```cpp
#include <mavlink_hub_sdk/agent/Agent.h>
#include <mavlink_hub_sdk/agent/AgentDefinition.h>
#include <mavlink_hub_sdk/manager_resource_requester/IManagerResourceRequester.h>

namespace pendarlab::app::mavlink_hub::plugin
{
  using namespace pendarlab::sdk::mavlink_hub;

  class MyAgent : public Agent
  {
  protected:
    bool doStart() override { return true; }
    bool doStop() override { return true; }
    bool doConfigure(const AgentConfig&) override { return true; }
  };

  class MyAgentDefinition : public AgentDefinition
  {
  public:
    AgentConfigParseResult parseConfig(const std::unordered_map<std::string, std::string>&) const override;
    std::unique_ptr<Agent> create(const AgentConfig&, std::unique_ptr<IManagerResourceRequester>) const override;
  };
}

// The factory entry point mavlink_hub dlsym's to load this plugin.
extern "C" const pendarlab::app::mavlink_hub::plugin::MyAgentDefinition* getMyAgentDefinition();
```

See [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) for the full plugin-authoring guide, including how `mavlink_hub` loads and registers the plugin.

## Repository status

This repository currently ships the SDK code and its packaging only: `include/`, `src/`, `test/`, and the CMake build. This README and `docs/ARCHITECTURE.md` are the repository's documentation.
