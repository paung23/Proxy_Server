/**
 * Austin Butz and Phyo Thuta Aung
 * CPS 373: Intro to Computer Networking
 * Final Project: "PROXY SERVER"
 */

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

/**
 * A function to make a custom HTTP response (for error handling)
 * @param status_code
 * @param status_message
 * @param explanation
 * @return custom HTTP page
 */
std::string makeCustomHTTPresponse(unsigned int status_code, const std::string& status_message, const std::string& explanation)
{
    // Prepare a custom HTTP page with CSS (based on index.html from example.com)
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

    // Prepare HTTP headers
    std::string output = "HTTP/1.0 " + std::to_string(status_code) + " " + status_message + "\r\n";
    output += "Content-Type: text/html; charset=UTF-8\r\n";
    output += "Content-Length: "+std::to_string(content.size())+"\r\n";
    output += "Connection: close\r\n\r\n";
    output += content;

    return output;
}

/**
 * A function to make a HTTP request using Asio library (based on Professor McDanel's code)
 * @param get_string
 * @param host_string
 * @return vector of HTTP responses
 */
std::vector<std::string> http_request(std::string get_string, std::string host_string) {
    std::vector<std::string> responses;
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

        long packet_size = 0; // Phyo: Packet Size
        long received_size = 0; // Phyo: Number of received bytes so far

        bool connection_status = true; // Sometimes, there is no content-length so we will check whether we still need to read more with the connection status

        // For each HTTP response, read it from the socket, analyze for errors, form the output and push it to "responses" vector
        do {
            std::string http_response = ""; // Phyo: Formation of HTTP response

            // Read the response status line. The response streambuf will automatically
            // grow to accommodate the entire line. The growth may be limited by passing
            // a maximum size to the streambuf constructor.
            asio::streambuf response;
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
                responses.push_back(makeCustomHTTPresponse(400, "Bad Request", "The server did not understand the request."));
                return responses;
            }
            if (status_code != 200) {
                responses.push_back(makeCustomHTTPresponse(status_code, status_message.c_str(), "Oops! There are some issues. Please keep in mind that our proxy server can only support HTTP protocol!"));
                return responses;
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

                    //std::cout << "* Content-Length: " << packet_size << std::endl;
                }
                if (header.find("Connection: close") != std::string::npos) {
                    connection_status = false;
                }

                http_response += header + "\n"; // Phyo: Formation of HTTP response
            }
            http_response += "\n"; // Phyo: Formation of HTTP response

            std::string body;
            bool first = true;

            while (std::getline(response_stream, body)) {
                std::string curLine = "";
                if (first) {
                    curLine = body;
                    http_response += body; // Phyo: Formation of HTTP response
                    first = false;
                }
                else {
                    curLine = "\n" + body;
                    http_response += "\n" + body; // Phyo: Formation of HTTP response
                }
                received_size += curLine.size();
            }

            while (asio::read(socket, response, asio::transfer_all(), error)) {
                std::istream new_response_stream(&response);
                first = true;
                while (std::getline(new_response_stream, body)) {
                    std::string curLine = "";
                    if (first) {
                        curLine = body;
                        http_response += body; // Phyo: Formation of HTTP response
                        first = false;
                    }
                    else {
                        curLine = "\n" + body;
                        http_response += "\n" + body; // Phyo: Formation of HTTP response
                    }
                    received_size += curLine.size();
                }
            }
            if (error != asio::error::eof)
                throw error;

            responses.push_back(http_response);
        } while ((packet_size > 1500) && (connection_status));

        return responses;
    }
    catch (std::exception& e) {
        std::cout << "Exception: " << e.what() << "\n";
    }

    responses.push_back(makeCustomHTTPresponse(400, "Bad Request", "The server did not understand the request."));
    return responses;
}

int main(int argc, char* argv[]) {
    int PORT = 8001;
    std::vector<std::string> blacklist;

    // Default port will be 8001 unless specified otherwise
    if (argc > 1) {
        PORT = atoi(argv[1]);
    }

    // Prepare a list of blacklisted websites for content control
    std::ifstream infile("../data/blacklist.txt");
    std::string line;
    while (std::getline(infile, line)) {
        if (!line.empty()) {
            blacklist.push_back(line);
        }
    }

    // Log the network traffic in a file called log.txt from the /data folder
    std::ofstream outfile("../data/log.txt");

    asio::io_context io_context;
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), PORT));

    while (true) {
        std::string client_request = "";

        // Wait for client
        std::cout << ">>> Listening to incoming requests from Firefox at port " << PORT << "..." << std::endl;
        tcp::socket socket(io_context);
        acceptor.accept(socket);

        std::array<uint8_t, 1500> buf;
        asio::error_code error;
        size_t len = socket.read_some(asio::buffer(buf), error);

        time_t now = time(0);
        char* dt = ctime(&now);

        for (int i = 0; i < len; i++) {
            auto x = *reinterpret_cast<char*>(&buf.data()[i]);
            client_request += x;
            std::cout << x;
        }

        std::cout << "- A request has been made." << std::endl;

        // Parsing GET and host from the request
        std::string::size_type get_begin = client_request.find("GET ");
        std::string::size_type get_end;
        if (get_begin == std::string::npos) { // Sometimes, it starts with CONNECT
            get_begin = client_request.find("CONNECT ") + 8;
            get_end = client_request.find("HTTP/1.1", get_begin) - 1;
        }
        else {
            get_begin += 4;
            get_end = client_request.find("HTTP/1.0", get_begin) - 1;
        }
        std::string get_string = "";
        if (get_begin < len) {
            get_string = client_request.substr(get_begin, get_end - get_begin);
        }

        std::string::size_type host_begin = client_request.find("Host: ") + 6;
        std::string::size_type host_end = client_request.find("User-Agent:", host_begin) - 2;
        std::string host_string = "";
        if (host_begin < len) {
            host_string = client_request.substr(host_begin, host_end - host_begin);
        }

        // If the request is not using HTTP protocol, make a custom HTTP error response
        if (get_string.find("http://") == std::string::npos) {
            asio::write(socket, asio::buffer(makeCustomHTTPresponse(400, "Bad Request", "Our proxy server can only support HTTP protocol!")), error);
            std::cout << "- Bad Request" << std::endl;
        }
        else {
            // Log the request
            outfile << "GET " << get_string << std::endl;
            outfile << "Host: " << host_string << std::endl;
            outfile << "Timestamp: " << dt << std::endl;

            // Do a check with the blacklisted sites
            bool blacklist_check = false;
            for (size_t i = 0; i < blacklist.size(); i++) {
                if (blacklist[i] == host_string) blacklist_check = true;
            }

            if (blacklist_check == false) {
                std::vector<std::string> responses;
                responses = http_request(get_string, host_string);

                for (int i = 0; i < responses.size(); i++) {
                    asio::write(socket, asio::buffer(responses[i]), error);
                    std::cout << "- HTTP response " << i+1 << std::endl;
                }
                std::cout << "- The request has been fulfilled." << std::endl;
            }
            else {
                asio::write(socket, asio::buffer(makeCustomHTTPresponse(403, "Forbidden", "Access is forbidden to the requested page.")), error);
                std::cout << "- Forbidden request" << std::endl;
            }
        }

        std::cout << std::endl;
    }

    infile.close();
    outfile.close();

    return 0;
}