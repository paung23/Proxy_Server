#include <inttypes.h>
#include <asio.hpp>
#include <ctime>
#include <iostream>
#include <istream>
#include <ostream>
#include <string>

using asio::ip::tcp;

std::string http_request(std::string get_string, std::string host_string) {
    try {
        asio::io_service io_service;
        asio::error_code error;

        // Get a list of endpoints corresponding to the server name.
        tcp::resolver resolver(io_service);
        tcp::resolver::query query(host_string,  "http");
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
        tcp::resolver::iterator end;

        // Try each endpoint until we successfully establish a connection.
        tcp::socket socket(io_service);
        while (endpoint_iterator != end) {
            socket.close();
            socket.connect(*endpoint_iterator++, error);
        }

        // Form the request. We specify the "Connection: close" header so that the
        // server will close the socket after transmitting the response. This will
        // allow us to treat all data up until the EOF as the content.
        asio::streambuf request;
        std::ostream request_stream(&request);
        request_stream << "GET " << get_string << " HTTP/1.0\r\n";
        request_stream << "Host: " << host_string << "\r\n";
        request_stream << "Accept: */*\r\n";
        request_stream << "Connection: close\r\n\r\n";

        // Send the request.
        asio::write(socket, request);

        std::string http_response = ""; // Phyo: Formation of HTTP response

        // Read the response status line. The response streambuf will automatically
        // grow to accommodate the entire line. The growth may be limited by passing
        // a maximum size to the streambuf constructor.
        asio::streambuf response;
        asio::read_until(socket, response, "\r\n");

        // Check that response is OK.
        std::istream response_stream(&response);
        std::string http_version;
        response_stream >> http_version;
        unsigned int status_code;
        response_stream >> status_code;
        std::string status_message;
        std::getline(response_stream, status_message);
        if (!response_stream || http_version.substr(0, 5) != "HTTP/") {
            return "Invalid response\n";
        }
        if (status_code != 200) {
            return "Response returned with status code "+std::to_string(status_code)+"\n";
        }

        http_response += "HTTP/1.0 200 OK\n"; // Phyo: Formation of HTTP response

        // Read the response headers, which are terminated by a blank line.
        asio::read_until(socket, response, "\r\n\r\n");

        // Process the response headers.
        std::string header;
        while (std::getline(response_stream, header) && header != "\r") {
            http_response += header + "\n"; // Phyo: Formation of HTTP response
        }
        http_response += "\n"; // Phyo: Formation of HTTP response

        std::string body;
        bool first = true;
        while (std::getline(response_stream, body)) {
            if (first) {
                http_response += body; // Phyo: Formation of HTTP response
                first = false;
            }
            else {
                http_response += "\n" + body; // Phyo: Formation of HTTP response
            }

        }
        while (asio::read(socket, response, asio::transfer_at_least(1), error)) {
            std::istream new_response_stream(&response);
            first = true;
            while (std::getline(new_response_stream, body)) {
                if (first) {
                    http_response += body; // Phyo: Formation of HTTP response
                    first = false;
                }
                else {
                    http_response += "\n" + body; // Phyo: Formation of HTTP response
                }
            }
        }
        if (error != asio::error::eof)
            throw error;

        return http_response;
    }
    catch (std::exception& e) {
        std::cout << "Exception: " << e.what() << "\n";
    }

    return "Error!";
}

int main() {
    asio::io_context io_context;
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 8001));

    while (true) {
        std::string request = "";

        // Wait for client
        std::cout << ">>> Listening to incoming requests from Firefox at port 8001..." << std::endl;
        tcp::socket socket(io_context);
        acceptor.accept(socket);

        std::array<uint8_t, 1500> buf;
        asio::error_code error;
        size_t len = socket.read_some(asio::buffer(buf), error);

        for (int i = 0; i < len; i++) {
            auto x = *reinterpret_cast<char*>(&buf.data()[i]);
            request += x;
        }

        int get_begin = request.find("GET ")+4;
        int get_end = request.find("HTTP/1.0")-5; //Only for Firefox on Phyo's laptop
        std::string get_string = request.substr(get_begin, get_end);

        int host_begin = request.find("Host: ")+6;
        int host_end = request.find("User-Agent:")-46; //Only for Firefox on Phyo's laptop
        std::string host_string = request.substr(host_begin, host_end);

        std::string http_response = http_request(get_string, host_string);

        asio::write(socket, asio::buffer(http_response), error);
    }

    return 0;
}