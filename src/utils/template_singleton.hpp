//
// Created by JeonilKang on 2018-11-26.
//

#ifndef GRUUT_ENTERPRISE_MERGER_TEMPLATESINGLETON_HPP
#define GRUUT_ENTERPRISE_MERGER_TEMPLATESINGLETON_HPP

template < typename T >
class TemplateSingleton/
{
protected:
  TemplateSingleton()
  {

  }
  virtual ~TemplateSingleton()
  {

  }

public:
  static T * getInstance()
  {
    if (m_instance == nullptr)
      m_instance = new T;
    return m_instance;
  };

  static void destroyInstance()
  {
    if (m_instance)
    {
      delete m_instance;
      m_instance = nullptr;
    }
  };

private:
  static T * m_pInstance;
};

template <typename T> T * TemplateSingleton<T>::m_pInstance = 0;


#endif //GRUUT_ENTERPRISE_MERGER_TEMPLATESINGLETON_HPP
