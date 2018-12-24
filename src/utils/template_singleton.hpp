#pragma once

#include <cstdlib>
#include <memory>
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

/*
template <typename T> class TemplateSingleton {
protected:
  TemplateSingleton() {}
  virtual ~TemplateSingleton() {}

public:
  static T *getInstance() {
    if (m_instance == nullptr)
      m_instance = new T;
    return m_instance;
  };

  static void destroyInstance() {
    if (m_instance) {
      delete m_instance;
      m_instance = nullptr;
    }
  };

private:
  static T *m_instance;
};

template <typename T> T *TemplateSingleton<T>::m_instance = 0;
 */