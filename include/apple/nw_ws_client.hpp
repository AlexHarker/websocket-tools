
#ifndef NW_WS_CLIENT_HPP
#define NW_WS_CLIENT_HPP

#include "Network/Network.h"

#include "nw_ws_common.hpp"
#include "../common/ws_client_base.hpp"

#include <string>

// Apple Network framework-based websocket client

/**
 * @brief A WebSocket client implementation using the Apple Network framework.
 *
 * The `nw_ws_client` class provides a WebSocket client based on Apple's Network framework.
 * It inherits functionality from `nw_ws_common` for managing common WebSocket operations,
 * and from `ws_client_base` to handle client-specific behaviors and connection types.
 *
 * @tparam nw_ws_client The specific WebSocket client type used in conjunction with the `ws_client_base` template.
 * @tparam nw_connection_t The connection type managed by the Network framework, representing the underlying network connection.
 *
 * This class integrates both common WebSocket functionality and client-specific behavior, providing methods
 * to send and manage WebSocket connections. The `nw_ws_client` is responsible for initializing the connection,
 * sending data, and handling cleanup when the client is no longer in use.
 *
 * - Inherits from `nw_ws_common` to manage shared WebSocket operations such as message sending and receiving.
 * - Inherits from `ws_client_base` to provide templated behavior for handling network connections.
 *
 * @see nw_ws_common
 * @see ws_client_base
 */
class nw_ws_client : public nw_ws_common, public ws_client_base<nw_ws_client, nw_connection_t>
{
    friend ws_base<nw_ws_client, nw_connection_t>;

public:

    // Destructor
    /**
     * @brief Destructor for the nw_ws_client class.
     *
     * This destructor handles the cleanup of network resources associated with
     * the WebSocket client. If the connection is ready, it cancels the current
     * network connection and releases the associated resources. Additionally,
     * it waits for the connection to be fully closed.
     *
     * - If the connection is active, it will be canceled.
     * - All network handles and resources are released appropriately.
     * - Ensures that the connection is closed before completing the destruction process.
     */
    ~nw_ws_client()
    {
        if (m_completion.ready())
            nw_connection_cancel(m_handle);
        nw_release(m_handle);
            
        m_completion.wait_for_closed();
    }
    
    // Send
    /**
     * @brief Sends data over the WebSocket connection.
     *
     * This method sends a specified block of data over the established WebSocket connection.
     * It forwards the data to the underlying `nw_ws_common::send` method, which handles the
     * low-level network operations.
     *
     * @param data A pointer to the block of data to be sent.
     * @param size The size (in bytes) of the data to be sent.
     *
     * This function does not handle framing or protocol-level details, which are managed
     * internally by the network framework. Ensure that the WebSocket connection is active
     * before calling this method to avoid sending errors.
     */
    void send(const void *data, size_t size)
    {
        nw_ws_common::send(m_handle, data, size);
    }
    
private:
    
    // Constructor
    /**
     * @brief Constructs a new nw_ws_client object and establishes a WebSocket connection.
     *
     * This constructor initializes a WebSocket client using the provided host, port, path, and
     * owner. It sets up the necessary network resources to establish the connection and binds
     * the provided WebSocket client handlers to manage the communication.
     *
     * @tparam handlers A constant reference to the WebSocket client handler structure, which contains
     * callback functions to manage WebSocket events like connection open, message received, or connection close.
     *
     * @param host A pointer to a string representing the server hostname or IP address.
     * @param port The port number used to connect to the WebSocket server.
     * @param path A pointer to a string specifying the path or endpoint on the server to which the WebSocket connects.
     * @param owner A reference to a `ws_client_owner` object that manages the lifetime and ownership of the WebSocket client.
     *
     * This constructor sets up and configures the WebSocket connection, ensuring that the handlers are correctly
     * associated for managing WebSocket events throughout the client's lifecycle.
     */
    template <const ws_client_handlers& handlers>
    nw_ws_client(const char *host, uint16_t port, const char *path, ws_client_owner<handlers> owner)
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
                
                completion.set(completion_modes::ready);
                receive(connection, id, handlers, owner.m_owner);
            }
            else if (state == nw_connection_state_waiting)
            {
                nw_connection_cancel(connection);
            }
            else if (state == nw_connection_state_cancelled || state == nw_connection_state_failed)
            {
                completion.set(completion_modes::closed);
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
        completion.wait_for_completion(nw_ws_connection_timeout_ms);
        
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
