
#define TESTING
#include <cassert>
#include <iostream>
#include <fmt/core.h>
#include <fmt/format.h>
#include <iomanip>
#include <ctime>
#include <chrono>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <string>
#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include <json/json.h>

#include "data_persister.hpp"

namespace fs = std::filesystem;

class DataPersisterTest : public ::testing::Test
{
public:
    /**
     * Convert time string "2025-05-06 09:48" to epoch millis utc
     */
    static long to_millis(const std::string &datetime)
    {
        std::tm tm = {};
        std::istringstream ss(datetime);
        int milliseconds = 0;
        char dot;

        // Parse the string into tm struct and milliseconds
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S") >> dot >> milliseconds;

        // Convert tm to time_t (UTC)
        std::time_t time = timegm(&tm); // Note: timegm is not standard but common

        // If timegm not available, use this alternative:
        // std::time_t time = std::mktime(&tm); // But this uses local timezone

        // Calculate total milliseconds
        return time * 1000LL + milliseconds;
    }

protected:
    fs::path temp_dir;
    fs::path temp_file;

    void SetUp() override
    {
        temp_dir = fs::temp_directory_path() / fs::path("data_persister_test");
        fs::create_directory(temp_dir);
    }

    void TearDown() override
    {
        // Clean up - remove the temp directory and all contents
        fs::remove_all(temp_dir);
    }
};

TEST_F(DataPersisterTest, getTime)
{
    // given
    DataPersister persister("");
    Json::Value jsonValue;
    // Tue May 06 2025 06:25:27.354
    jsonValue["ts"] = 1746512727354L;

    // call
    tm time = persister.getTime(jsonValue);
    // Tue May 06 2025 06:25:27.354
    //  asserts
    assert(time.tm_year == 125);
    assert(time.tm_mon == 4); // month is zero-based
    assert(time.tm_mday == 6);
    assert(time.tm_hour == 6);
    assert(time.tm_min == 25);
    assert(time.tm_sec == 27);
}

TEST_F(DataPersisterTest, buildfilePath)
{
    // Given
    DataPersister persister("rootdir");
    tm time = {};
    time.tm_year = 125;
    time.tm_mon = 4;
    time.tm_mday = 4;

    // Call
    // topic is like market.BTC-USDT.depth.step0
    std::string filePath = persister.buildFilePath("depth.step0", "BTC-USDT", time);

    // Assert
    assert(filePath == "rootdir/BTC-USDT/2025-05-04_BTC-USDT_depth.step0.csv"); // Replaces BOOST_CHECK_EQUAL
}

TEST_F(DataPersisterTest, WriteAndReadFile)
{
    // Write to temp file
    auto temp_file = temp_dir / fs::path("tmp.csv");
    {
        std::ofstream out(temp_file);
        ASSERT_TRUE(out.is_open());
        out << "Test content\nSecond line";
    }

    // Verify file exists
    auto existsFlag = fs::exists(temp_file);
    ASSERT_TRUE(fs::exists(temp_file));

    // Read back and verify content
    std::ifstream in(temp_file);
    std::string content((std::istreambuf_iterator<char>(in)),
                        std::istreambuf_iterator<char>());

    ASSERT_EQ(content, "Test content\nSecond line");
}

TEST_F(DataPersisterTest, persistShouldSplitByDays)
{
    Json::Value value1;
    value1["ts"] = DataPersisterTest::to_millis("2025-05-04 00:00:00");
    value1["bid"] = 1;
    value1["volume"] = 10;

    Json::Value value2;
    value2["ts"] = DataPersisterTest::to_millis("2025-05-04 23:59:59");
    value2["bid"] = 2;
    value2["volume"] = 20;

    Json::Value value3;
    value3["ts"] = DataPersisterTest::to_millis("2025-05-05 00:00:00");
    value3["bid"] = 3;
    value3["volume"] = 30;

    auto jsons = vector<Json::Value>{
        value1, value2, value3};

    // Call
    DataPersister dataPersister(temp_dir.string());
    dataPersister.persist("BTC-USDT", "level2", jsons);

    // First day file
    fs::path filePath = temp_dir / fs::path("BTC-USDT") / fs::path("2025-05-04_BTC-USDT_level2.csv");
    ASSERT_TRUE(fs::exists(filePath));
    ifstream fileStream(filePath);
    // These jsons should present in saved file
    const char *jsonTemplate = "{{\"bid\":{},\"ts\":{},\"volume\":{}}}";

    // Read first day file to vector
    std::vector<std::string> lines;
    for (std::string line; std::getline(fileStream, line);)
    {
        lines.push_back(line);
    }

    // Check first day
    ASSERT_EQ(lines.size(), 2);
    ASSERT_STREQ(lines[0].c_str(), fmt::format(jsonTemplate, 1, value1["ts"].asInt64(), 10).c_str());
    ASSERT_STREQ(lines[1].c_str(), fmt::format(jsonTemplate, 2, value2["ts"].asInt64(), 20).c_str());


    // Second day file
    filePath = temp_dir / fs::path("BTC-USDT") / fs::path("2025-05-05_BTC-USDT_level2.csv");
    fileStream = ifstream(filePath);
    ASSERT_TRUE(fs::exists(filePath));

    // Read second day file to vector
    lines.clear();
    for (std::string line; std::getline(fileStream, line);)
    {
        lines.push_back(line);
    }

    // Check second day
    ASSERT_EQ(lines.size(), 1);
    ASSERT_STREQ(lines[0].c_str(), fmt::format(jsonTemplate, 3, value3["ts"].asInt64(), 30).c_str());
}
