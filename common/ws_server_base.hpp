
#ifndef WS_SERVER_BASE_HPP
#define WS_SERVER_BASE_HPP

#include "ws_base.hpp"

#include <algorithm>
#include <map>
#include <type_traits>

// A base class for all websocket servers

template <class T, class server_type, class connection_type>
class ws_server_base : public ws_base<T, server_type>
{
    using const_connection_type = const typename std::remove_pointer<connection_type>::type *const;
    
public:
    
    // Create
    
    template <const ws_server_handlers& handlers>
    static T *create(const char *port, const char *path, ws_server_owner<handlers> owner)
    {
        return ws_base<T, server_type>::create(port, path, owner);
    }
    
    // The number of connected clients
    
    size_t size() const
    {
        return m_map_from_connection.size();
    }
    
protected:
    
    // Find connection pointers from ids
    
    connection_type find(ws_connection_id id)
    {
        auto it = m_map_from_id.find(id);
        return (it == m_map_from_id.end()) ? nullptr : it->second;
    }
    
    // Find ids from connection pointers
    
    ws_connection_id find(const_connection_type connection)
    {
        auto it = m_map_from_connection.find(connection);
        return (it == m_map_from_connection.end()) ? -1 : it->second;
    }
    
    // Add a new connection to the maps
    
    ws_connection_id add_connection(connection_type connection)
    {
        ws_connection_id id = new_id();
        
        m_map_from_connection[connection] = id;
        m_map_from_id[id] = connection;
        
        assert(m_map_from_connection.size() == m_map_from_id.size());
        
        return id;
    }
    
    // Remove an expired connection from the maps
    
    ws_connection_id remove_connection(const_connection_type connection)
    {
        ws_connection_id id = m_map_from_connection[connection];
        
        m_map_from_connection.erase(connection);
        m_map_from_id.erase(id);
        
        assert(m_map_from_connection.size() == m_map_from_id.size());
        
        return id;
    }
    
    // Call a function on each connection
    
    template <typename F>
    void for_each_connection(F func)
    {
        for (auto it = m_map_from_id.begin(); it != m_map_from_id.end(); it++)
            func(it->second);
    }
    
    // Generate a new ID
    
    ws_connection_id new_id()
    {
        ws_connection_id id = 0;
        
        while (find(++id)) {}
        
        return id;
    }
    
    std::map<const_connection_type, ws_connection_id> m_map_from_connection;
    std::map<ws_connection_id, connection_type> m_map_from_id;
};

#endif /* WS_SERVER_BASE_HPP */
