#include <data_persister.hpp>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <json/json.h>
#include <ctime>
#include <iomanip>
#include <boost/format.hpp>
#include <regex>
#include <string>

using namespace std;

DataPersister::DataPersister(string dirRaw) : m_dirRaw(dirRaw) {};

void DataPersister::persist( const string &ticker, const string &kind,vector<Json::Value> data)
{
    if (data.empty())
    {
        cout << "Empty data, nothing to persist" << endl;
        return;
    }
    // Build initial daily file stream
    tm curDayTime = getTime(data[0]);
    string dailyFilePath = buildFilePath(kind, ticker, curDayTime);

    // Create parent directories if they don't exist
    std::filesystem::path parentDir = std::filesystem::path(dailyFilePath).parent_path();
    if (!std::filesystem::exists(parentDir))
    {
        std::filesystem::create_directories(parentDir);
    }
    // file to start writing to
    ofstream dailyFileStream = startNewFileStream(kind, ticker, curDayTime, data[0]);

    // Go through jsons, write to daily file stream, or open a new stream if new day comes.
    for (Json::Value jsonValue : data)
    {
        tm curTime = getTime(jsonValue);
        if (curTime.tm_yday != curDayTime.tm_yday || curTime.tm_year != curDayTime.tm_year)
        {
            // New day comes, close old day, open new
            dailyFileStream.close();
            curDayTime = curTime;
            dailyFileStream = startNewFileStream(kind, ticker, curDayTime, jsonValue);
        }
        
        // Write json string to file
        Json::FastWriter writer;
        writer.omitEndingLineFeed(); // Remove final newline
        dailyFileStream << writer.write(jsonValue) << endl;
     }
}

ofstream DataPersister::startNewFileStream(const string &topic, const string &ticket, const tm &dayTime, Json::Value jsonValue)
{
    // Path like rootdir/level2/2025-05-04_BTC-USDT_level2.csv"
    string dailyFilePath = buildFilePath(topic, ticket, dayTime);
    cout << "Persist the data to " << dailyFilePath << endl;
    ofstream dailyFileStream = ofstream{dailyFilePath};

    return dailyFileStream;
}

tm DataPersister::getTime(const Json::Value &jsonValue)
{
    long millis = jsonValue["ts"].asInt64()/1000L;
    std::tm time = *gmtime(&millis);
    return time;
}

string DataPersister::buildFilePath(const string &topic, const string &ticket, const tm &day)
{
    // Format the date as YYYY-MM-DD
    std::ostringstream dateStream;
    dateStream << std::setfill('0')
               << (day.tm_year + 1900) << "-"
               << std::setw(2) << (day.tm_mon + 1) << "-"
               << std::setw(2) << day.tm_mday;
    std::string dateStr = dateStream.str();

    // Construct the file path
    const string EXTENSION = ".json";
    std::ostringstream pathStream;
    pathStream << m_dirRaw << "/"
               << ticket << "/"
               << dateStr << "_"
               << ticket << "_"
               << topic << EXTENSION;

    return pathStream.str();
}


