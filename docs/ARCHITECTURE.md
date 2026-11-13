# mavlink_hub_sdk — Architecture and Plugin Authoring Guide

This document describes the public surface of **mavlink_hub_sdk** (Pendarlab), the SDK behind
[mavlink_hub](https://github.com/bayukael/mavlink_hub). It is written for both SDK audiences:

- **Consumers** — `mavlink_hub` (and any other code) that programs against the SDK's Agent
  interface.
- **Implementers / plugin authors** — developers who implement their own Agent plugin that
  `mavlink_hub` loads at runtime.

All public types live in the **`pendarlab::sdk::mavlink_hub`** namespace. The repository is
C++17 and installs as the CMake package `PendarlabMavlinkHubSdk` (target
`pendarlab::MavlinkHubSdk`), which in turn depends on `pendarlab::MavlinkEndpoint` from
`mavlink_endpoint`.

---

## 1. Where the SDK fits

`mavlink_hub_sdk` is the contract between `mavlink_hub` and the agents it loads. The hub is
MAVLink-centric: every endpoint it owns is a MAVLink endpoint, and agents are how the hub
reaches systems that do not natively speak MAVLink (simulators, onboard compute, middleware,
custom hardware).

At runtime:

1. `mavlink_hub` loads an agent plugin as a shared library and looks up its
   `AgentDefinition` factory by a symbol name (see §4).
2. When the hub wants an agent instance, it calls `AgentDefinition::parseConfig()` to validate
   the user-supplied config and `AgentDefinition::create()` to build an `Agent`.
3. The hub injects an `IManagerResourceRequester` into the agent. Through it the agent can ask
   the Manager for resources — such as a MAVLink endpoint — and receive an `IMavlinkEndpointUser`
   handle it can listen on and write to.
4. The hub drives the agent's lifecycle through `Agent::start()` / `stop()` / `configure()` /
   `getState()`.

Both consumer and implementer build against the same headers, so the SDK is the single
compatibility point between the hub and its plugins.

---

## 2. Public SDK surface

The SDK publishes seven types, grouped by directory under `include/mavlink_hub_sdk/`.

### 2.1 `Agent` — `agent/Agent.h`

The lifecycle of a single agent instance. An `Agent` is a concrete base you derive from; it
owns mutable state (state machine + mutex) via the PIMPL idiom and delegates the actual work to
three protected hooks you override.

**Public API:**

- `bool start()` — transitions the agent from `IDLE` to `STARTING`, calls `doStart()`, and, on
  success, to `ACTIVE`. Returns `false` if the agent is not `IDLE`, or if `doStart()` returns
  `false` (in which case it falls back to `IDLE`).
- `bool stop()` — if the agent is `ACTIVE`, transitions it to `STOPPING`, calls `doStop()`, and
  returns to `IDLE`. Returns `false` if the agent is `STARTING`. Stopping an `IDLE` agent is a
  no-op that returns `true`.
- `bool configure(const AgentConfig&)` — applies a config. Only allowed while the agent is
  `IDLE`; delegates to `doConfigure()`. Returns `false` otherwise.
- `AgentState getState() const` — the current `AgentState`.
- `virtual std::string report() const` — returns an opaque report string whose format is chosen
  by the agent. The default implementation returns an indication that the agent does not
  implement a report, so existing agents keep compiling and running unchanged until they
  override it.

**Hooks you override (protected):**

- `virtual bool doStart() = 0` — perform one-time startup (e.g. request resources, spawn
  threads). Return `false` to signal failure.
- `virtual bool doStop() = 0` — tear down what `doStart()` set up.
- `virtual bool doConfigure(const AgentConfig&) = 0` — apply an `AgentConfig`.

The class is movable: `Agent(Agent&&)` and `operator=(Agent&&)` are provided (the impl is a
`std::unique_ptr`).

### 2.2 `AgentConfig` — `agent/AgentConfig.h`

The base type for an agent's configuration. It has a virtual destructor and is otherwise empty;
a concrete agent derives its own config struct/class from it (e.g. `HeartbeatAgentConfig` in
`mavlink_hub_heartbeat_generator`) to add its specific fields.

### 2.3 `AgentDefinition` — `agent/AgentDefinition.h`

The plugin factory. An implementer ships one definition per agent type; `mavlink_hub` uses it
to validate configs and create instances.

- `virtual AgentConfigParseResult parseConfig(const std::unordered_map<std::string, std::string>& cfg) const = 0`
  — take the raw string key/value options the hub provides for an agent and validate them into
  a concrete `AgentConfig`. Return the parsed config on success, or error messages otherwise.
  Invalid values must be reported as messages and must not produce a parsed config.
- `virtual std::unique_ptr<Agent> create(const AgentConfig& cfg, std::unique_ptr<IManagerResourceRequester> requester) const = 0`
  — build an `Agent` from a config this definition previously parsed, handing it the resource
  requester it will use to obtain endpoints. Typically `dynamic_cast`s `cfg` to the concrete
  config type and returns `nullptr` if the type does not match.

### 2.4 `AgentState` — `agent/AgentState.h`

```cpp
enum class AgentState {
  IDLE,      // not started (or stopped); the only state where configure() is allowed
  STARTING,  // start() in progress (doStart() running)
  ACTIVE,    // running
  STOPPING   // stop() in progress (doStop() running)
};
```

### 2.5 `AgentConfigParseResult` — `common/AgentConfigParseResult.h`

The result of `AgentDefinition::parseConfig()`:

```cpp
struct AgentConfigParseResult {
  std::unique_ptr<AgentConfig> parsed;   // the parsed config, or nullptr on failure
  std::vector<std::string> messages;     // human-readable messages (errors/warnings)
  bool ok() const { return parsed != nullptr; }
};
```

### 2.6 `IManagerResourceRequester` — `manager_resource_requester/IManagerResourceRequester.h`

What the hub injects into every agent so it can ask the Manager for resources **without holding
a concrete Manager reference**. Implemented by the hub runtime and handed to the agent at
`AgentDefinition::create()` time.

- `virtual std::unique_ptr<IMavlinkEndpointUser> requestMavlinkEndpoint(const std::string& ep_name) = 0`
  — request a MAVLink endpoint by name, returning a handle to it (or `nullptr` if unavailable).
- `virtual std::vector<std::string> getMavlinkEndpointList() = 0` — the names of all currently
  known (registered) MAVLink endpoints. The list is *not* filtered by connection state, so
  disconnected endpoints may still appear; an agent can treat it as the complete set of
  endpoints it may target (e.g. for "all except" / "every endpoint" targeting).

### 2.7 `IMavlinkEndpointUser` — `mavlink_endpoint_user/IMavlinkEndpointUser.h`

The safe endpoint handle an agent receives after requesting an endpoint. It exposes only what
an agent needs to talk and listen, and uses the `mavlink_endpoint` types
(`MavlinkEndpointToken`, `MavlinkEndpointState`, `MavlinkEndpointPacket`).

- `virtual std::unique_ptr<MavlinkEndpointToken> createListener(std::function<void(const MavlinkEndpointPacket&)> listener_cb) = 0`
  — subscribe to incoming MAVLink packets; the returned token represents (and can cancel) the
  subscription.
- `virtual std::optional<int> getNumOfListener() = 0` — number of current listeners.
- `virtual std::optional<int> writeMessage(const mavlink_message_t& msg) = 0` — send a MAVLink
  message.
- `virtual std::optional<MavlinkEndpointState> getState() = 0` — the endpoint's connection state.

---

## 3. Build & dependency layout

The SDK builds as a single SHARED library (`MavlinkHubSdk`, output `mavlink_hub_sdk`) plus two
verification executables. It depends on `pendarlab::MavlinkEndpoint` (from `mavlink_endpoint`,
which itself depends on `byte_transport`).

| CMake target | Output | Links |
| --- | --- | --- |
| `MavlinkHubSdk` (alias `pendarlab::MavlinkHubSdk`) | `libmavlink_hub_sdk.so` | `pendarlab::MavlinkEndpoint` |
| `MavlinkHubSdkHeader` | `mavlink_hub_sdk_header` | `pendarlab::MavlinkEndpoint` |
| `MavlinkHubSdkAgentReportTest` | `mavlink_hub_sdk_agent_report_test` | `MavlinkHubSdk`, GTest |

Dependencies are found via `CMAKE_PREFIX_PATH`. The SDK's install-time CMake config performs a
`find_dependency(PendarlabMavlinkEndpoint ...)`, but it cannot resolve that package on its own;
consumers must also expose `mavlink_endpoint` (and `byte_transport`) via `CMAKE_PREFIX_PATH`.
The equivalent holds when building the SDK itself (`find_package(PendarlabMavlinkEndpoint
1.0.0 REQUIRED)` at configure time).

The SDK also generates a `Version.h` from `Version.h.in`, exposing the constants
`pendarlab::sdk::mavlink_hub::versionMajor`, `versionMinor`, `versionPatch`, and `version`
(a string).

### Installation layout

Installing the SDK into a prefix produces:

```
<prefix>/
  include/mavlink_hub_sdk/...            # all public headers + generated Version.h
  lib/libmavlink_hub_sdk.so
  lib/cmake/PendarlabMavlinkHubSdk/      # package config (find_package(PendarlabMavlinkHubSdk))
```

### Consuming from a plugin

```cmake
find_package(PendarlabMavlinkHubSdk 1.0.0 REQUIRED)
target_link_libraries(my_plugin PRIVATE pendarlab::MavlinkHubSdk)
```

with `CMAKE_PREFIX_PATH` set to the SDK prefix and the prefixes of its transitive dependencies.
See the repository `README.md` for a full example.

---

## 4. How a plugin author implements and registers an Agent

This is the implementer-facing guide. It follows how the existing plugins
(`mavlink_hub_gz_agent`, `mavlink_hub_heartbeat_generator`) are structured, so a new plugin
stays consistent with the ecosystem.

### 4.1 Outline

- **Config type.** Derive a concrete config from `AgentConfig` to hold the plugin's parsed
  settings.
- **Agent type.** Derive from `Agent`. Store the config and the resource requester (and any
  runtime objects) in a PIMPL `struct` held by a `std::unique_ptr`. Override `doStart`,
  `doStop`, `doConfigure`; optionally override `report()`.
- **Definition (factory) type.** Derive from `AgentDefinition`. Implement `parseConfig()` to
  validate the string options into the config type, and `create()` to build an `Agent` and hand
  it the injected requester.
- **Factory symbol.** Expose an `extern "C"` function returning a pointer to a singleton
  definition. `mavlink_hub` dlsym's this function by name to load the plugin.

### 4.2 Step by step

**1. Build against the SDK.**

```cmake
find_package(PendarlabMavlinkHubSdk 1.0.0 REQUIRED)
add_library(my_agent SHARED ...)
target_link_libraries(my_agent PRIVATE pendarlab::MavlinkHubSdk)
```

Configure with `CMAKE_PREFIX_PATH` pointing at the SDK and its transitive dependencies.

**2. Define a concrete config** (`MyAgentConfig.h`):

```cpp
#include <mavlink_hub_sdk/agent/AgentConfig.h>

namespace pendarlab::app::mavlink_hub::plugin
{
  using namespace pendarlab::sdk::mavlink_hub;

  class MyAgentConfig : public AgentConfig
  {
  public:
    std::string target_name;   // example of a plugin-specific field
    double some_rate = 1.0;
  };
} // namespace pendarlab::app::mavlink_hub::plugin
```

**3. Define the agent** (`MyAgent.h` / `MyAgent.cpp`). Hold the config and the injected
requester, ask for an endpoint in `doStart()` via `IManagerResourceRequester`, and talk through
the resulting `IMavlinkEndpointUser`:

```cpp
#include <mavlink_hub_sdk/agent/Agent.h>
#include <mavlink_hub_sdk/manager_resource_requester/IManagerResourceRequester.h>
#include "MyAgentConfig.h"

namespace pendarlab::app::mavlink_hub::plugin
{
  using namespace pendarlab::sdk::mavlink_hub;

  class MyAgent : public Agent
  {
  public:
    MyAgent(const MyAgentConfig& cfg, std::unique_ptr<IManagerResourceRequester> requester)
      : d(std::make_unique<Impl>())
    {
      d->config = cfg;
      d->requester = std::move(requester);
    }
    ~MyAgent() override = default;
    MyAgent(MyAgent&&) noexcept = default;
    MyAgent& operator=(MyAgent&&) noexcept = default;

  protected:
    bool doStart() override
    {
      d->endpoint = d->requester->requestMavlinkEndpoint(d->config.target_name);
      return d->endpoint != nullptr;
    }
    bool doStop() override { return true; }
    bool doConfigure(const AgentConfig&) override { return true; }

  private:
    struct Impl {
      MyAgentConfig config;
      std::unique_ptr<IManagerResourceRequester> requester;
      std::unique_ptr<IMavlinkEndpointUser> endpoint;
    };
    std::unique_ptr<Impl> d;
  };
} // namespace pendarlab::app::mavlink_hub::plugin
```

**4. Define the definition (factory)** (`MyAgentDefinition.h` / `.cpp`). `parseConfig()` returns
a `MyAgentConfig` on success (and only populates `messages`/`parsed` appropriately), and
`create()` dynamic-casts the config back to `MyAgentConfig` and builds the agent:

```cpp
#include <mavlink_hub_sdk/agent/AgentDefinition.h>
#include <mavlink_hub_sdk/common/AgentConfigParseResult.h>

namespace pendarlab::app::mavlink_hub::plugin
{
  using namespace pendarlab::sdk::mavlink_hub;

  class MyAgentDefinition : public AgentDefinition
  {
  public:
    AgentConfigParseResult parseConfig(const std::unordered_map<std::string, std::string>& cfg) const override
    {
      AgentConfigParseResult r;
      auto c = std::make_unique<MyAgentConfig>();
      // ... read/validate keys from cfg, push messages on error, set hasError ...
      if (!hasError) r.parsed = std::move(c);
      return r;
    }
    std::unique_ptr<Agent> create(const AgentConfig& cfg, std::unique_ptr<IManagerResourceRequester> requester) const override
    {
      const auto* concrete = dynamic_cast<const MyAgentConfig*>(&cfg);
      if (concrete == nullptr) return nullptr;
      return std::make_unique<MyAgent>(*concrete, std::move(requester));
    }
  };
} // namespace pendarlab::app::mavlink_hub::plugin
```

**5. Export the factory symbol** so `mavlink_hub` can `dlopen` and look it up:

```cpp
extern "C" const pendarlab::app::mavlink_hub::plugin::MyAgentDefinition* getMyAgentDefinition()
{
  static pendarlab::app::mavlink_hub::plugin::MyAgentDefinition def;
  return &def;
}
```

> `mavlink_hub` resolves the exported symbol by name. Pick a unique, descriptive function name
> (`get<Something>AgentDefinition`) so multiple plugins can coexist.

### 4.3 Registering the plugin with mavlink_hub

`mavlink_hub` loads agent plugins at runtime using `dlopen`, then looks up the exported factory
symbol and registers the returned `AgentDefinition` under a name. To make the plugin loadable,
provide `mavlink_hub` with:

1. The **path** to the built plugin shared library (e.g. `libmy_agent.so`).
2. The **factory symbol name** (`getMyAgentDefinition` above).
3. A **registration name** used to reference this agent definition when spawning an agent.

The exact mechanism and JSON config format are defined by `mavlink_hub` (see its
`docs/ARCHITECTURE.md` and `README.md`); from the SDK's side the only requirement is the
`extern "C"` factory symbol returning a `pendarlab::sdk::mavlink_hub::AgentDefinition*`.

---

## 5. Tests and verification

The repository ships two verification targets:

- **`mavlink_hub_sdk_header`** (`test/header_check.cpp`) — the header_check smoke test. It
  includes the SDK's public headers (via `AgentDefinition.h`, which pulls in the others) to
  ensure the public surface always compiles. This is the minimal gate that adding public types
  must keep green.
- **`mavlink_hub_sdk_agent_report_test`** (`test/agent_report_test.cpp`) — a GoogleTest runner
  verifying the default (backward-compatible) `Agent::report()` and that an override returns its
  own string.

There is no `ctest` registration; tests are run as executables directly from the build dir.
Building and running them is the only verification.
