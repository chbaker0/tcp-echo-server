#include <iostream>

#include <array>
#include <vector>

#include <exception>
#include <functional>
#include <memory>

#include <boost/asio.hpp>

class tcp_echo_connection : public std::enable_shared_from_this<tcp_echo_connection>
{
public:
    std::shared_ptr<tcp_echo_connection> static new_connection(boost::asio::io_service& io_service)
    {
        return std::shared_ptr<tcp_echo_connection>(new tcp_echo_connection(io_service));
    }

    boost::asio::ip::tcp::socket& socket()
    {
        return _socket;
    }

    void start()
    {
        data.resize(256);
        using namespace std::placeholders;
        _socket.async_receive(boost::asio::buffer(data),
                              std::bind(&tcp_echo_connection::handle_echo,
                                        shared_from_this(), _1, _2));
    }

private:
    tcp_echo_connection(boost::asio::io_service& io_service): _socket(io_service) {}

    void handle_echo(const boost::system::error_code& e, std::size_t len)
    {
        for(std::size_t i = 0; i < len; ++i)
            std::cout << data[i];
        if(e)
        {
            std::cerr << "\nConnection closed." << std::endl;
            return;
        }
        else if(len)
        {
            using namespace std::placeholders;
            boost::asio::async_write(_socket,
                                     boost::asio::buffer(std::move(data), len),
                                     std::bind(&tcp_echo_connection::handle_write,
                                               shared_from_this(), _1, _2));
        }
        start();
    }

    void handle_write(const boost::system::error_code&, std::size_t)
    {

    }

    std::vector<char> data;
    boost::asio::ip::tcp::socket _socket;
};

class tcp_echo_server
{
public:
    tcp_echo_server(boost::asio::io_service& io_service, const boost::asio::ip::tcp::endpoint& endpoint):
        acceptor(io_service, endpoint)
    {
        start();
    }

private:
    void start()
    {
        auto connection = tcp_echo_connection::new_connection(acceptor.get_io_service());

        using namespace std::placeholders;
        acceptor.async_accept(connection->socket(), std::bind(&tcp_echo_server::handle_accept, this, connection, _1));
    }

    void handle_accept(std::shared_ptr<tcp_echo_connection> connection, const boost::system::error_code& e)
    {
        if(!e)
            connection->start();
        start();
    }

    boost::asio::ip::tcp::acceptor acceptor;
};

int main()
{
    using namespace boost::asio::ip;
    try
    {
        boost::asio::io_service io_service;
        tcp_echo_server srv(io_service, tcp::endpoint(tcp::v4(), 110));
        io_service.run();
    }
    catch(std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
}
