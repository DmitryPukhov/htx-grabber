
#define TESTING

#include <cassert>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <string>
#include <gtest/gtest.h>
#include <boost/filesystem.hpp>
#include <json/json.h>

#include "app.hpp"

namespace fs = std::filesystem;

class AppTest : public ::testing::Test
{
};

TEST_F(AppTest, getTickerNormalCase)
{
    auto[ticker, kind] = App::getTicker("market.BTC-USDT.depth.step0");
    ASSERT_STREQ(ticker.c_str(), "BTC-USDT");
    ASSERT_STREQ(kind.c_str(), "depth.step0");
}

TEST_F(AppTest, getTickerCannotParse)
{
    auto [ticker, kind] = App::getTicker("the whole string");
    ASSERT_STREQ(ticker.c_str(), "the whole string");
    ASSERT_STREQ(kind.c_str(), "the whole string");

}
