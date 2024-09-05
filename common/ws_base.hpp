
#ifndef WS_BASE_HPP
#define WS_BASE_HPP

#include <cstdint>

// Type for Connection IDs
/**
 * @brief Type alias for WebSocket connection IDs.
 *
 * This type is used to represent WebSocket connection IDs
 * using a pointer type (uintptr_t).
 */
using ws_connection_id = uintptr_t;

// Use an arbitrary pointer as an ID (e.g. in clients)
/**
 * @brief Converts a pointer to a WebSocket connection ID.
 *
 * This function takes a pointer and casts it to a
 * ws_connection_id (uintptr_t) type.
 *
 * @param ptr A pointer to be cast into a connection ID.
 * @return A ws_connection_id created from the given pointer.
 */
ws_connection_id as_ws_connection_id(const void *ptr)
{
    return reinterpret_cast<uintptr_t>(ptr);
}

// A base class for all websocket clients/servers
/**
 * @brief A base class template for WebSocket clients and servers.
 *
 * This templated class serves as a base for both WebSocket clients
 * and servers, providing shared functionality like object creation.
 *
 * @tparam T The derived type (client or server).
 * @tparam handle_type The type of handle used by the WebSocket implementation.
 */
template <class T, class handle_type>
class ws_base
{
protected:
    
    // Create
    /**
    * @brief Creates a new WebSocket object.
    *
    * This method allocates a new object of type T and checks if
    * the handle is valid. If the handle is not valid, the object
    * is deleted and null is returned.
    *
    * @tparam Args Types of arguments for constructing the object.
    * @param args Arguments to pass to the constructor of T.
    * @return A pointer to the newly created object, or nullptr if the handle is invalid.
    */
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
    /**
    * @brief Handle for the WebSocket client or server.
    *
    * This member holds the handle for the WebSocket, which could be
    * specific to either a client or a server depending on the usage.
    */
    handle_type m_handle = nullptr;
};

#endif /* WS_BASE_HPP */
