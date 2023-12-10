
#ifndef WS_HANDLERS_HPP
#define WS_HANDLERS_HPP

#include "ws_base.hpp"

// Function type definitions

struct ws_handler_funcs
{
    using ready_handler = void(*)(ws_connection_id, void *);
    using connect_handler = void(*)(ws_connection_id, void *);
    using receive_handler = void(*)(ws_connection_id, const void *, size_t, void *);
};

// Client handlers

struct ws_client_handlers
{
    const ws_handler_funcs::receive_handler m_receive;
    const ws_handler_funcs::connect_handler m_close;
};

template <const ws_client_handlers& handlers>
struct ws_client_owner
{
    void *m_owner;
};

// Server handlers

struct ws_server_handlers
{
    const ws_handler_funcs::connect_handler m_connect;
    const ws_handler_funcs::ready_handler m_ready;
    const ws_handler_funcs::receive_handler m_receive;
    const ws_handler_funcs::connect_handler m_close;
};

template <const ws_server_handlers& handlers>
struct ws_server_owner
{
    void *m_owner;
};

#endif /* WS_HANDLERS_HPP */
