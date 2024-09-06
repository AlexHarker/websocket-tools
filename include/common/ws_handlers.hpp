
#ifndef WS_HANDLERS_HPP
#define WS_HANDLERS_HPP

#include "ws_base.hpp"

/**
 * @brief Structure containing function type definitions for WebSocket handlers.
 */
struct ws_handler_funcs
{
    /**
     * @brief Function pointer for the ready handler.
     * @param ws_connection_id Connection ID for the WebSocket.
     * @param void* User-defined data.
     */
    using ready_handler = void(*)(ws_connection_id, void *);

    /**
     * @brief Function pointer for the connect handler.
     * @param ws_connection_id Connection ID for the WebSocket.
     * @param void* User-defined data.
     */
    using connect_handler = void(*)(ws_connection_id, void *);

    /**
     * @brief Function pointer for the receive handler.
     * @param ws_connection_id Connection ID for the WebSocket.
     * @param const void* Pointer to the received data.
     * @param size_t Size of the received data.
     * @param void* User-defined data.
     */
    using receive_handler = void(*)(ws_connection_id, const void *, size_t, void *);
};

/**
 * @brief Structure containing client-side WebSocket handlers.
 */
struct ws_client_handlers
{
    /**
     * @brief Function pointer for the receive handler.
     * @param ws_connection_id Connection ID for the WebSocket.
     * @param const void* Pointer to the received data.
     * @param size_t Size of the received data.
     * @param void* User-defined data.
     */
    const ws_handler_funcs::receive_handler m_receive;

    /**
     * @brief Function pointer for the close handler.
     * @param ws_connection_id Connection ID for the WebSocket.
     * @param void* User-defined data.
     */
    const ws_handler_funcs::connect_handler m_close;
};

/**
 * @brief Template structure representing a WebSocket client owner.
 * @tparam handlers Reference to the ws_client_handlers structure.
 */
template <const ws_client_handlers& handlers>
struct ws_client_owner
{
    /**
     * @brief Pointer to the owner of the WebSocket client.
     */
    void *m_owner;
};

/**
 * @brief Structure containing server-side WebSocket handlers.
 */
struct ws_server_handlers
{
    /**
     * @brief Function pointer for the connect handler.
     * @param ws_connection_id Connection ID for the WebSocket.
     * @param void* User-defined data.
     */
    const ws_handler_funcs::connect_handler m_connect;

    /**
     * @brief Function pointer for the ready handler.
     * @param ws_connection_id Connection ID for the WebSocket.
     * @param void* User-defined data.
     */
    const ws_handler_funcs::ready_handler m_ready;

    /**
     * @brief Function pointer for the receive handler.
     * @param ws_connection_id Connection ID for the WebSocket.
     * @param const void* Pointer to the received data.
     * @param size_t Size of the received data.
     * @param void* User-defined data.
     */
    const ws_handler_funcs::receive_handler m_receive;

    /**
     * @brief Function pointer for the close handler.
     * @param ws_connection_id Connection ID for the WebSocket.
     * @param void* User-defined data.
     */
    const ws_handler_funcs::connect_handler m_close;
};

/**
 * @brief Template structure representing a WebSocket server owner.
 * @tparam handlers Reference to the ws_server_handlers structure.
 */
template <const ws_server_handlers& handlers>
struct ws_server_owner
{
    /**
     * @brief Pointer to the owner of the WebSocket server.
     */
    void *m_owner;
};

#endif // WS_HANDLERS_HPP
