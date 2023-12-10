
#ifndef NW_WS_CLIENT_HPP
#define NW_WS_CLIENT_HPP

#include "Network/Network.h"

#include "nw_ws_common.hpp"
#include "../common/ws_client_base.hpp"

#include <string>

// Apple Network Framework-based Websocket Client

class nw_ws_client : public nw_ws_common, public ws_client_base<nw_ws_client, nw_connection_t>
{
    friend ws_base<nw_ws_client, nw_connection_t>;

public:
    
    // Types
    
    using connection_type = nw_connection_t;
        
    // Destructor
    
    ~nw_ws_client()
    {
        if (m_completion.ready())
            nw_connection_cancel(m_handle);
        nw_release(m_handle);
            
        m_completion.wait_for_closed();
    }
    
    // Send
    
    void send(const void *data, size_t size)
    {
        nw_ws_common::send(m_handle, data, size);
    }
    
private:
    
    template <const ws_client_handlers& handlers>
    nw_ws_client(const char *host, int port, const char *path, ws_client_owner<handlers> owner)
    {
        __block connection_completion& completion = m_completion;
        
        auto id = as_ws_connection_id(this);
        
        std::string port_str = std::to_string(port);
        std::string sock_address_url = "ws://" + std::string(host) + ":" + port_str + path;
        
        auto endpoint = nw_endpoint_create_url(sock_address_url.c_str());
        
        // Create connection with the correct parameters
        
        auto parameters = create_websocket_parameters();
        auto connection = nw_connection_create(endpoint, parameters);
        
        // Hold a reference until cancelled
        
        nw_retain(connection);
        
        // Handle state changes
        
        auto state_block = ^(nw_connection_state_t state, nw_error_t error)
        {
            errno = error ? nw_error_get_error_code(error) : 0;
            
            if (state == nw_connection_state_ready)
            {
                // Start the receive process
                
                completion.m_mode = connection_completion::modes::ready;
                receive(connection, id, handlers, owner.m_owner);
            }
            else if (state == nw_connection_state_waiting)
            {
                nw_connection_cancel(connection);
            }
            else if (state == nw_connection_state_cancelled || state == nw_connection_state_failed)
            {
                completion.m_mode = connection_completion::modes::closed;
                handlers.m_close(id, owner.m_owner);
                
                // Release the primary reference on the connection that was taken at creation time
                
                nw_release(connection);
            }
        };
        
        // Set queue, state changed handler
        
        nw_connection_set_queue(connection, m_queue);
        nw_connection_set_state_changed_handler(connection, state_block);
        
        // Start connection
        
        nw_connection_start(connection);
        completion.wait_for_completion(nw_ws_connection_timeout);
        
        // Cancel if timed out
        
        if (!completion.completed())
        {
            nw_connection_cancel(connection);
            completion.wait_for_completion();
        }
        
        // Release resources
        
        nw_release(parameters);
        nw_release(endpoint);
        
        // Store the connection if it is ready, else release
        
        if (completion.ready())
            m_handle = connection;
        else
            nw_release(connection);
    }
    
    connection_completion m_completion;
};

#endif /* NW_WS_CLIENT_HPP */
