#include "tcp_server.h"
#include "controller.h"

Fix_Parser fix_parser;
controller ctrl;
void tcp_connection::start()
{
    do_write("Welcome to the FIX Server\n");
    do_read();
}

void tcp_connection::do_read(int from_index)
{
    auto self = shared_from_this();
    curr_loc = 0;
    socket_.async_read_some(boost::asio::buffer(read_buffer_.data()+from_index, 10240), [self, from_index](const boost::system::error_code &error, size_t bytes_transferred)
                            { self->handle_reads(error, bytes_transferred+from_index); });
}

void tcp_connection::handle_reads(const boost::system::error_code &error,
                                  size_t bytes_transferred)
{
    if (!error)
    {
        int last_traversed = 0;
        while(curr_loc < (bytes_transferred-1))
        {
            Parse_Result result = fix_parser.parse(read_buffer_, curr_loc, last_traversed, bytes_transferred);
            // std::cout << "Parsed message: " << (result.msg.has_value() ? result.msg->fix_str : "No valid message parsed") << "\n"
            //           << "Is valid: " << result.is_valid << ", Is half message: " << result.is_half_message << "\n";
            if (result.is_valid &&
                (client_id_ == std::nullopt || (result.msg->has_field(49) && result.msg->get_field(49) == client_id_.value())))
            {
                if (client_id_ == std::nullopt && result.msg->has_field(49))
                {
                    client_id_ = result.msg->get_field(49);
                }
                ctrl.OnMessageReceive(result.msg.value(), shared_from_this());
                curr_loc += last_traversed;
            }
            else if (result.is_half_message)
            {
                // Handle half message case
                std::string remaining_data(read_buffer_.data() + curr_loc, last_traversed);
                if (read_buffer_.size() < (last_traversed + 10240))
                {
                    read_buffer_.resize(last_traversed + 10240,'\0');
                }
                std::copy(remaining_data.begin(), remaining_data.end(), read_buffer_.begin());
                // std::cout << "Received half message msg=" << read_buffer_.substr(0, last_traversed) << ", waiting for the rest...\n";
                do_read(last_traversed);
                return;
            }
            else
            {
                do_write("Initial Fix Validation failed\n");
                curr_loc += last_traversed;
                if (state_ == tcp_connection_state::Authenticating)
                {
                    close();
                }
            }
            std::cout << "Current location in buffer: " << curr_loc << " out of " << bytes_transferred << "\n";
        }
        do_read();
    }
    else
    {
        std::cout << "read or byte transfer failed: " << error.message() << "\n";
        close();
    }
}

void tcp_connection::do_write(const std::string &msg)
{
    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(msg),
                             [self](const boost::system::error_code &error, size_t bytes_transferred)
                             { self->handle_write(error, bytes_transferred); });
}

void tcp_connection::close()
{
    std::cout << "closing connection\n";
    do_write("## getting disconnected ##\n");
    boost::system::error_code ec;
    state_ = tcp_connection_state::Closed;
    if (client_id_ != std::nullopt)
    {
        ctrl.active_connections.erase(client_id_.value());
    }
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    socket_.close(ec);
}

void tcp_server::start_accept()
{
    tcp_connection::pointer new_connection =
        tcp_connection::create(io_context_);
    // FIXED: Replaced std::bind with a modern C++ lambda
    acceptor_.async_accept(new_connection->socket(),
                           [this, new_connection](const boost::system::error_code &error)
                           {
                               handle_accept(new_connection, error);
                           });
}

void tcp_server::handle_accept(tcp_connection::pointer new_connection,
                               const boost::system::error_code &error)
{
    if (!error)
    {
        new_connection->start();
    }
    else
    {
        std::cout << "new connection accept failed: " << error.message() << "\n";
    }
    start_accept();
}