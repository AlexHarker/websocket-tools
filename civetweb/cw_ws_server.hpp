
#ifndef CW_WS_SERVER_HPP
#define CW_WS_SERVER_HPP

#include "../common/ws_handlers.hpp"
#include "../common/ws_server_base.hpp"

#include "../../dependencies/civetweb/include/civetweb.h"

// CivetWeb-based websocket server

class cw_ws_server : public ws_server_base<cw_ws_server, mg_context *, struct mg_connection *>
{
    friend ws_base<cw_ws_server, mg_context *>;

public:
    
    // Destructor
    
    ~cw_ws_server()
    {
        mg_stop(m_handle);
    }

    // Send
    
    void send(ws_connection_id id, const void *data, size_t size)
    {
        auto const_char_data = reinterpret_cast<const char *>(data);
        mg_websocket_write(find(id), MG_WEBSOCKET_OPCODE_BINARY, const_char_data, size);
    }
    
    // Send (to all)
    
    void send(const void *data, size_t size)
    {
        auto const_char_data = reinterpret_cast<const char *>(data);

        for (auto it = m_map_from_id.begin(); it != m_map_from_id.end(); it++)
            mg_websocket_write(it->second, MG_WEBSOCKET_OPCODE_BINARY, const_char_data, size);
    }
    
private:
    
    // Conversion to Server Object
    
    static cw_ws_server *as_server(void *x)
    {
        return reinterpret_cast<cw_ws_server *>(x);
    }
    
    // CivetWeb handler wrapper
    
    template <const ws_server_handlers& handlers>
    struct cw_handlers
    {
        static int connect(const struct mg_connection *connection, void *x)
        {
            auto id = as_server(x)->add_connection(const_cast<struct mg_connection *>(connection));
            handlers.m_connect(id, get_owner(connection, x));
            return 0;
        }
        
        static void ready(struct mg_connection *connection, void *x)
        {
            auto id = as_server(x)->find(connection);
            handlers.m_ready(id, get_owner(connection, x));
        }
        
        static int receive(struct mg_connection *connection, int, char *buffer, size_t size, void *x)
        {
            auto id = as_server(x)->find(connection);
            handlers.m_receive(id, buffer, size, get_owner(connection, x));
            return 1;
        }
        
        static void close(const struct mg_connection *connection, void *x)
        {
            auto id = as_server(x)->remove_connection(connection);
            handlers.m_close(id, get_owner(connection, x));
        }
        
        // Cast server object
        
        static void *get_owner(const struct mg_connection *connection, void *untyped_local_server)
        {
            const struct mg_request_info *request_info = mg_get_request_info(connection);
            
            assert(request_info != nullptr);
            
            void *owner = request_info->user_data;
            
            assert(as_server(untyped_local_server)->m_owner == owner);
                        
            return owner;
        }
    };
    
    // Constructor

    template <const ws_server_handlers& handlers>
    cw_ws_server(const char *port, const char *path, ws_server_owner<handlers> owner)
    : m_owner(owner.m_owner)
    {
        const char* options[] =
        {
            "listening_ports", port,
            "tcp_nodelay", "1",
            "enable_keep_alive", "yes",
            "keep_alive_timeout_ms", "500",
            NULL
        };
        
        struct mg_init_data mg_start_init_data = {};
        mg_start_init_data.callbacks = nullptr;
        mg_start_init_data.user_data = owner.m_owner;
        mg_start_init_data.configuration_options = options;
        
        struct mg_error_data mg_start_error_data = {};
        char errtxtbuf[256] = {0};
        mg_start_error_data.text = errtxtbuf;
        mg_start_error_data.text_buffer_size = sizeof(errtxtbuf);
        
        m_handle = mg_start2(&mg_start_init_data, &mg_start_error_data);
        
        if (m_handle)
        {
            mg_set_websocket_handler(m_handle,
                                     path,
                                     cw_handlers<handlers>::connect,
                                     cw_handlers<handlers>::ready,
                                     cw_handlers<handlers>::receive,
                                     cw_handlers<handlers>::close,
                                     this);
        }
    }
    
    void *m_owner;
};

#endif /* CW_WS_SERVER_HPP */
