
#ifndef WS_CLIENT_BASE_HPP
#define WS_CLIENT_BASE_HPP

#include "ws_base.hpp"

// A base class for websocket clients

template <class T, class U>
class ws_client_base : public ws_base<T, U>
{
public:
    
    // Create
    
    template <const ws_client_handlers& handlers>
    static T *create(const char *host,
                     int port,
                     const char *path,
                     ws_client_owner<handlers> owner)

    {
        return ws_base<T, U>::create(host, port, path, owner);
    }
};

#endif /* WS_CLIENT_BASE_HPP */
