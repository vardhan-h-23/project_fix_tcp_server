#pragma once

#include <ctime>
#include <iostream>
#include <memory>
#include <string>
#include <boost/asio.hpp>
#include "fix_parser.h"

using boost::asio::ip::tcp;

enum tcp_connection_state
{
    Authenticating,
    Connected,
    Closed
};

class tcp_connection
    : public std::enable_shared_from_this<tcp_connection>
{
public:
    typedef std::shared_ptr<tcp_connection> pointer;

    static pointer create(boost::asio::io_context &io_context)
    {
        return pointer(new tcp_connection(io_context));
    }

    tcp::socket &socket()
    {
        return socket_;
    }

    void start();
    void close();
    void do_write(const std::string &msg);
    tcp_connection_state state_;
private:
    tcp_connection(boost::asio::io_context &io_context)
        : socket_(io_context), state_(tcp_connection_state::Authenticating)
    {
    }
    
    void handle_write(const boost::system::error_code & /*error*/, size_t /*bytes_transferred*/) {}
    void do_read();
    void handle_reads(const boost::system::error_code &error,
                      size_t bytes_transferred);
    tcp::socket socket_;
    char data_[10240];
};

class tcp_server
{
public:
    tcp_server(boost::asio::io_context &io_context)
        : io_context_(io_context),
          acceptor_(io_context, tcp::endpoint(tcp::v4(), 8080))
    {
        start_accept();
    }

private:
    void start_accept();

    void handle_accept(tcp_connection::pointer new_connection,
                       const boost::system::error_code &error);

    boost::asio::io_context &io_context_;
    tcp::acceptor acceptor_;
};

class tcp_server_impl
{
public:
    void start()
    {
        try
        {
            boost::asio::io_context io_context;
            tcp_server server(io_context);
            io_context.run();
        }
        catch (std::exception &e)
        {
            std::cerr << "Exception: " << e.what() << std::endl;
        }
    }
};