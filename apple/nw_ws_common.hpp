
#ifndef NW_WS_COMMON_HPP
#define NW_WS_COMMON_HPP

#include "Network/Network.h"

#include "../common/ws_handlers.hpp"
#include "../common/ws_base.hpp"

#include <atomic>
#include <chrono>

class nw_ws_common
{
protected:
    
    static constexpr int nw_ws_connection_timeout_ms = 400;
    enum class completion_modes { connecting, ready, closed };

    // Connection Helper
    
    class connection_completion
    {
        using clock = std::chrono::steady_clock;
        
    public:
        // FIX - sleep/yield threads?
        
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
        
        void set(completion_modes mode) { m_mode.store(mode); }
        
        void wait_for_closed() { while (!closed()){}; }
        
        bool completed() { return m_mode.load() != completion_modes::connecting; }
        bool closed() { return m_mode.load() == completion_modes::closed; }
        bool ready() { return m_mode.load() == completion_modes::ready; }
        
    private:
        
        std::atomic<completion_modes> m_mode = completion_modes::connecting;
    };
    
    // Constructor and Destructor
    
    nw_ws_common() : m_queue(nullptr)
    {
        auto attr = dispatch_queue_attr_make_with_qos_class(DISPATCH_QUEUE_SERIAL, QOS_CLASS_USER_INITIATED, -4);
        m_queue = dispatch_queue_create("websocket_queue", attr);
    }
    
    ~nw_ws_common()
    {
        // FIX - do we need to make sure the queue is empty?
        
        dispatch_release(m_queue);
    }

    // Parameters
    
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
