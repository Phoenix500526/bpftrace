#pragma once

#include <cassert>
#include <iostream>
#include <optional>
#include <sstream>
#include <unordered_map>

#include "location.hh"

namespace bpftrace {

// clang-format off
enum class LogType
{
  DEBUG,
  V1,
  HINT,
  WARNING,
  ERROR,
  BUG,
};
// clang-format on

class Log {
public:
  static Log& get();
  void take_input(LogType type,
                  const std::optional<location>& loc,
                  std::ostream& out,
                  std::string&& input);
  inline void set_source(std::string_view filename, std::string_view source)
  {
    src_ = source;
    filename_ = filename;
  }
  inline const std::string& get_source()
  {
    return src_;
  }
  const std::string get_source_line(unsigned int n);

  // Can only construct with get()
  Log(const Log& other) = delete;
  Log& operator=(const Log& other) = delete;

  inline void enable(LogType type)
  {
    enabled_map_[type] = true;
  }
  inline void disable(LogType type)
  {
    assert(type != LogType::BUG && type != LogType::ERROR);
    enabled_map_[type] = false;
  }
  inline bool is_enabled(LogType type)
  {
    return enabled_map_[type];
  }

private:
  Log();
  ~Log() = default;
  std::string src_;
  std::string filename_;
  void log_with_location(LogType,
                         const location&,
                         std::ostream&,
                         const std::string&);
  std::unordered_map<LogType, bool> enabled_map_;
};

class LogStream {
public:
  LogStream(const std::string& file,
            int line,
            LogType type,
            std::ostream& out = std::cerr);
  LogStream(const std::string& file,
            int line,
            LogType type,
            std::optional<location> loc,
            std::ostream& out = std::cerr);
  template <typename T>
  LogStream& operator<<(const T& v)
  {
    if (sink_.is_enabled(type_))
      buf_ << v;
    return *this;
  }
  virtual ~LogStream();

protected:
  std::string internal_location();

  Log& sink_;
  LogType type_;
  const std::optional<location> loc_;
  std::ostream& out_;
  std::string log_file_;
  int log_line_;
  std::ostringstream buf_;
};

class LogStreamBug : public LogStream {
public:
  LogStreamBug(const std::string& file,
               int line,
               __attribute__((unused)) LogType,
               std::ostream& out = std::cerr)
      : LogStream(file, line, LogType::BUG, out) {};
  LogStreamBug(const std::string& file,
               int line,
               __attribute__((unused)) LogType,
               std::optional<location> loc,
               std::ostream& out = std::cerr)
      : LogStream(file, line, LogType::BUG, loc, out) {};
  [[noreturn]] ~LogStreamBug();
};

// Usage examples:
// 1. LOG(WARNING) << "this is a " << "warning!"; (this goes to std::cerr)
// 2. LOG(DEBUG, std::cout) << "this is a " << " message.";
// 3. LOG(ERROR, call.loc, std::cerr) << "this is a semantic error";
// Note: LogType::DEBUG will prepend __FILE__ and __LINE__ to the debug message

// clang-format off
#define LOGSTREAM_COMMON(...) bpftrace::LogStream(__FILE__, __LINE__, __VA_ARGS__)
#define LOGSTREAM_DEBUG(...) LOGSTREAM_COMMON(__VA_ARGS__)
#define LOGSTREAM_V1(...) LOGSTREAM_COMMON(__VA_ARGS__)
#define LOGSTREAM_HINT(...) LOGSTREAM_COMMON(__VA_ARGS__)
#define LOGSTREAM_WARNING(...) LOGSTREAM_COMMON(__VA_ARGS__)
#define LOGSTREAM_ERROR(...) LOGSTREAM_COMMON(__VA_ARGS__)
#define LOGSTREAM_BUG(...) bpftrace::LogStreamBug(__FILE__, __LINE__, __VA_ARGS__)
// clang-format on

#define LOG(type, ...) LOGSTREAM_##type(bpftrace::LogType::type, ##__VA_ARGS__)

#define DISABLE_LOG(type) bpftrace::Log::get().disable(LogType::type)
#define ENABLE_LOG(type) bpftrace::Log::get().enable(LogType::type)

}; // namespace bpftrace
