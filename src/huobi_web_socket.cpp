
#include <stdio.h>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <json/json.h>
#include <zlib.h>
#include <iostream>
#include <iomanip>
#include <functional>
#include <string>
#include <unordered_set>
#include "huobi_web_socket.hpp"

using namespace std;

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> context_ptr;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

HuobiWebSocket::HuobiWebSocket(
    const unordered_set<string> topics,
    string url)
    : HuobiWebSocket(topics)
{
    m_url = url;
}

HuobiWebSocket::HuobiWebSocket(const unordered_set<string> topics) : m_topics(topics)
{
    // Initialize WebSocket client
    m_client.clear_access_channels(websocketpp::log::alevel::all);
    m_client.clear_error_channels(websocketpp::log::elevel::all);
    m_client.init_asio();
    m_client.set_tls_init_handler(bind(&HuobiWebSocket::on_tls_init, this));
    m_client.set_open_handler(bind(&HuobiWebSocket::onOpen, this, _1));
    m_client.set_message_handler(bind(&HuobiWebSocket::onMessage, this, _1, _2));
    m_client.set_close_handler(bind(&HuobiWebSocket::onClose, this, _1));
    m_client.set_fail_handler(bind(&HuobiWebSocket::onFail, this, _1));
}

void HuobiWebSocket::connect()
{
    cout << "Connecting to " << m_url << endl;
    websocketpp::lib::error_code ec;
    client::connection_ptr con = m_client.get_connection(m_url, ec);

    if (ec)
    {
        cerr << "Connection error: " << ec.message() << endl;
        return;
    }

    m_client.connect(con);
}

void HuobiWebSocket::run()
{
    m_client.run();
}

bool HuobiWebSocket::is_connected()
{
    return m_connected;
}

context_ptr HuobiWebSocket::on_tls_init()
{
    context_ptr ctx = make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12_client);

    try
    {
        ctx->set_options(boost::asio::ssl::context::default_workarounds |
                         boost::asio::ssl::context::no_sslv2 |
                         boost::asio::ssl::context::no_sslv3 |
                         boost::asio::ssl::context::single_dh_use);

        ctx->set_verify_mode(boost::asio::ssl::verify_none);
    }
    catch (exception &e)
    {
        cerr << "TLS init error: " << e.what() << endl;
    }
    return ctx;
}

void HuobiWebSocket::onOpen(websocketpp::connection_hdl hdl)
{
    m_connected = true;
    cout << "Connected to HTX WebSocket" << endl;

    for (const auto &topic : m_topics)
    {
        // Subscribe to topic updates
        cout << "Subscribing to " << topic << endl;
        Json::Value subscribe;
        subscribe["sub"] = topic;
        subscribe["id"] = "htx_grabber_app";

        Json::StreamWriterBuilder writer;
        string message = Json::writeString(writer, subscribe);
        m_client.send(hdl, message, websocketpp::frame::opcode::text);
    }
}

void HuobiWebSocket::onMessage(websocketpp::connection_hdl hdl, client::message_ptr msg)
{
    try
    {
        string payload = msg->get_payload();
        string decompressed = decompressGzip(payload);

        Json::Value root;
        JSONCPP_STRING err;
        Json::CharReaderBuilder builder;
        const unique_ptr<Json::CharReader> reader(builder.newCharReader());

        if (!reader->parse(decompressed.c_str(), decompressed.c_str() + decompressed.length(), &root, &err))
        {
            cerr << "JSON parse error: " << err << endl;
            return;
        }

        // Handle ping/pong
        if (root.isMember("ping"))
        {
            Json::Value pong;
            pong["pong"] = root["ping"];

            Json::StreamWriterBuilder writer;
            string pong_msg = Json::writeString(writer, pong);

            m_client.send(hdl, pong_msg, websocketpp::frame::opcode::text);
            return;
        }

        // If we are grabbing this topic, put it to buffer
        if (root.isMember("ch") && m_topics.find(root["ch"].asString()) != m_topics.end())
        {
            // Put the message to the buffer
            bufferMap[root["ch"].asString()].push_back(root);
        }
    }
    catch (const exception &e)
    {
        cerr << "Message processing error: " << e.what() << endl;
    }
}

string HuobiWebSocket::decompressGzip(const string &compressed)
{
    z_stream zs{};
    if (inflateInit2(&zs, 16 + MAX_WBITS) != Z_OK)
    {
        throw runtime_error("inflateInit2 failed");
    }

    zs.next_in = (Bytef *)compressed.data();
    zs.avail_in = compressed.size();

    int ret;
    char outbuffer[32768];
    string outstring;

    do
    {
        zs.next_out = reinterpret_cast<Bytef *>(outbuffer);
        zs.avail_out = sizeof(outbuffer);

        ret = inflate(&zs, 0);

        if (outstring.size() < zs.total_out)
        {
            outstring.append(outbuffer, zs.total_out - outstring.size());
        }
    } while (ret == Z_OK);

    inflateEnd(&zs);

    if (ret != Z_STREAM_END)
    {
        throw runtime_error("zlib decompression error: " + to_string(ret));
    }

    return outstring;
}

void HuobiWebSocket::onClose(websocketpp::connection_hdl hdl)
{
    m_connected = false;
    cout << "Disconnected from HTX" << endl;
}

void HuobiWebSocket::onFail(websocketpp::connection_hdl hdl)
{
    m_connected = false;
    cerr << "Connection failed" << endl;
}
