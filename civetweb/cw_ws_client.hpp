
#ifndef CW_WS_CLIENT_HPP
#define CW_WS_CLIENT_HPP

#include "../common/ws_handlers.hpp"
#include "../common/ws_base.hpp"

#include "../../dependencies/civetweb/include/civetweb.h"

// CivetWeb-based Websocket Client

class cw_ws_client : public ws_client_base<cw_ws_client, struct mg_connection *>
{
    friend ws_base<cw_ws_client, struct mg_connection *>;

public:
    
    // Types
    
    using connection_type = struct mg_connection *;
    
    // Destructor
    
    ~cw_ws_client()
    {
        // FIX
    }
    
    // Send
    
    void send(const void *data, size_t size)
    {
        auto char_data = reinterpret_cast<const char *>(data);
        int bytes = mg_websocket_client_write(m_handle, MG_WEBSOCKET_OPCODE_BINARY, char_data, size);
        
        if (bytes != size)
        {
            // Handle errors
        }
    }
    
private:
    
    // CivetWeb handler wrapper
    
    template <const ws_client_handlers& handlers>
    struct cw_handlers
    {
        static int data(struct mg_connection *connection, int, char *buffer, size_t size, void *x)
        {
            auto id = as_ws_connection_id(connection);
            handlers.m_receive(id, buffer, size, x);
            return 1;
        }
        
        static void close(const struct mg_connection *connection, void *x)
        {
            auto id = as_ws_connection_id(connection);
            handlers.m_close(id, x);
        }
    };
    
    template <const ws_client_handlers& handlers>
    cw_ws_client(const char *host, int port, const char *path, ws_client_owner<handlers> owner)
    {
        m_handle = mg_connect_websocket_client(host,
                                               port,
                                               0,                           // ssl off
                                               nullptr, 0,                  // errors buffer / size
                                               path, "null",                // origin (use "null")
                                               cw_handlers<handlers>::data,
                                               cw_handlers<handlers>::close,
                                               owner.m_owner);
    }
};

#endif /* CW_WS_CLIENT_HPP */
