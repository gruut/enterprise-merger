#ifndef GRUUT_ENTERPRISE_MERGER_PERIODIC_TASK_HPP
#define GRUUT_ENTERPRISE_MERGER_PERIODIC_TASK_HPP

#include <atomic>
#include <boost/asio.hpp>
#include <memory>

class PeriodicTask : boost::noncopyable {
private:
  std::shared_ptr<boost::asio::io_service> m_io_service;
  std::unique_ptr<boost::asio::deadline_timer> m_even_timer;
  std::unique_ptr<boost::asio::deadline_timer> m_start_timer;
  std::unique_ptr<boost::asio::strand> m_strand;
  std::function<void()> m_task_function;
  uint64_t m_interval{1000};
  std::atomic<bool> m_strand_mode{false};

public:
  PeriodicTask() = default;

  template <typename T = boost::asio::io_service>
  PeriodicTask(T &&io_service, const std::function<void()> &task_function,
               uint64_t intval = 0, int after = -1) {
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
               const std::function<void()> &task_function, uint64_t intval = 0,
               int after = -1) {
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
    m_even_timer.reset(new boost::asio::deadline_timer(io_service));
    m_strand.reset(new boost::asio::strand(io_service));
    m_start_timer.reset(new boost::asio::deadline_timer(io_service));
  }

  void setIoService(std::shared_ptr<boost::asio::io_service> io_service) {
    m_io_service = io_service;
    m_even_timer.reset(new boost::asio::deadline_timer(*io_service));
    m_strand.reset(new boost::asio::strand(*io_service));
    m_start_timer.reset(new boost::asio::deadline_timer(*io_service));
  }

  void setTaskFunction(const std::function<void()> &task_function) {
    m_task_function = task_function;
  }

  void setInterval(uint64_t intval) { m_interval = intval; }

  void runTask() { runTaskAfter(0); }

  void runTaskAfter(int after) {
    if (!m_task_function) {
      return;
    }

    if (after > 0) {
      m_start_timer->expires_from_now(boost::posix_time::milliseconds(after));
      m_start_timer->async_wait([this](const boost::system::error_code &error) {
        if (!error) {
          event_loop();
        }
      });
    } else {
      event_loop();
    }
  }

  void stopTask() { m_even_timer->cancel(); }

private:
  void event_loop() {
    if (m_task_function) {
      if (m_strand_mode) {
        m_io_service->post(m_strand->wrap([this]() { m_task_function(); }));
      } else {
        m_io_service->post([this]() { m_task_function(); });
      }
    }

    m_even_timer->expires_from_now(boost::posix_time::milliseconds(m_interval));
    m_even_timer->async_wait([this](const boost::system::error_code &error) {
      if (!error) {
        event_loop();
      }
    });
  }
};

#endif // GRUUT_ENTERPRISE_MERGER_PERIODIC_TASK_HPP
