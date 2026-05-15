#pragma once

#include "tcp_server.h"
#include "controller.h"

Fix_Parser fix_parser;
controller ctrl;
void tcp_connection::start()
{
    do_write("Welcome to the FIX Server\n");
    do_read();
}

void tcp_connection::do_read()
{
    auto self = shared_from_this();
    socket_.async_read_some(boost::asio::buffer(data_, 10240), [self](const boost::system::error_code &error, size_t bytes_transferred)
                            { self->handle_reads(error, bytes_transferred); });
}

void tcp_connection::handle_reads(const boost::system::error_code &error,
                                  size_t bytes_transferred)
{
    if (!error)
    {
        std::optional<Fix_Message> pmsg = fix_parser.parse(data_, bytes_transferred);
        if (pmsg != std::nullopt &&
            (client_id_ == std::nullopt || (pmsg->has_field(49) && pmsg->get_field(49) == client_id_.value())))
        {
            if (client_id_ == std::nullopt && pmsg->has_field(49))
            {
                client_id_ = pmsg->get_field(49);
            }
            ctrl.OnMessageReceive(pmsg.value(), shared_from_this());
        }
        else
        {
            do_write("Initial Fix Validation failed\n");
            if (state_ == tcp_connection_state::Authenticating)
            {
                close();
            }
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
                               this->handle_accept(new_connection, error);
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