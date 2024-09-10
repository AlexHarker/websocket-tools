
#ifndef NW_WS_SERVER_HPP
#define NW_WS_SERVER_HPP

#include "Network/Network.h"

#include "nw_ws_common.hpp"
#include "../common/ws_server_base.hpp"

#include <string>

// Apple Network framework-based websocket server

class nw_ws_server
: public nw_ws_common, public ws_server_base<nw_ws_server, nw_listener_t, nw_connection_t>
{
    friend ws_base<nw_ws_server, nw_listener_t>;
    
public:
 
    // Send
    
    void send(ws_connection_id id, const void *data, size_t size)
    {
        nw_ws_common::send(find(id), data, size);
    }
    
    // Send (to all)
    
    void send(const void *data, size_t size)
    {
        for (auto it = m_map_from_id.begin(); it != m_map_from_id.end(); it++)
            nw_ws_common::send(it->second, data, size);
    }
    
    // Destructor
    
    ~nw_ws_server()
    {
        // Release all connections
        
        for_each_connection(nw_connection_cancel);

        if (m_completion.ready())
            nw_listener_cancel(m_handle);
        nw_release(m_handle);
        
        m_completion.wait_for_closed();
    }
    
private:
    
    // Constructor
    
    template <const ws_server_handlers& handlers>
    nw_ws_server(const char *port, const char *path, ws_server_owner<handlers> owner)
    {
        __block connection_completion& completion = m_completion;
        
        std::string sock_address_url = "ws://localhost:" + std::string(port) + path;
        auto endpoint = nw_endpoint_create_url(sock_address_url.c_str());
        
        // Parameters and protocol for websockets
        
        auto parameters = create_websocket_parameters();
        nw_parameters_set_local_endpoint(parameters, endpoint);
        
        // Create listener
        
        auto listener = nw_listener_create_with_port(port, parameters);
        
        // Hold a reference until cancelled
        
        nw_retain(listener);
        
        // Handlers for connection and state updates
        
        // Listener state block
        
        auto listener_state_block = ^(nw_listener_state_t state, nw_error_t _Nullable error)
        {
            errno = error ? nw_error_get_error_code(error) : 0;
            
            if (state == nw_listener_state_ready)
            {
                completion.set(completion_modes::ready);
            }
            else if (state == nw_listener_state_cancelled || state == nw_listener_state_failed)
            {
                completion.set(completion_modes::closed);
                
                // Release the primary reference on the connection that was taken at creation time
                
                nw_release(listener);
            }
        };
            
        // Connection block (for client connections)
        
        auto connection_block = ^(nw_connection_t _Nonnull connection)
        {
            auto id = add_connection(connection);
            handlers.m_connect(id, owner.m_owner);
            
            // Keep a reference for the client connection
            
            nw_retain(connection);
            
            // State block (for client connections)

            auto client_state_block = ^(nw_connection_state_t state, nw_error_t error)
            {
                nw_endpoint_t remote = nw_connection_copy_endpoint(connection);
                errno = error ? nw_error_get_error_code(error) : 0;
                
                if (state == nw_connection_state_ready)
                {
                    handlers.m_ready(find(connection), owner.m_owner);
                }
                else if (state == nw_connection_state_waiting)
                {
                    nw_connection_cancel(connection);
                }
                else if (state == nw_connection_state_cancelled || state == nw_connection_state_failed)
                {
                    auto id = remove_connection(connection);
                    handlers.m_close(id, owner.m_owner);
                    
                    // Release the  reference that was taken at creation time
                    
                    nw_release(connection);
                }
                nw_release(remote);
            };
            
            // Setup queue and handlers
            
            nw_connection_set_queue(connection, m_queue);
            nw_connection_set_state_changed_handler(connection, client_state_block);
            
            // Accept the connection
            
            nw_connection_start(connection);
                        
            // Start receiving
            
            receive(connection, id, handlers, owner.m_owner);
        };
        
        // Setup queue and handlers
        
        nw_listener_set_queue(listener, m_queue);
        nw_listener_set_state_changed_handler(listener, listener_state_block);
        nw_listener_set_new_connection_handler(listener, connection_block);
        
        // Start server
        
        nw_listener_start(listener);
        completion.wait_for_completion(nw_ws_connection_timeout_ms);
        
        // Cancel if timed out
        
        if (!completion.completed())
        {
            nw_listener_cancel(listener);
            completion.wait_for_completion();
        }
        
        // Release resources
        
        nw_release(parameters);
        nw_release(endpoint);
        
        if (completion.ready())
        {
            m_handle = listener;
            m_port = nw_listener_get_port(listener);
        }
        else
            nw_release(listener);
    }
    
    connection_completion m_completion;
};

#endif /* NW_WS_SERVER_HPP */
