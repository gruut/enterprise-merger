#pragma once

#include <cstdlib>

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
    static T * getInstance()
    {
        if (m_instance == nullptr)
            m_instance = new T;
        //atexit(DestroyInstance);
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
    static T * m_instance;
};

template <typename T> T * TemplateSingleton<T>::m_instance = 0;