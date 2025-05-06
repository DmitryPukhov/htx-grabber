#pragma once

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <json/json.h>
#include <zlib.h>
#include <iostream>
#include <iomanip>
#include <functional>
#include <string>
#include <unordered_set>
#include <queue>

using namespace std;

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> context_ptr;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

/**
 * Connect, subscribe and get the data from htx websocket
 */
class HuobiWebSocket
{
public:
    HuobiWebSocket(const unordered_set<string> topics);
    HuobiWebSocket(const unordered_set<string> topics, string url);

    void connect();
    void run();
    bool is_connected();

    // map: topic name -> accumulated messages
    map<string, vector<Json::Value>> bufferMap;

private:
    context_ptr on_tls_init();
    /**
     * When the socket has been opened, subscribe to bid/ask, level2 etc. topics
     */
    void onOpen(websocketpp::connection_hdl hdl);

    /**
     * The heart of this class: have got new data message from the socket, so put it to the buffer
     */
    void onMessage(websocketpp::connection_hdl hdl, client::message_ptr msg);

    string decompressGzip(const string &compressed);

    void onClose(websocketpp::connection_hdl hdl);

    void onFail(websocketpp::connection_hdl hdl);

    string m_url = "wss://api-aws.huobi.pro/ws";
    client m_client;
    bool m_connected;

    // htx topics to subscribe, like market.btcusdt.depth.step0
    unordered_set<string> m_topics = {};
};
