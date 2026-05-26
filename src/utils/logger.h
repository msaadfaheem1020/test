#pragma once

#include <memory>
#include <spdlog/spdlog.h>

class Logger {
public:
  static void init();
  static std::shared_ptr<spdlog::logger> &get_logger();

private:
  static std::shared_ptr<spdlog::logger> s_Logger;
};

#define LOG_TRACE(...) ::Logger::get_logger()->trace(__VA_ARGS__)
#define LOG_INFO(...) ::Logger::get_logger()->info(__VA_ARGS__)
#define LOG_WARN(...) ::Logger::get_logger()->warn(__VA_ARGS__)
#define LOG_ERROR(...) ::Logger::get_logger()->error(__VA_ARGS__)
#define LOG_CRITICAL(...) ::Logger::get_logger()->critical(__VA_ARGS__)