#include <huobi_web_socket.hpp>
#include <data_persister.hpp>

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

using namespace std;

/*
 * Entrypoint: App().run()
 */
class App
{

public:
    // How often we write accumulated data to disk
    int persistIntervalSec = 5;

    /**
     * Entry point App().run()
     */
    void run();

    /**
     * Process new messages forever
     */
    void processingLoop(map<string, vector<Json::Value>> &bufferMap);

#ifndef TESTING
private:
#endif

    static tuple<string, string> getTicker(const string &channel);
    /**
     * Get topics from subscribe from configuration
     */
    unordered_set<string> getTopics();
    string getDataDir();

    const char *DIR_PERSIST_ROOT_ENV_KEY = "DIR_PERSIST_ROOT";
    const char *DEFAULT_DIR_PERSIST_ROOT = "./data/raw"; // will write to current dir
    const char *LISTEN_CHANNELS_ENV_KEY = "LISTEN_CHANNELS";
    const char *DEFAULT_CHANNELS = "market.btcusdt.depth.step0";
};
