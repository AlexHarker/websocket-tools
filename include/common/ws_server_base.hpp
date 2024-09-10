
#ifndef WS_SERVER_BASE_HPP
#define WS_SERVER_BASE_HPP

#include "ws_base.hpp"

#include <algorithm>
#include <map>
#include <type_traits>

// A base class for all websocket servers
/**
 * @brief A WebSocket server base class template.
 *
 * This class acts as a base class for WebSocket server implementations.
 * It inherits from `ws_base` and provides the necessary structure for
 * managing connections and communication in a WebSocket server context.
 *
 * @tparam T The type of data that will be processed by the server.
 * @tparam server_type The type of server implementation to be used.
 * @tparam connection_type The type representing the connection in the WebSocket server.
 */
template <class T, class server_type, class connection_type>
class ws_server_base : public ws_base<T, server_type>
{
    using const_connection_type = const typename std::remove_pointer<connection_type>::type *const;
    
public:
    
    // Create
    /**
     * @brief Creates a new WebSocket server instance.
     *
     * This static method creates and returns a pointer to a new instance of a WebSocket server.
     * It initializes the server with the provided port, path, and owner, and assigns the given
     * handlers for processing WebSocket events.
     *
     * @tparam handlers The constant reference to the `ws_server_handlers` object that defines
     * the event handling functions (e.g., for connection, disconnection, message processing).
     *
     * @param port The port on which the WebSocket server will listen for incoming connections.
     * @param path The path under which the WebSocket server will operate (e.g., `/ws`).
     * @param owner The owner of the WebSocket server, typically handling higher-level management.
     *
     * @return T* A pointer to the newly created WebSocket server instance.
     */
    template <const ws_server_handlers& handlers>
    static T *create(const char *port, const char *path, ws_server_owner<handlers> owner)
    {
        return ws_base<T, server_type>::create(port, path, owner);
    }
    
    // The number of connected clients
    /**
     * @brief Retrieves the number of active connections.
     *
     * This method returns the current number of active WebSocket connections managed by the server.
     *
     * @return size_t The number of active WebSocket connections.
     */
    size_t size() const
    {
        return m_map_from_connection.size();
    }
    
    /**
     * @brief Retrieves the current port of the WebSocket server.
     *
     * This method returns the port on which the WebSocket server is listening for incoming connections.
     *
     * @return uint16_t The current port of the WebSocket server.
     */    
    uint16_t port() const
    {
        return m_port;
    }
    
protected:
    
    // Find connection pointers from ids
    /**
     * @brief Finds a WebSocket connection by its ID.
     *
     * This method searches for and returns the WebSocket connection associated
     * with the given connection ID. If no connection is found, an appropriate
     * default or null value may be returned depending on the `connection_type`.
     *
     * @param id The unique identifier (`ws_connection_id`) for the WebSocket connection to find.
     *
     * @return connection_type The WebSocket connection corresponding to the given ID, or
     * a default/null value if the connection is not found.
     */
    connection_type find(ws_connection_id id)
    {
        auto it = m_map_from_id.find(id);
        return (it == m_map_from_id.end()) ? nullptr : it->second;
    }
    
    // Find ids from connection pointers
    /**
     * @brief Finds the connection ID associated with a given WebSocket connection.
     *
     * This method searches for and returns the unique `ws_connection_id` associated
     * with the provided WebSocket connection. If the connection is not found, a
     * default or invalid ID may be returned depending on the implementation.
     *
     * @param connection The WebSocket connection (`const_connection_type`) whose associated ID is to be found.
     *
     * @return ws_connection_id The unique identifier for the specified connection,
     * or a default/invalid ID if the connection is not found.
     */
    ws_connection_id find(const_connection_type connection)
    {
        auto it = m_map_from_connection.find(connection);
        return (it == m_map_from_connection.end()) ? -1 : it->second;
    }
    
    // Add a new connection to the maps
    /**
     * @brief Adds a new WebSocket connection to the server.
     *
     * This method adds the specified WebSocket connection to the server's internal connection list
     * and assigns a unique `ws_connection_id` to it. The method returns the ID that corresponds
     * to the newly added connection.
     *
     * @param connection The WebSocket connection (`connection_type`) to be added to the server.
     *
     * @return ws_connection_id The unique identifier assigned to the newly added connection.
     */
    ws_connection_id add_connection(connection_type connection)
    {
        ws_connection_id id = new_id();
        
        m_map_from_connection[connection] = id;
        m_map_from_id[id] = connection;
        
        assert(m_map_from_connection.size() == m_map_from_id.size());
        
        return id;
    }
    
    // Remove an expired connection from the maps
    /**
     * @brief Removes a WebSocket connection from the server.
     *
     * This method removes the specified WebSocket connection from the server's internal connection list.
     * It also returns the unique `ws_connection_id` associated with the removed connection.
     *
     * @param connection The WebSocket connection (`const_connection_type`) to be removed from the server.
     *
     * @return ws_connection_id The unique identifier associated with the removed connection. If the
     * connection is not found, a default or invalid `ws_connection_id` may be returned.
     */
    ws_connection_id remove_connection(const_connection_type connection)
    {
        ws_connection_id id = m_map_from_connection[connection];
        
        m_map_from_connection.erase(connection);
        m_map_from_id.erase(id);
        
        assert(m_map_from_connection.size() == m_map_from_id.size());
        
        return id;
    }
    
    // Call a function on each connection
    /**
     * @brief Applies a function to each active WebSocket connection.
     *
     * This method iterates over all active WebSocket connections managed by the server
     * and applies the provided function (`func`) to each one. The function can be used
     * to perform custom operations on each connection.
     *
     * @tparam F The type of the function or callable object to be applied to each connection.
     *
     * @param func The function or callable object that will be applied to each active WebSocket connection.
     * The function should accept a `connection_type` as its parameter.
     */
    template <typename F>
    void for_each_connection(F func)
    {
        for (auto it = m_map_from_id.begin(); it != m_map_from_id.end(); it++)
            func(it->second);
    }
    
    // Generate a new ID
    /**
     * @brief Generates a new unique WebSocket connection ID.
     *
     * This method generates and returns a new, unique `ws_connection_id`
     * that can be assigned to a WebSocket connection. Each ID is guaranteed
     * to be unique within the server's context.
     *
     * @return ws_connection_id A new unique identifier for a WebSocket connection.
     */
    ws_connection_id new_id()
    {
        ws_connection_id id = 0;
        
        while (find(++id)) {}
        
        return id;
    }
    
    std::map<const_connection_type, ws_connection_id> m_map_from_connection;
    std::map<ws_connection_id, connection_type> m_map_from_id;
    uint16_t m_port = 0;
};

#endif /* WS_SERVER_BASE_HPP */
