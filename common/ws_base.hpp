
#ifndef WS_BASE_HPP
#define WS_BASE_HPP

#include <cstdint>

using ws_connection_id = uintptr_t;

ws_connection_id as_ws_connection_id(const void *ptr)
{
    return reinterpret_cast<uintptr_t>(ptr);
}

template <class T, class U>
class ws_base
{
protected:
    
    // Create
    
    template <typename ...Args>
    static T *create(Args...args)
    {
        auto object = new T(args...);
        
        if (!object->m_handle)
        {
            delete object;
            return nullptr;
        }
        
        return object;
    }
    
    U m_handle = nullptr;
};

#endif /* WS_BASE_HPP */
