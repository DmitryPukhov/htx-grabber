#include "huobi_web_socket.hpp"
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <json/json.h>
#include <zlib.h>
#include <iostream>
#include <iomanip>
#include <functional>
#include <string>
#include <boost/algorithm/string.hpp>
#include <set>
#include <queue>
#include <ctime>
#include <chrono>
#include <regex>
#include <string>

#include <data_persister.hpp>
#include <app.hpp>


using namespace std;

void App::run()
{
    // Create websocket for given topic
    unordered_set<string> topics = getTopics();

    HuobiWebSocket ws(topics);
    ws.connect();
    // Listen web socket in a separate thread
    thread ws_thread([&ws]()
                     { ws.run(); });

    // Get received packets in main loop
    processingLoop(ws.bufferMap);

    ws_thread.join();
}

void App::processingLoop(map<string, vector<Json::Value>> &bufferMap)
{
    DataPersister dataPersister(getDataDir());

    // main processing loop
    while (true)
    {
        // Go across all topics: level2, bidask, candles
        for (auto &[topic, messages] : bufferMap)
        {
            cout << "Topic: " << topic << ", messages processing: " << messages.size() << endl;

            // Get accumulated messages and reset the buffer
            auto accumulated = move(messages);
            messages = vector<Json::Value>();

            // Write to disk
            auto[ticker, kind] = getTicker(topic); // todo: set proper ticker
            dataPersister.persist(ticker, kind, accumulated);
        }

        // Have a rest if everything is processed
        this_thread::sleep_for(chrono::seconds(persistIntervalSec));
    }
}

tuple<string, string> App::getTicker(const string &channel)
{
    // Get btc-usdt, it's on second place: market.btc-usdt.level2.step0
    regex pattern(R"(^[^\.]+\.([^\.]+)\.([^\.]+.*)$)");
    smatch matches;

    if (regex_match(channel, matches, pattern) && matches.size() > 2)
    {
        // ticker, kind
        return {matches[1].str(), matches[2].str()};
    }
    // If cannot parse, use the whole string as name
    return {channel, channel};
}
unordered_set<string> App::getTopics()
{
    // Get environment variable: topics to listen
    unordered_set<string> topics = {};
    const char *envChannels = getenv(LISTEN_CHANNELS_ENV_KEY);
    if (envChannels == nullptr)
    {
        envChannels = DEFAULT_CHANNELS;
    }
    boost::split(topics, envChannels, boost::is_any_of(","));
    return topics;
}

string App::getDataDir()
{
    const char *dir = getenv(DIR_PERSIST_ROOT_ENV_KEY);
    if (dir == nullptr)
    {
        dir = DEFAULT_DIR_PERSIST_ROOT;
    }
    return dir;
}
