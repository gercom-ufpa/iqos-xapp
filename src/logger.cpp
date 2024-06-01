#include "logger.hpp"

#include "spdlog/sinks/stdout_color_sinks-inl.h"

void configureLogger(const std::string& xAppName, spdlog::level::level_enum logLevel)
{
    auto console {spdlog::stdout_color_mt(xAppName)};
    console->set_pattern("[%Y-%m-%d %H:%M:%S][" + xAppName + "]%^[%l]%$[%s:%#] %v");
    console->set_level(logLevel);
    spdlog::set_default_logger(console);
}