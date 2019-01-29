#ifndef GRUUT_ENTERPRISE_MERGER_PERIODIC_TASK_HPP
#define GRUUT_ENTERPRISE_MERGER_PERIODIC_TASK_HPP

#include <atomic>
#include <boost/asio.hpp>
#include <memory>
#include <mutex>

class PeriodicTask : boost::noncopyable {
private:
  std::shared_ptr<boost::asio::io_service> m_io_service;
  std::unique_ptr<boost::asio::deadline_timer> m_event_timer;
  std::unique_ptr<boost::asio::deadline_timer> m_start_timer;
  std::unique_ptr<boost::asio::strand> m_strand;
  std::function<void()> m_task_function;
  uint64_t m_interval{1000};
  std::atomic<bool> m_strand_mode{false};

public:
  PeriodicTask() = default;

  template <typename T = boost::asio::io_service> PeriodicTask(T &&io_service) {
    setIoService(io_service);
  }

  template <typename T = boost::asio::io_service>
  PeriodicTask(T &&io_service, uint64_t intval, int after,
               const std::function<void()> &task_function) {

    setIoService(io_service);
    setTaskFunction(task_function);
    if (intval != 0) {
      setInterval(intval);
    }

    if (after >= 0) {
      runTaskAfter(after);
    }
  }

  PeriodicTask(std::shared_ptr<boost::asio::io_service> io_service,
               uint64_t intval, int after,
               const std::function<void()> &task_function) {

    setIoService(io_service);
    setTaskFunction(task_function);
    if (intval != 0) {
      setInterval(intval);
    }

    if (after >= 0) {
      runTaskAfter(after);
    }
  }

  void setStrandMod() { m_strand_mode = true; }

  template <typename T = boost::asio::io_service>
  void setIoService(T &&io_service) {
    m_io_service = std::shared_ptr<boost::asio::io_service>(&io_service);
    m_event_timer.reset(new boost::asio::deadline_timer(io_service));
    m_strand.reset(new boost::asio::strand(io_service));
    m_start_timer.reset(new boost::asio::deadline_timer(io_service));
  }

  void setIoService(std::shared_ptr<boost::asio::io_service> io_service) {
    m_io_service = io_service;
    m_event_timer.reset(new boost::asio::deadline_timer(*io_service));
    m_strand.reset(new boost::asio::strand(*io_service));
    m_start_timer.reset(new boost::asio::deadline_timer(*io_service));
  }

  void setTaskFunction(const std::function<void()> &task_function) {
    m_task_function = task_function;
  }

  void setInterval(uint64_t intval) { m_interval = intval; }

  void runTask(const std::function<void()> &task_function) {
    setTaskFunction(task_function);
    runTaskAfter(0);
  }

  void runTask(int after, const std::function<void()> &task_function) {
    setTaskFunction(task_function);
    runTaskAfter(after);
  }

  void runTask(const std::function<void()> &task_function, int after) {
    setTaskFunction(task_function);
    runTaskAfter(after);
  }

  void runTask() { runTaskAfter(0); }

  void runTaskAfter(int after) {
    if (!m_task_function) {
      return;
    }

    if (after > 0) {
      m_start_timer->expires_from_now(boost::posix_time::milliseconds(after));
      m_start_timer->async_wait([this](const boost::system::error_code &error) {
        if (!error) {
          eventLoop();
        }
      });
    } else {
      eventLoop();
    }
  }

  void stopTask() { m_event_timer->cancel(); }

private:
  void eventLoop() {
    if (m_task_function) {
      if (m_strand_mode) {
        m_io_service->post(m_strand->wrap([this]() { m_task_function(); }));
      } else {
        m_io_service->post([this]() { m_task_function(); });
      }
    }

    m_event_timer->expires_from_now(
        boost::posix_time::milliseconds(m_interval));
    m_event_timer->async_wait([this](const boost::system::error_code &error) {
      if (!error && error != boost::asio::error::operation_aborted) {
        eventLoop();
      }
    });
  }
};

class DelayedTask : boost::noncopyable {
private:
  std::unique_ptr<boost::asio::deadline_timer> m_event_timer;
  std::function<void()> m_task_function;

public:
  DelayedTask() = default;

  template <typename T = boost::asio::io_service> DelayedTask(T &&io_service) {
    setIoService(io_service);
  }

  template <typename T = boost::asio::io_service>
  DelayedTask(T &&io_service, const std::function<void()> &task_function,
              uint64_t timeout) {
    setIoService(io_service);
    setTaskFunction(task_function);
    runTask(timeout);
  }

  template <typename T = boost::asio::io_service>
  void setIoService(T &&io_service) {
    m_event_timer.reset(new boost::asio::deadline_timer(io_service));
  }

  void setTaskFunction(const std::function<void()> &task_function) {
    m_task_function = task_function;
  }

  void runTask(uint64_t timeout, const std::function<void()> &task_function) {
    setTaskFunction(task_function);
    runTask(timeout);
  }

  void runTask(const std::function<void()> &task_function, uint64_t timeout) {
    setTaskFunction(task_function);
    runTask(timeout);
  }

  void runTask(uint64_t timeout) {
    if (timeout > 0) {
      m_event_timer->expires_from_now(boost::posix_time::milliseconds(timeout));
      m_event_timer->async_wait([this](const boost::system::error_code &error) {
        if (!error) {
          if (m_task_function)
            m_task_function();
        }
      });
    } else {
      if (m_task_function)
        m_task_function();
    }
  }
};

class TaskOnTime : boost::noncopyable {
private:
  std::shared_ptr<boost::asio::io_service> m_io_service;
  std::unique_ptr<boost::asio::deadline_timer> m_event_timer;
  std::unique_ptr<boost::asio::strand> m_strand;
  std::function<void()> m_task_function;
  int m_at_time{1000};
  uint64_t m_intval{10000};
  std::atomic<bool> m_strand_mode{false};

public:
  TaskOnTime() = default;

  template <typename T = boost::asio::io_service> TaskOnTime(T &&io_service) {
    setIoService(io_service);
  }

  template <typename T = boost::asio::io_service>
  TaskOnTime(T &&io_service, int at_time, uint64_t intval,
             const std::function<void()> &task_function) {
    setIoService(io_service);
    setTaskFunction(task_function);
    setTime(at_time, intval);
    runTaskOnTime();
  }

  TaskOnTime(std::shared_ptr<boost::asio::io_service> io_service, int at_time,
             uint64_t intval, const std::function<void()> &task_function) {
    setIoService(io_service);
    setTaskFunction(task_function);
    setTime(at_time, intval);
    runTaskOnTime();
  }

  void setStrandMod() { m_strand_mode = true; }

  template <typename T = boost::asio::io_service>
  void setIoService(T &&io_service) {
    m_io_service = std::shared_ptr<boost::asio::io_service>(&io_service);
    m_event_timer.reset(new boost::asio::deadline_timer(io_service));
    m_strand.reset(new boost::asio::strand(io_service));
  }

  void setIoService(std::shared_ptr<boost::asio::io_service> io_service) {
    m_io_service = io_service;
    m_event_timer.reset(new boost::asio::deadline_timer(*io_service));
    m_strand.reset(new boost::asio::strand(*io_service));
  }

  void setTaskFunction(const std::function<void()> &task_function) {
    m_task_function = task_function;
  }

  void setTime(int at_time, uint64_t intval) {
    if (at_time >= intval) {
      at_time %= intval;
    }

    if (at_time < 0) {
      at_time = 0;
    }

    m_at_time = at_time;
    m_intval = intval;
  }

  void runTask(int at_time, uint64_t intval,
               const std::function<void()> &task_function) {
    setTaskFunction(task_function);
    setTime(at_time, intval);
    runTaskOnTime();
  }

  void runTask(const std::function<void()> &task_function, int at_time,
               uint64_t intval) {
    setTaskFunction(task_function);
    setTime(at_time, intval);
    runTaskOnTime();
  }

  void runTaskOnTime() {
    if (!m_task_function) {
      return;
    }

    if (m_at_time >= 0 && m_intval >= 1000) {
      eventLoop();
    }
  }

  void stopTask() { m_event_timer->cancel(); }

private:
  void eventLoop() {

    uint64_t this_time = now_ms();
    uint64_t this_slot = this_time / m_intval;
    uint64_t this_slot_event_time = this_slot * m_intval + m_at_time;
    uint64_t next_slot_event_time = (this_slot + 1) * m_intval + m_at_time;

    uint64_t remain_time = 0;
    if (this_slot_event_time <= this_time) {
      remain_time = next_slot_event_time - this_time;
    } else {
      remain_time = this_slot_event_time - this_time;
    }

    if (remain_time < 100) { // too close, skip this event
      remain_time += m_intval;
    }

    m_event_timer->expires_from_now(
        boost::posix_time::milliseconds(remain_time));
    m_event_timer->async_wait([this](const boost::system::error_code &error) {
      if (!error && error != boost::asio::error::operation_aborted) {
        eventLoop();
        if (m_task_function) {
          if (m_strand_mode) {
            m_io_service->post(m_strand->wrap([this]() { m_task_function(); }));
          } else {
            m_io_service->post([this]() { m_task_function(); });
          }
        }
      }
    });
  }

  uint64_t now_ms() {
    auto milliseconds_since_epoch = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
    return milliseconds_since_epoch;
  }
};

#endif // GRUUT_ENTERPRISE_MERGER_PERIODIC_TASK_HPP
