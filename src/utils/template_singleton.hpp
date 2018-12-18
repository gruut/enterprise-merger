#pragma once

#include <cstdlib>

/*
#include <mutex>

template <typename T> class TemplateSingleton {
public:
  static T *getInstance() {

    std::call_once(m_once_flag, [] { m_instance.reset(new T); });

    return m_instance.get();
  };

private:
  static std::shared_ptr<T> m_instance;
  static std::once_flag m_once_flag;
};
template <typename T>
std::shared_ptr<T> TemplateSingleton<T>::m_instance = nullptr;
template <typename T> std::once_flag TemplateSingleton<T>::m_once_flag;
*/

template <typename T> class TemplateSingleton {
public:
  static std::shared_ptr<T> getInstance() {
    auto pointer = m_instance.lock();

    if (!pointer) {
      pointer = std::make_shared<T>();
      m_instance = pointer;
    }

    return pointer;
  }

private:
  static std::weak_ptr<T> m_instance;
};

template <typename T> std::weak_ptr<T> TemplateSingleton<T>::m_instance;
