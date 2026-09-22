#pragma once

#include <optional>
#include <vector>
#include <string>

namespace pendarlab::sdk::mavlink_hub
{
  template <typename T>
  struct ParseResult{
    std::optional<T> parsed;
    std::vector<std::string> messages;

    bool ok() const {return parsed.has_value();}
  };
} // namespace pendarlab::sdk::mavlink_hub
