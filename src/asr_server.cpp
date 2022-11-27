//------------------------------------------------------------------------------
//
// Based on https://www.boost.org/doc/libs/develop/libs/beast/example/websocket/server/async/websocket_server_async.cpp
//
//------------------------------------------------------------------------------

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/program_options.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <string_view>

#include "vosk_api.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace po = boost::program_options;  // cli arguments parsing
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

//------------------------------------------------------------------------------
static VoskModel *model;

// custom messages
const char* FINAL_RESULT_REQUEST_MESSAGE = "__final_result_request__";
const char* RESET_RECOGNIZER_MESSAGE = "__reset_recognizer__";

struct Args
{
    float sample_rate = 16000;
    int max_alternatives = 0;
    bool show_words = true;
};


// Report a failure
void fail(beast::error_code ec, char const *what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}


class session : public std::enable_shared_from_this<session>
{
    struct Chunk
    {
        std::string_view result;
    };

    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;
    VoskRecognizer *rec_;
    Chunk chunk_;
    Args args_;

public:
    // Take ownership of the socket
    explicit session(tcp::socket &&socket, Args &&args)
        : ws_(std::move(socket)), args_(std::move(args))

    {
        rec_ = vosk_recognizer_new(model, args.sample_rate);
        vosk_recognizer_set_max_alternatives(rec_, args.max_alternatives);
        vosk_recognizer_set_words(rec_, args.show_words);
    }

    ~session()
    {
        vosk_recognizer_free(rec_);
    }

    // Get on the correct executor
    void run()
    {
        // We need to be executing within a strand to perform async operations
        // on the I/O objects in this session. Although not strictly necessary
        // for single-threaded contexts, this example code is written to be
        // thread-safe by default.
        net::dispatch(ws_.get_executor(),
                      beast::bind_front_handler(
                          &session::on_run,
                          shared_from_this()));
    }

    // Start the asynchronous operation
    void on_run()
    {
        // Set suggested timeout settings for the websocket
        ws_.set_option(
            websocket::stream_base::timeout::suggested(
                beast::role_type::server));

        // Set a decorator to change the Server of the handshake
        ws_.set_option(websocket::stream_base::decorator(
            [](websocket::response_type &res)
            {
                res.set(http::field::server,
                        std::string(BOOST_BEAST_VERSION_STRING) +
                            " websocket-server-async");
            }));
        // Accept the websocket handshake
        ws_.async_accept(
            beast::bind_front_handler(
                &session::on_accept,
                shared_from_this()));

    }

    void
    on_accept(beast::error_code ec)
    {
        if (ec)
            return fail(ec, "accept");

        // Read a message
        do_read();
    }

    void do_read()
    {
        // Read a message into our buffer
        ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                &session::on_read,
                shared_from_this()));
    }

    Chunk process_chunk(const char *message, int len)
    {
        std::string s_message(message);
        if (s_message.find(RESET_RECOGNIZER_MESSAGE) != std::string::npos) {
            vosk_recognizer_reset(rec_);
            return Chunk{};
        }

        std::vector<std::string> final_result_message_options = {
            FINAL_RESULT_REQUEST_MESSAGE,
            "{\"eof\" : 1}"
        };

        const bool final_result_message_found = std::any_of(final_result_message_options.begin(),
                                                      final_result_message_options.end(),
            [&](const std::string& pattern) {
                return s_message.find(pattern) != std::string::npos;
            });

        if (final_result_message_found) {
            return Chunk{ vosk_recognizer_final_result(rec_) };
        } else if (vosk_recognizer_accept_waveform(rec_, message, len)) {
            return Chunk{vosk_recognizer_result(rec_)};
        } else {
            return Chunk{vosk_recognizer_partial_result(rec_)};
        }
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        // This indicates that the session was closed
        if (ec == websocket::error::closed)
            return;


        if (ec)
            fail(ec, "read");

        const char *buf = boost::asio::buffer_cast<const char *>(buffer_.cdata());
        int len = static_cast<int>(buffer_.size());

        chunk_ = process_chunk(buf, len);

        ws_.text(ws_.got_binary());

        ws_.async_write(
            boost::asio::const_buffer(chunk_.result.data(), chunk_.result.size()),
            beast::bind_front_handler(
                &session::on_write,
                shared_from_this()));
    }

    void on_write(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec)
            return fail(ec, "write");

        // Clear the buffer
        buffer_.consume(buffer_.size());

        // Do another read
        do_read();
    }
};

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
class listener : public std::enable_shared_from_this<listener>
{
    net::io_context &ioc_;
    tcp::acceptor acceptor_;
    Args args_;

public:
    listener(
        net::io_context &ioc,
        tcp::endpoint endpoint,
        Args args)
        : ioc_(ioc), acceptor_(ioc), args_(args)
    {
        beast::error_code ec;

        // Open the acceptor
        acceptor_.open(endpoint.protocol(), ec);
        if (ec)
        {
            fail(ec, "open");
            return;
        }

        // Allow address reuse
        acceptor_.set_option(net::socket_base::reuse_address(true), ec);
        if (ec)
        {
            fail(ec, "set_option");
            return;
        }

        // Bind to the server address
        acceptor_.bind(endpoint, ec);
        if (ec)
        {
            fail(ec, "bind");
            return;
        }

        // Start listening for connections
        acceptor_.listen(net::socket_base::max_listen_connections, ec);
        if (ec)
        {
            fail(ec, "listen");
            return;
        }
    }

    // Start accepting incoming connections
    void
    run()
    {
        do_accept();
    }

private:
    void do_accept()
    {
        // The new connection gets its own strand
        acceptor_.async_accept(
            net::make_strand(ioc_),
            beast::bind_front_handler(
                &listener::on_accept,
                shared_from_this()));
    }

    void on_accept(beast::error_code ec, tcp::socket socket)
    {
        if (ec)
        {
            fail(ec, "accept");
        }
        else
        {
            // Create the session and run it
            std::make_shared<session>(std::move(socket), std::move(args_))->run();
        }

        // Accept another connection
        do_accept();
    }
};

//------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    std::string _address = "0.0.0.0";
    int _port = 8080;
    int _num_threads = 1;
    int _sample_rate = 16000;
    int _max_alternatives = 0;
    bool _show_words = false;
    std::string _model_path = "";

    po::options_description cli_args("server parameters");
    std::stringstream usage;
    usage << "Usage:\n\t";
    usage << "asr_server -m \"D:/PATH/TO/MODEL\"\n\t";

    cli_args.add_options()
            // First parameter describes option name/short name
            // The second is parameter to option
            // The third is description
            ("help,?", usage.str().c_str())
            ("address,a", po::value<std::string>(&_address)->default_value(_address), "server address")
            ("port,p", po::value<int>(&_port)->default_value(_port), "server port")
            ("threads,t", po::value<int>(&_num_threads)->default_value(_num_threads), "number of threads")
            ("sample-rate,s", po::value<int>(&_sample_rate)->default_value(_sample_rate), "audio stream sample rate")
            ("max-alternatives,l", po::value<int>(&_max_alternatives)->default_value(_max_alternatives), "maximum number of alternative variants")
            ("show-words,w", po::value<bool>(&_show_words)->default_value(_show_words), "show words")
            ("model-path,m", po::value<std::string>(&_model_path)->required(), "path to model")
            ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, cli_args), vm);

    if(vm.count("help") || argc == 0 || vm.count("model-path") == 0)
    {
        cli_args.print(std::cout, 0);
        return EXIT_SUCCESS;

    }

    auto const port = vm["port"].as<int>();
    auto const threads = std::max<int>(1, vm["threads"].as<int>());
    model = vosk_model_new(vm["model-path"].as<std::string>().c_str());

    Args args;
    args.sample_rate = vm["sample-rate"].as<int>();
    args.max_alternatives = vm["max-alternatives"].as<int>();
    args.show_words = vm["show-words"].as<bool>();

    // The io_context is required for all I/O
    net::io_context ioc {threads};
    // Create and launch a listening port
    std::make_shared<listener>(ioc, tcp::endpoint{net::ip::make_address(_address), (unsigned short)port}, args)->run();

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.reserve(threads - 1);
    for (auto i = threads - 1; i > 0; --i)
        v.emplace_back(
            [&ioc]
            {
                ioc.run();
            });
    ioc.run();

    vosk_model_free(model);
    return EXIT_SUCCESS;
}
