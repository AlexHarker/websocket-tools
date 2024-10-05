
#ifndef WS_CLIENT_BASE_HPP
#define WS_CLIENT_BASE_HPP

#include "ws_base.hpp"

/**
 * @brief A base class template for WebSocket clients.
 *
 * This class serves as a base for WebSocket client implementations, inheriting from `ws_base`. It provides a static 
 * method to create a WebSocket client instance.
 *
 * @tparam T The type of the WebSocket client.
 * @tparam U The type of the WebSocket client handler.
 */

template <class T, class U>
class ws_client_base : public ws_base<T, U>
{
public:
    
    /**
     * @brief Creates a WebSocket client instance.
     *
     * This static method creates and returns a WebSocket client instance by delegating to the `ws_base` class's 
     * `create` method.
     *
     * @tparam handlers A constant reference to the WebSocket client handlers.
     * @param host The host address of the WebSocket server.
     * @param port The port number of the WebSocket server.
     * @param path The path of the WebSocket server.
     * @param owner The WebSocket client owner, containing the necessary handlers.
     * @return T* A pointer to the newly created WebSocket client instance.
     */
    
    template <const ws_client_handlers& handlers>
    static T *create(const char *host,
                     uint16_t port,
                     const char *path,
                     ws_client_owner<handlers> owner)

    {
        return ws_base<T, U>::create(host, port, path, owner);
    }
};

#endif /* WS_CLIENT_BASE_HPP */
