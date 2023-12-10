
#ifndef WS_BASE_HPP
#define WS_BASE_HPP

#include <cstdint>

// Type for Connection IDs

using ws_connection_id = uintptr_t;

// Use an arbitrary pointer as an ID (e.g. in clients)

ws_connection_id as_ws_connection_id(const void *ptr)
{
    return reinterpret_cast<uintptr_t>(ptr);
}

// A base class for all websocket clients/servers

template <class T, class handle_type>
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
    
    // Server or Client Handle
    
    handle_type m_handle = nullptr;
};

#endif /* WS_BASE_HPP */
