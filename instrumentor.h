#ifndef INSTRUMENTOR_H_
#define INSTRUMENTOR_H_

#include <chrono>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>

struct ProfileResult {
  std::string name;
  long long start;
  long long end;
  unsigned long long thread_id;
};

// A profiler to write executaion info into given json file.
class Instrumentor {
public:
  // Delete copy constructor
  Instrumentor(const Instrumentor &) = delete;

  // End the session before destructing object.
  ~Instrumentor() { GetInstance().EndSessionImpl(); }

  // Start to record timestamps into json file and output to given path.
  // This function will not create directory automatically.
  // Load the output json file via `chrome://tracing` to observe the results.
  static inline void BeginSession(const std::string &session_name,
                                  const std::string &output_dir = "./") {
    GetInstance().BeginSessionImpl(session_name, output_dir);
  }

  static inline void EndSession() { GetInstance().EndSessionImpl(); }

  static inline void WriteProfile(const ProfileResult &result) {
    GetInstance().WriteProfileImpl(result);
  }

private:
  Instrumentor() : session_name_(""), profile_count_(0), is_active_(false) {}

  static inline Instrumentor &GetInstance() {
    static Instrumentor instance;
    return instance;
  }

  void BeginSessionImpl(const std::string &session_name,
                        const std::string &output_dir) {
    if (is_active_) {
      // Stop last session and restart.
      EndSessionImpl();
    }
    session_name_ = session_name;
    is_active_ = true;

    output_stream_.open(output_dir + session_name_ + ".json");
    WriteHeader();
  }

  void EndSessionImpl() {
    if (!is_active_) {
      return;
    }
    is_active_ = false;
    profile_count_ = 0;
    WriteFooter();
    output_stream_.close();
  }

  void WriteHeader() {
    output_stream_ << "{ \"otherData\": {}, \"traceEvents\": [";
    output_stream_.flush();
  }

  void WriteFooter() {
    output_stream_ << "]}";
    output_stream_.flush();
  }

  void WriteProfileImpl(const ProfileResult &result) {
    std::lock_guard<std::mutex> lock_guard(lock_);
    if (profile_count_++ > 0) {
      output_stream_ << ", ";
    }

    std::string name = result.name;
    std::replace(name.begin(), name.end(), '"', '\'');

    output_stream_ << "{";
    output_stream_ << "\"cat\": \"function\", ";
    output_stream_ << "\"dur\": " << (result.end - result.start) << ", ";
    output_stream_ << "\"name\": \"" << name << "\", ";
    output_stream_ << "\"ph\": \"X\", ";
    output_stream_ << "\"pid\": \"" << session_name_ << "\", ";
    output_stream_ << "\"tid\": " << result.thread_id << ", ";
    output_stream_ << "\"ts\": " << result.start;
    output_stream_ << "}";
    output_stream_.flush();
  }

  std::string session_name_;
  int profile_count_;
  bool is_active_;
  std::ofstream output_stream_;
  std::mutex lock_;
};

class InstrumentorTimer {
public:
  explicit InstrumentorTimer(const char *name) : name_(name), is_stop_(false) {
    start_ = std::chrono::steady_clock::now();
  }

  ~InstrumentorTimer() {
    if (!is_stop_) {
      Stop();
    }
  }

  void Stop() {
    auto end_ = std::chrono::steady_clock::now();

    long long start =
        std::chrono::time_point_cast<std::chrono::microseconds>(start_)
            .time_since_epoch()
            .count();

    long long end =
        std::chrono::time_point_cast<std::chrono::microseconds>(end_)
            .time_since_epoch()
            .count();

    // Hashes the raw thread ID for cross-platform and consistency?
    unsigned long long thread_id =
        std::hash<std::thread::id>{}(std::this_thread::get_id());

    Instrumentor::WriteProfile({name_, start, end, thread_id});
    is_stop_ = true;
  }

private:
  const char *name_;
  bool is_stop_;
  std::chrono::time_point<std::chrono::steady_clock> start_;
};

#endif // INSTRUMENTOR_H_

// Some useful macros.
#if defined(PROFILING)
#define PROFILE_SCOPE(name) InstrumentorTimer timer##__LINE__(name)

#if defined(_MSC_VER)
#define PROFILE_FUNC() PROFILE_SCOPE(__FUNCSIG__)
#else
#define PROFILE_FUNC() PROFILE_SCOPE(__PRETTY_FUNCTION__)
#endif //_MSC_VER

#else
#define PROFILE_SCOPE(name)
#define PROFILE_FUNC()
#endif // PROFILING