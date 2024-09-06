
#ifndef CW_WS_SERVER_HPP
#define CW_WS_SERVER_HPP

#include "../common/ws_handlers.hpp"
#include "../common/ws_server_base.hpp"

#include "../../dependencies/civetweb/include/civetweb.h"

// CivetWeb-based websocket server
/**
 * @brief A WebSocket server class based on CivetWeb, derived from ws_server_base.
 *
 * The `cw_ws_server` class provides WebSocket server functionality using CivetWeb,
 * inheriting core functionality from the `ws_server_base` class. This class
 * specializes `ws_server_base` by using CivetWeb's `mg_context *` for server context
 * and `mg_connection *` for individual WebSocket connections.
 *
 * @tparam cw_ws_server The current class type, used in the CRTP (Curiously Recurring Template Pattern).
 * @tparam mg_context* A pointer type representing the CivetWeb server context.
 * @tparam mg_connection* A pointer type representing an individual WebSocket connection.
 *
 * @note This class inherits from the base class `ws_server_base` and specializes
 *       it to work with CivetWeb's API for WebSocket communication.
 */
class cw_ws_server : public ws_server_base<cw_ws_server, mg_context *, struct mg_connection *>
{
    friend ws_base<cw_ws_server, mg_context *>;

public:
    
    // Destructor
    /**
     * @brief Destructor for the cw_ws_server class.
     *
     * Stops the CivetWeb-based WebSocket server by calling the mg_stop function
     * to properly terminate the server and release resources associated with it.
     */
    ~cw_ws_server()
    {
        mg_stop(m_handle);
    }

    // Send
    /**
     * @brief Sends binary data to a specific WebSocket connection.
     *
     * This method sends a binary message through a WebSocket connection identified
     * by the given connection ID. The data is transmitted as a binary frame.
     *
     * @param id The WebSocket connection ID through which the data will be sent.
     * @param data A pointer to the data that will be sent.
     * @param size The size of the data (in bytes) to be transmitted.
     */
    void send(ws_connection_id id, const void *data, size_t size)
    {
        auto const_char_data = reinterpret_cast<const char *>(data);
        mg_websocket_write(find(id), MG_WEBSOCKET_OPCODE_BINARY, const_char_data, size);
    }
    
    // Send (to all)
    /**
     * @brief Broadcasts binary data to all active WebSocket connections.
     *
     * This method sends the provided binary data to all active WebSocket clients
     * connected to the server. The data is sent as a binary frame.
     *
     * @param data A pointer to the binary data to be sent.
     * @param size The size of the binary data (in bytes).
     *
     * @note This method sends the same data to all currently active WebSocket connections.
     */
    void send(const void *data, size_t size)
    {
        auto const_char_data = reinterpret_cast<const char *>(data);

        for (auto it = m_map_from_id.begin(); it != m_map_from_id.end(); it++)
            mg_websocket_write(it->second, MG_WEBSOCKET_OPCODE_BINARY, const_char_data, size);
    }
    
private:
    
    // Conversion to Server Object
    /**
     * @brief Casts a generic pointer to a cw_ws_server object.
     *
     * This static method takes a generic pointer and casts it to a pointer
     * of type `cw_ws_server`. This can be useful when interacting with
     * APIs or code that uses void pointers.
     *
     * @param x A generic pointer (void*) to be casted to a cw_ws_server object.
     * @return A pointer to the cw_ws_server object.
     */
    static cw_ws_server *as_server(void *x)
    {
        return reinterpret_cast<cw_ws_server *>(x);
    }
    
    // CivetWeb handler wrapper
    /**
     * @brief Template struct to handle WebSocket events with custom handlers.
     *
     * This template struct defines WebSocket event handling logic using a set of custom
     * handlers passed as a template parameter. The `handlers` argument is expected to
     * be a constant reference to an object of type `ws_server_handlers`.
     *
     * @tparam handlers A constant reference to a `ws_server_handlers` object that defines
     *                  various event handlers (e.g., on_connect, on_message, on_disconnect).
     *
     * @note This struct is specialized for WebSocket event handling using the CivetWeb server.
     */
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
    /**
     * @brief Constructor for the cw_ws_server class with custom WebSocket handlers.
     *
     * This constructor initializes a `cw_ws_server` instance with the specified port,
     * WebSocket path, and owner. It also initializes the server with a custom set of
     * WebSocket handlers provided via the `ws_server_owner` template parameter.
     *
     * @tparam handlers A constant reference to a `ws_server_handlers` object that defines
     *                  various event handlers (e.g., on_connect, on_message, on_disconnect).
     * @param port A string representing the port number on which the WebSocket server listens.
     * @param path A string representing the WebSocket endpoint (URL path).
     * @param owner A `ws_server_owner` object that manages ownership and event handling
     *              for the WebSocket server.
     *
     * @note This constructor assumes that the custom event handlers are passed via
     *       the `ws_server_owner` object, which is initialized from the provided `owner` parameter.
     */
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
    
    /**
     * @brief A pointer to the owner or manager of the WebSocket server.
     *
     * This member variable holds a generic pointer to an object that represents
     * the owner or manager responsible for handling the WebSocket server's lifecycle
     * and events. The type of the owner is unspecified, hence the use of a `void *` pointer.
     *
     * @note This variable is typically set during the server initialization and is
     *       used internally to manage ownership and event dispatching.
     */
    void *m_owner;
};

#endif /* CW_WS_SERVER_HPP */
