#pragma once
#include <iostream>
#include <string>
#include <json/json.h>
#include <vector>
#include <string>
#include <numeric>
#include <json/value.h>
using namespace std;

/**
 * Write data to files like "data/raw/level2/2025-05-04_BTC-USDT_level2.csv"
 */
class DataPersister
{
public:
    DataPersister(string dirRaw);

    /**
     * Append to existing topic's file or create a new one
     */
    void persist(const string &ticker, const string &kind, vector<Json::Value>);

#ifndef TESTING
private:
#endif
    const string m_dirRaw;

    string buildFilePath(const string &topic, const string &ticket, const tm &day);
    tm getTime(const Json::Value &jsonValue);
    ofstream startNewFileStream(const string &topic, const string &ticket, const tm &dayTime, Json::Value jsonValue);
    string getTicker(const string& channel);
};