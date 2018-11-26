//
// Created by JeonilKang on 2018-11-26.
//

#ifndef GRUUT_ENTERPRISE_MERGER_TEMPLATESINGLETON_HPP
#define GRUUT_ENTERPRISE_MERGER_TEMPLATESINGLETON_HPP

template < typename T >
class TemplateSingleton
{
protected:
  TemplateSingleton()
  {

  }
  virtual ~TemplateSingleton()
  {

  }

public:
  static T * GetInstance()
  {
    if (m_pInstance == nullptr)
      m_pInstance = new T;
    return m_pInstance;
  };

  static void DestroyInstance()
  {
    if (m_pInstance)
    {
      delete m_pInstance;
      m_pInstance = nullptr;
    }
  };

private:
  static T * m_pInstance;
};

template <typename T> T * TemplateSingleton<T>::m_pInstance = 0;


#endif //GRUUT_ENTERPRISE_MERGER_TEMPLATESINGLETON_HPP
