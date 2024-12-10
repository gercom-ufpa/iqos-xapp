//
// Created by murilo on 31/05/24.
//

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>

#include "spdlog/spdlog.h"

void configureLogger(const std::string& xAppName, spdlog::level::level_enum logLevel);

#endif //LOGGER_HPP