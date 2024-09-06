
#ifndef CW_WS_CLIENT_HPP
#define CW_WS_CLIENT_HPP

#include "../common/ws_handlers.hpp"
#include "../common/ws_base.hpp"

#include "../../dependencies/civetweb/include/civetweb.h"

// CivetWeb-based websocket client
/**
 * @brief WebSocket client implementation using the CivetWeb library.
 *
 * The cw_ws_client class inherits from the ws_client_base template class, providing WebSocket
 * client functionality with CivetWeb as the underlying implementation. It is parameterized
 * with its own type (cw_ws_client) and the CivetWeb connection structure (`mg_connection`).
 *
 * This class manages the WebSocket connection lifecycle, sending and receiving data,
 * and handling WebSocket events such as connection closures.
 *
 * @tparam ws_client_base<cw_ws_client, struct mg_connection *>
 * The base class provides common WebSocket client behavior and is specialized for the
 * CivetWeb connection type.
 */
class cw_ws_client : public ws_client_base<cw_ws_client, struct mg_connection *>
{
    friend ws_base<cw_ws_client, struct mg_connection *>;

public:
    
    // Destructor
    /**
     * @brief Destructor for the cw_ws_client class.
     *
     * Cleans up resources used by the cw_ws_client instance.
     * This destructor currently contains a placeholder comment for fixes or additional cleanup logic.
     */
    ~cw_ws_client()
    {
        // FIX
    }
    
    // Send
    /**
     * @brief Sends binary data through the WebSocket connection.
     *
     * This method uses the CivetWeb function `mg_websocket_client_write` to send binary data over
     * an established WebSocket connection. The data is sent with the `MG_WEBSOCKET_OPCODE_BINARY` opcode.
     *
     * @param data Pointer to the data buffer to be sent.
     * @param size Size of the data buffer to be sent.
     *
     * @note If the number of bytes sent is not equal to the size of the data, an error handling
     * mechanism is triggered (not fully implemented in the code).
     */
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
    /**
     * @brief CivetWeb handler wrapper for WebSocket events.
     *
     * This template structure provides static callback methods to handle WebSocket events like
     * receiving data and closing the connection. These callbacks are called by CivetWeb when
     * corresponding WebSocket events occur.
     *
     * @tparam handlers The WebSocket client handler structure that provides implementations for
     * handling data and connection closure events.
     */
    template <const ws_client_handlers& handlers>
    struct cw_handlers
    {
        /**
        * @brief Data handler for incoming WebSocket messages.
        *
        * This method is called when data is received over the WebSocket connection.
        * It invokes the `m_receive` handler from the provided handler structure.
        *
        * @param connection The CivetWeb WebSocket connection.
        * @param buffer The data buffer containing the received message.
        * @param size Size of the received message.
        * @param x User-defined context for the handler.
        *
        * @return Always returns 1 to indicate success.
        */
        static int data(struct mg_connection *connection, int, char *buffer, size_t size, void *x)
        {
            auto id = as_ws_connection_id(connection);
            handlers.m_receive(id, buffer, size, x);
            return 1;
        }
        
        /**
        * @brief Close handler for WebSocket connection closure.
        *
        * This method is called when the WebSocket connection is closed.
        * It invokes the `m_close` handler from the provided handler structure.
        *
        * @param connection The CivetWeb WebSocket connection.
        * @param x User-defined context for the handler.
        */
        static void close(const struct mg_connection *connection, void *x)
        {
            auto id = as_ws_connection_id(connection);
            handlers.m_close(id, x);
        }
    };
    
    // Constructor
    /**
     * @brief Constructor for the cw_ws_client class.
     *
     * Initializes the WebSocket client by connecting to the WebSocket server using CivetWeb's
     * `mg_connect_websocket_client` function.
     *
     * @tparam handlers The WebSocket client handler structure that provides callbacks for data
     * reception and connection management.
     * @param host The hostname or IP address of the WebSocket server.
     * @param port The port number of the WebSocket server.
     * @param path The path to the WebSocket endpoint on the server.
     * @param owner The WebSocket client owner that manages the lifecycle and events of this client.
     *
     * @note SSL is turned off in this implementation (the third argument is 0).
     */
    template <const ws_client_handlers& handlers>
    cw_ws_client(const char *host, uint16_t port, const char *path, ws_client_owner<handlers> owner)
    {
        m_handle = mg_connect_websocket_client(host,
                                               port,
                                               0,                           // ssl off
                                               errors,                      // errors buffer
                                               256,                         // errors buffer size
                                               path,
                                               "null",                      // origin (use "null")
                                               cw_handlers<handlers>::data,
                                               cw_handlers<handlers>::close,
                                               owner.m_owner);
    }
    
    char errors[256];
};

#endif /* CW_WS_CLIENT_HPP */
