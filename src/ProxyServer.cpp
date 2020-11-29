#include <inttypes.h>
#include <asio.hpp>
#include <ctime>
#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <fstream>
#include <ctime>
#include <vector>

using asio::ip::tcp;

std::string makeCustomHTTPresponse(int status_code, const std::string& status_message, const std::string& explanation)
{
    std::string content = "<!doctype html>\n"
                          "<html>\n"
                          "<head>\n"
                          "    <title>"+std::to_string(status_code)+" "+status_message+"</title>\n"
                          "\n"
                          "    <meta charset=\"utf-8\" />\n"
                          "    <meta http-equiv=\"Content-type\" content=\"text/html; charset=utf-8\" />\n"
                          "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\" />\n"
                          "    <style type=\"text/css\">\n"
                          "    body {\n"
                          "        background-color: #f0f0f2;\n"
                          "        margin: 0;\n"
                          "        padding: 0;\n"
                          "        font-family: -apple-system, system-ui, BlinkMacSystemFont, \"Segoe UI\", \"Open Sans\", \"Helvetica Neue\", Helvetica, Arial, sans-serif;\n"
                          "        \n"
                          "    }\n"
                          "    div {\n"
                          "        width: 600px;\n"
                          "        margin: 5em auto;\n"
                          "        padding: 2em;\n"
                          "        background-color: #fdfdff;\n"
                          "        border-radius: 0.5em;\n"
                          "        box-shadow: 2px 3px 7px 2px rgba(0,0,0,0.02);\n"
                          "    }\n"
                          "    </style>    \n"
                          "</head>\n"
                          "\n"
                          "<body>\n"
                          "<div>\n"
                          "    <h1>"+status_message+"</h1>\n"
                          "    <p>"+explanation+"</p>\n"
                          "</div>\n"
                          "</body>\n"
                          "</html>";

    std::string output = "HTTP/1.0 " + std::to_string(status_code) + " " + status_message + "\n";
    output += "Content-Type: text/html; charset=UTF-8\n";
    output += "Content-Length: "+std::to_string(content.size())+"\n";
    output += "Connection: close\n\n";
    output += content;

    return output;
}

std::vector<std::string> http_request(std::string get_string, std::string host_string) {
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

        /**
         * Reading the whole HTTP response directly --> works for some images
         *
        std::string http_response = ""; // Phyo: Formation of HTTP response

        //long packet_size = 0; // Phyo: Packet Size

        asio::streambuf response;
        asio::read(socket, response, error);
        std::istream response_stream(&response);
        std::string line;
        std::string output;
        while (std::getline(response_stream, line)) {
            //std::cout << line << std::endl;
            output += line+"\n";
        }
        if (error != asio::error::eof)
            throw error;

        return output;
         */

        std::vector<std::string> responses;
        long packet_size = 0; // Phyo: Packet Size

        // Read the response status line. The response streambuf will automatically
        // grow to accommodate the entire line. The growth may be limited by passing
        // a maximum size to the streambuf constructor.
        asio::streambuf response(1500);

        do {
            std::string http_response = ""; // Phyo: Formation of HTTP response

            asio::read_until(socket, response, "\r\n"); // 1st line

            // Check that response is OK.
            std::istream response_stream(&response);
            std::string http_version;
            response_stream >> http_version;
            unsigned int status_code;
            response_stream >> status_code;
            std::string status_message;
            std::getline(response_stream, status_message);

            if (!response_stream || http_version.substr(0, 5) != "HTTP/") {
                responses.push_back("Invalid response\n");
            }
            if (status_code != 200) {
                responses.push_back(makeCustomHTTPresponse(status_code, status_message.c_str(), "Oops! There are some issues. Please keep in mind that our proxy server can only support HTTP protocol!"));
            }

            http_response += "HTTP/1.0 200 OK\n"; // Phyo: Formation of HTTP response

            // Read the response headers, which are terminated by a blank line.
            asio::read_until(socket, response, "\r\n\r\n");

            // Process the response headers.
            std::string header;
            while (std::getline(response_stream, header) && header != "\r") {
                if (header.find("Content-Length:") != std::string::npos) {
                    std::string::size_type start = header.find("Content-Length: ") + 16;
                    std::string::size_type end = header.find("\n", start) - 2;
                    std::string packet_size_str = header.substr(start, end - start);
                    packet_size = std::stol(packet_size_str);

                    std::cout << "* Content-Length: " << packet_size << std::endl;
                }

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

            responses.push_back(http_response);
        } while (packet_size > 1500);

        return responses;
    }
    catch (std::exception& e) {
        std::cout << "Exception: " << e.what() << "\n";
    }

    //return "Something went wrong!";
    return std::vector<std::string>();
}

int main(int argc, char* argv[]) {
    int PORT = 8001;
    if (argc > 1) {
        PORT = atoi(argv[1]);
    }

    std::vector<std::string> blacklist;
    blacklist.push_back("www.archives.nd.edu");

    std::ofstream outfile;
    outfile.open("../output.txt");

    time_t now = time(0);
    char* dt = ctime(&now);

    asio::io_context io_context;
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 8001));

    while (true) {
        std::string request = "";

        // Wait for client
        std::cout << ">>> Listening to incoming requests from Firefox at port " << PORT << "..." << std::endl;
        tcp::socket socket(io_context);
        acceptor.accept(socket);

        std::array<uint8_t, 1500> buf;
        asio::error_code error;
        size_t len = socket.read_some(asio::buffer(buf), error);

        for (int i = 0; i < len; i++) {
            auto x = *reinterpret_cast<char*>(&buf.data()[i]);
            request += x;
        }

        std::string::size_type get_begin = request.find("GET ") + 4;
        std::string::size_type get_end = request.find("\n", get_begin) - 10;
        std::string get_string = request.substr(get_begin, get_end - get_begin);

        std::string::size_type host_begin = request.find("Host: ") + 6;
        std::string::size_type host_end = request.find("User-Agent:", host_begin) - 2;
        std::string host_string = request.substr(host_begin, host_end - host_begin);

        if (get_string.find("www.") == std::string::npos) {
            get_string.insert(7, "www.");
        }

        if (host_string.find("www.") == std::string::npos) {
            host_string = "www." + host_string;
        }

        outfile << "Get: " << get_string << std::endl;
        outfile << "Host: " << host_string << std::endl;
        outfile << "Date: " << dt << std::endl;

        bool blacklist_check = false;
        for (size_t i = 0; i < blacklist.size(); i++) {
            if (blacklist[i] == host_string) blacklist_check = true;
        }

        if (blacklist_check == false) {
            std::vector<std::string> responses;
            responses = http_request(get_string, host_string);

            for (int i = 0; i < responses.size(); i++) {
                asio::write(socket, asio::buffer(responses[i]), error);

                //std::cout << "----------------------" << std::endl;
                //std::cout << responses[i] << std::endl;
                //std::cout << "----------------------" << std::endl;
            }

            std::cout << std::endl;
        }

        else std::cout << "Invalid request, this domain is blacklisted" << std::endl;
    }
    return 0;
}