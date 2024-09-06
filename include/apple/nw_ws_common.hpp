
#ifndef NW_WS_COMMON_HPP
#define NW_WS_COMMON_HPP

#include "Network/Network.h"

#include "../common/ws_handlers.hpp"
#include "../common/ws_base.hpp"

#include <atomic>
#include <chrono>

/**
 * @brief A class that provides common functionality for WebSocket operations.
 *
 * The `nw_ws_common` class encapsulates several utilities and operations related to WebSocket
 * communication, such as managing connection states, sending and receiving data, and handling
 * timeouts. It provides a base structure for creating and managing WebSocket connections with
 * various configuration options.
 *
 * The class includes methods for setting up WebSocket parameters, sending and receiving
 * messages, and waiting for connections to complete or close. It also uses a dispatch queue
 * to manage WebSocket tasks in a serialized fashion.
 *
 * Key components include:
 * - Managing WebSocket connection states (`connecting`, `ready`, `closed`)
 * - Sending and receiving WebSocket data
 * - Handling timeouts for connection completion
 * - Dispatch queue initialization and cleanup
 */
class nw_ws_common
{
protected:
    
    /**
     * @brief Defines the connection timeout in milliseconds for WebSocket operations.
     *
     * The constant `nw_ws_connection_timeout_ms` specifies the default timeout duration
     * (in milliseconds) for WebSocket connection attempts. If a connection is not established
     * within this time frame, it may be considered failed or incomplete.
     */
    static constexpr int nw_ws_connection_timeout_ms = 400;
    
    /**
     * @brief Enum representing the different states of a WebSocket connection.
     *
     * The `completion_modes` enum defines the possible states a WebSocket connection can be in:
     * - `connecting`: The connection is in the process of being established.
     * - `ready`: The connection has been successfully established and is ready for communication.
     * - `closed`: The connection has been closed, and no further communication is possible.
     */
    enum class completion_modes { connecting, ready, closed };

    // Connection Helper
    
    /**
     * @brief A helper class for managing the completion status of a WebSocket connection.
     *
     * The `connection_completion` class tracks and manages the state of a WebSocket connection.
     * It provides methods to wait for a connection to complete, check if it is ready,
     * and determine whether it has closed. The class uses an internal state machine,
     * represented by the `completion_modes` enum, to handle various connection states,
     * such as `connecting`, `ready`, and `closed`.
     *
     * Key functionalities include:
     * - Waiting for connection completion or closure using busy-waiting.
     * - Setting the current connection mode.
     * - Querying whether the connection is complete, ready, or closed.
     *
     * This class primarily assists in the asynchronous management of WebSocket connections.
     */
    class connection_completion
    {
        using clock = std::chrono::steady_clock;
        
    public:
        // FIX - sleep/yield threads?
        
        /**
         * @brief Waits for the completion of a network connection or operation.
         *
         * This function blocks the current thread until the operation completes or the timeout expires.
         * If a timeout is provided, the method will wait for the operation to complete within the specified
         * time frame. If no timeout is specified, it will wait indefinitely until the operation completes.
         *
         * The method utilizes busy-waiting, meaning it continuously checks the status of the operation.
         *
         * @param time_out Optional timeout value in milliseconds. If set to 0 (default),
         *                 the function will wait indefinitely for the operation to complete.
         */
        void wait_for_completion(int time_out = 0)
        {
            auto exit_time = clock::now() + std::chrono::milliseconds(time_out);
            
            // Busy waiting
            
            if (time_out)
            {
                while (!completed() && clock::now() < exit_time){};
            }
            else
            {
                while (!completed()){};
            }
        }
        
        /**
         * @brief Sets the current completion mode of the connection.
         *
         * This function updates the internal state of the connection by setting
         * the completion mode to the specified value. The completion mode determines
         * whether the connection is still in progress, ready, or closed.
         *
         * @param mode The new mode of the connection. It must be one of the values of the
         *             `completion_modes` enum (e.g., `connecting`, `ready`, or `closed`).
         */
        void set(completion_modes mode) { m_mode.store(mode); }
        
        /**
         * @brief Waits for the connection to close.
         *
         * This function blocks the current thread until the connection has fully closed.
         * It continuously checks the internal state of the connection, and only returns
         * once the connection is in the `closed` state.
         *
         * The function uses a busy-wait loop, which means it will repeatedly check the
         * connection status until it is closed.
         */
        void wait_for_closed() { while (!closed()){}; }
        
        /**
         * @brief Checks if the connection process has completed.
         *
         * This function returns a boolean value indicating whether the connection
         * process has finished. A connection is considered completed if the current
         * mode is not `connecting`.
         *
         * @return true if the connection is no longer in the `connecting` state.
         * @return false if the connection is still in progress (i.e., in the `connecting` state).
         */
        bool completed() { return m_mode.load() != completion_modes::connecting; }
        
        /**
         * @brief Checks if the connection is closed.
         *
         * This function returns a boolean value indicating whether the connection
         * has been fully closed. A connection is considered closed if the current
         * mode is set to `closed`.
         *
         * @return true if the connection is in the `closed` state.
         * @return false if the connection is not yet closed.
         */
        bool closed() { return m_mode.load() == completion_modes::closed; }
        
        /**
         * @brief Checks if the connection is ready.
         *
         * This function returns a boolean value indicating whether the connection
         * is in the `ready` state. A connection is considered ready when it is
         * fully established and can start communication.
         *
         * @return true if the connection is in the `ready` state.
         * @return false if the connection is not yet ready.
         */
        bool ready() { return m_mode.load() == completion_modes::ready; }
        
    private:
        
        std::atomic<completion_modes> m_mode = completion_modes::connecting;
    };
    
    // Constructor and Destructor
    /**
     * @brief Constructs a new `nw_ws_common` object and initializes the WebSocket queue.
     *
     * This constructor initializes the WebSocket queue with a user-initiated
     * quality-of-service (QoS) class. The queue is created to handle WebSocket operations
     * in a serial fashion, ensuring that tasks are processed one at a time.
     *
     * The dispatch queue is assigned to `m_queue`, which is initially set to `nullptr`.
     */
    nw_ws_common() : m_queue(nullptr)
    {
        auto attr = dispatch_queue_attr_make_with_qos_class(DISPATCH_QUEUE_SERIAL, QOS_CLASS_USER_INITIATED, -4);
        m_queue = dispatch_queue_create("websocket_queue", attr);
    }
    
    /**
     * @brief Destructor for the `nw_ws_common` class.
     *
     * This destructor is responsible for cleaning up resources used by the `nw_ws_common` object.
     * Specifically, it releases the WebSocket dispatch queue (`m_queue`) to ensure that
     * any resources associated with the queue are properly freed when the object is destroyed.
     *
     * It is also designed to check if additional cleanup is needed, such as ensuring
     * that the queue is empty before releasing it.
     */
    ~nw_ws_common()
    {
        // FIX - do we need to make sure the queue is empty?
        
        dispatch_release(m_queue);
    }

    // Parameters
    
    /**
     * @brief Creates and configures WebSocket parameters for a new connection.
     *
     * This static method generates a set of parameters needed to establish a WebSocket connection.
     * These parameters include configuration options such as protocols, security settings,
     * and other transport-specific options that will be used during the connection setup.
     *
     * The method is intended to provide a standardized way of setting WebSocket options
     * before initiating the connection.
     *
     * @return nw_parameters_t A structured object containing the WebSocket configuration parameters.
     */
    static nw_parameters_t create_websocket_parameters()
    {
        auto set_options = ^(nw_protocol_options_t options)
        {
            nw_tcp_options_set_no_delay(options, true);
            nw_tcp_options_set_enable_keepalive(options, true);
            nw_tcp_options_set_keepalive_idle_time(options, 1);
            nw_tcp_options_set_keepalive_count(options, 1);
            nw_tcp_options_set_keepalive_interval(options, 2);
            nw_tcp_options_set_connection_timeout(options, 2);
            nw_tcp_options_set_persist_timeout(options, 2);
            nw_tcp_options_set_retransmit_connection_drop_time(options, 2);
        };
        
        // Parameters and protocol for websockets
        
        auto parameters = nw_parameters_create_secure_tcp(NW_PARAMETERS_DISABLE_PROTOCOL, set_options);
        auto protocol_stack = nw_parameters_copy_default_protocol_stack(parameters);
        auto websocket_options = nw_ws_create_options(nw_ws_version_13);
        
        nw_protocol_stack_prepend_application_protocol(protocol_stack, websocket_options);
        nw_parameters_set_include_peer_to_peer(parameters, true);
        nw_parameters_set_service_class(parameters, nw_service_class_signaling);
        
        // Release temporaries
        
        nw_release(protocol_stack);
        nw_release(websocket_options);
        
        return parameters;
    }
    
    // Send
    
    /**
     * @brief Sends data over an established WebSocket connection.
     *
     * This method transmits the specified data through the given WebSocket connection.
     * It takes a pointer to the data, the size of the data to be sent, and the connection
     * through which the data is to be transmitted. The connection must be established before
     * invoking this method.
     *
     * @param connection The WebSocket connection (`nw_connection_t`) through which data will be sent.
     * @param data A pointer to the data buffer that contains the information to be transmitted.
     * @param size The size, in bytes, of the data to be sent.
     */
    void send(nw_connection_t connection, const void *data, size_t size)
    {
        auto dispatch_data = dispatch_data_create(data, size, m_queue, DISPATCH_DATA_DESTRUCTOR_DEFAULT);
        
        nw_protocol_metadata_t metadata = nw_ws_create_metadata(nw_ws_opcode_binary);
        nw_content_context_t context = nw_content_context_create("send");
        nw_content_context_set_metadata_for_protocol(context, metadata);
        
        auto send_complete_block = ^(nw_error_t _Nullable error)
        {
            // Release the context on completion
            
            nw_release(context);
        };
        
        nw_connection_send(connection, dispatch_data, context, true, send_complete_block);
    }
    
    // Receive
    
    /**
     * @brief Receives data over an established WebSocket connection and processes it using specified handlers.
     *
     * This static templated method is responsible for receiving data from a WebSocket connection.
     * It accepts a connection, a connection ID, a handler object or function that processes the received data,
     * and an owner object to manage the context in which the handlers operate. The method is designed
     * to be flexible by allowing custom handlers, which are passed as a template parameter.
     *
     * @tparam H The type of the handler used to process the received data. It can be a function,
     *           lambda, or a functor that defines how the incoming data will be handled.
     *
     * @param connection The WebSocket connection (`nw_connection_t`) from which data will be received.
     * @param id The unique identifier (`ws_connection_id`) for the WebSocket connection.
     * @param handlers A handler or handler object of type `H` that will process the incoming data.
     * @param owner A pointer to the object that owns the connection or the handler context.
     */
    template <typename H>
    static void receive(nw_connection_t connection, ws_connection_id id, H handlers, void *owner)
    {
        auto receive_block = ^(dispatch_data_t content,
                               nw_content_context_t context,
                               bool is_complete,
                               nw_error_t receive_error)
        {
            if (!receive_error)
            {
                if (is_complete && content)
                {
                    const void *buffer = nullptr;
                    size_t size = 0;
                    
                    dispatch_data_t contiguous = dispatch_data_create_map(content, &buffer, &size);
                    handlers.m_receive(id, buffer, size, owner);
                    dispatch_release(contiguous);
                }
                
                receive(connection, id, handlers, owner);
            }
            else
            {
                // Only cancel for posix errors that indicate no data

                bool posix = nw_error_get_error_domain(receive_error) == nw_error_domain_posix;
                bool no_data = nw_error_get_error_code(receive_error) == ENODATA;
                
                if (posix && no_data)
                    nw_connection_cancel(connection);
            }
        };
        
        nw_connection_receive(connection, 1, UINT32_MAX, receive_block);
    }
    
    dispatch_queue_t m_queue;
};

#endif /* NW_WS_COMMON_HPP */
