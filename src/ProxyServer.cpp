#include <inttypes.h>
#include <asio.hpp>
#include <ctime>
#include <iostream>
#include <string>

using asio::ip::tcp;

int main() {
    asio::io_context io_context;
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 8001));

    while (true) {
        // Wait for client
        std::cout << ">>> Waiting for incoming requests..." << std::endl;
        tcp::socket socket(io_context);
        acceptor.accept(socket);

        std::array<uint8_t, 1500> buf;
        asio::error_code error;
        size_t len = socket.read_some(asio::buffer(buf), error);

        // Example of error handling
        // if (error != asio::error::eof)
        //   throw asio::system_error(error);

        for (int i = 0; i < len; i++) {
            auto x = *reinterpret_cast<char*>(&buf.data()[i]);
            std::cout << x;
        }
    }

    return 0;
}