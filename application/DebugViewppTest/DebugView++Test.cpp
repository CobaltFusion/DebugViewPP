// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <gtest/gtest.h>
#include "Win32/Process.h"

#include <filesystem>
#include <random>
#include <fstream>
#include <iostream>
#include <print>

#include "Win32/Utilities.h"
#include "Win32/Win32Lib.h"
#include "CobaltFusion/stringbuilder.h"
#include "CobaltFusion/ExecutorClient.h"
#include "CobaltFusion/Executor.h"
#include "IndexedStorageLib/IndexedStorage.h"
#include "DebugViewppLib/ProcessInfo.h"
#include "DebugViewppLib/DBWinBuffer.h"
#include "DebugViewppLib/LogSources.h"
#include "DebugViewppLib/LogSource.h"
#include "DebugViewppLib/DBWinReader.h"
#include "DebugViewppLib/TestSource.h"
#include "DebugViewppLib/VectorLineBuffer.h"
#include "DebugViewppLib/LogFile.h"
#include "DebugViewppLib/FileIO.h"
#include "DebugViewppLib/Conversions.h"
#include "DebugViewppLib/Filter.h"
#include "DebugViewppLib/LogFilter.h"
#include "CobaltFusion/scope_guard.h"
#include "CobaltFusion/StringUtil.h"
#include "DebugViewppLib/DbgviewReader.h"
#include "Win32/TcpStream.h"
#include "DebugViewppLib/ConfigFile.h"
#include "DebugViewppLib/LegacyConfig.h"
#include "DebugViewppLib/Regex.h"
#include "DebugViewppLib/SessionConfig.h"
#include "CobaltFusion/UndoRedoStack.h"

namespace fusion {
namespace debugviewpp {

std::string GetTestString(size_t i)
{
    return stringbuilder() << "BB_TEST_ABCDEFGHI_EE_" << i;
}

class TestLineBuffer : public LineBuffer
{
public:
    TestLineBuffer(size_t size) :
        LineBuffer(size)
    {
    }

    virtual ~TestLineBuffer() = default;

    virtual void WaitForReaderTimeout()
    {
        throw std::exception("WaitForReader timeout");
    }
};

class ScopedTimezoneBias
{
public:
    ScopedTimezoneBias(LONG Bias)
    {
        Win32::SetPrivilege(SE_TIME_ZONE_NAME, true);
        GetTimeZoneInformation(&m_tz);
        TIME_ZONE_INFORMATION tz = m_tz;
        tz.Bias = Bias;
        SetTimeZoneInformation(&tz);
    }

    ~ScopedTimezoneBias()
    {
        SetTimeZoneInformation(&m_tz);
        Win32::SetPrivilege(SE_TIME_ZONE_NAME, false);
    }

private:
    TIME_ZONE_INFORMATION m_tz;
};

std::string GetTestFileName()
{
    using namespace std::filesystem;
    return absolute(path("SaveLoadLogFile_unique_test_filename")).string();
}

std::string SaveLogFile(const LogFile& logfile)
{
    auto filename = GetTestFileName();
    std::ofstream fs;
    OpenLogFile(fs, WStr(filename), OpenMode::Truncate);
    const int count = logfile.Count();
    for (int i = 0; i < count; ++i)
    {
        auto msg = logfile[i];
        WriteLogFileMessage(fs, msg.time, msg.systemTime, msg.processId, msg.processName, msg.text);
    }
    fs.close();
    return filename;
}

std::string AppendLogFile(const LogFile& logfile)
{
    auto filename = GetTestFileName();
    std::ofstream fs;
    OpenLogFile(fs, WStr(filename), OpenMode::Append);
    int count = logfile.Count();
    for (int i = 0; i < count; ++i)
    {
        auto msg = logfile[i];
        WriteLogFileMessage(fs, msg.time, msg.systemTime, msg.processId, msg.processName, msg.text);
    }
    fs.close();
    return filename;
}

LogFile LoadLogFile(const std::string& filename)
{
    // load file test
    std::ifstream file(filename);
    Line line;
    line.processName = "process";
    line.systemTime = Win32::GetSystemTimeAsFileTime();
    LogFile result;
    ReadLogFileMessage(file, line); // ignore header
    while (ReadLogFileMessage(file, line))
        result.Add(Message(line.time, line.systemTime, line.pid, line.processName, line.message));
    return result;
}

bool AreEqual(const Message& a, const Message& b)
{
    if (std::fabs(a.time - b.time) > 0.000001)
        return false;
    if (GetTimeText(a.systemTime) != GetTimeText(b.systemTime))
        return false;
    if (a.processId != b.processId)
        return false;
    if (a.processName != b.processName)
        return false;
    if (a.text != b.text)
        return false;
    // if (a.color != b.color) return false;    // not stored in file
    return true;
}

bool AreEqual(const LogFile& a, const LogFile& b)
{
    if (a.Count() != b.Count())
        return false;
    int count = a.Count();
    for (int i = 0; i < count; ++i)
    {
        if (!AreEqual(a[i], b[i]))
            return false;
    }
    return true;
}

TEST(DebugViewPlusPlusTest, TimeZone)
{
    Timer timer;
    LogFile logFile;
    auto t1 = Win32::GetSystemTimeAsFileTime();
    logFile.Add(Message(timer.Get(), t1, 0, "processname", "test message 1"));
    auto t2 = Win32::GetSystemTimeAsFileTime();
    logFile.Add(Message(timer.Get(), t2, 0, "processname", "test message 2"));
    auto t3 = Win32::GetSystemTimeAsFileTime();
    logFile.Add(Message(timer.Get(), t3, 0, "processname", "test message 3"));
    auto t4 = Win32::GetSystemTimeAsFileTime();
    logFile.Add(Message(0, t4, 0, "processname", "message 4 (zero time message)"));

    SCOPED_TRACE("UTC");
    {
        ScopedTimezoneBias(0);
        auto result = LoadLogFile(SaveLogFile(logFile));
        EXPECT_TRUE((AreEqual(result, logFile)));
    }
    SCOPED_TRACE("(UTC-8) Fremond");
    {
        ScopedTimezoneBias(-480);
        auto result = LoadLogFile(SaveLogFile(logFile));
        EXPECT_TRUE((AreEqual(result, logFile)));
    }
    SCOPED_TRACE("(UTC+8:30) Pyongyang Standard Time");
    {
        ScopedTimezoneBias(+510);
        auto result = LoadLogFile(SaveLogFile(logFile));
        EXPECT_TRUE((AreEqual(result, logFile)));
    }
    SCOPED_TRACE("test boundry case (UTC-12) Baker- / Howland Island (uninhabited islands belonging to the United States)");
    {
        ScopedTimezoneBias(-720);
        auto result = LoadLogFile(SaveLogFile(logFile));
        EXPECT_TRUE((AreEqual(result, logFile)));
    }
    SCOPED_TRACE("test boundry case (UTC+14) Kiritimati (Christmas Island, also uninhabited, part of the Kiribati Line Islands)");
    {
        ScopedTimezoneBias(+720);
        auto result = LoadLogFile(SaveLogFile(logFile));
        EXPECT_TRUE((AreEqual(result, logFile)));
    }

    // test crossing timezones
    SCOPED_TRACE("test sending a logfile into the past (UTC -> UTC-8)");
    {
        auto filename = SaveLogFile(logFile);
        ScopedTimezoneBias(-480);
        auto result = LoadLogFile(filename);
        EXPECT_TRUE((AreEqual(result, logFile)));
    }

    // test crossing timezones
    SCOPED_TRACE("test sending a logfile into the future (UTC-8 -> UTC)");
    {
        std::string filename;
        {
            ScopedTimezoneBias(-480);
            filename = SaveLogFile(logFile);
        }
        auto result = LoadLogFile(filename);
        EXPECT_TRUE((AreEqual(result, logFile)));
    }
}

static uint64_t FileTimeToUInt64(const FILETIME& ft)
{
    return (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
}

TEST(DebugViewPlusPlusTest, USTimeConverterMidnightRollover)
{
    constexpr uint64_t oneSecond = 10'000'000;
    constexpr uint64_t oneDay = 24 * 60 * 60 * oneSecond;

    USTimeConverter converter;
    FILETIME before = {};
    FILETIME after = {};
    ASSERT_TRUE((converter.ReadLocalTimeUSRegion("11:59:59 PM", before)));
    ASSERT_TRUE((converter.ReadLocalTimeUSRegion("12:00:01 AM", after)));

    // two seconds later, on the next day
    EXPECT_EQ(FileTimeToUInt64(after) - FileTimeToUInt64(before), 2 * oneSecond);

    // the next days again, past the end of a month (not too long, to stay clear of daylight saving time changes)
    auto previous = FileTimeToUInt64(after);
    for (int day = 0; day < 40; ++day)
    {
        FILETIME t1 = {};
        FILETIME t2 = {};
        ASSERT_TRUE((converter.ReadLocalTimeUSRegion("11:59:59 PM", t1)));
        ASSERT_TRUE((converter.ReadLocalTimeUSRegion("12:00:01 AM", t2)));
        EXPECT_GT(FileTimeToUInt64(t1), previous);
        EXPECT_EQ(FileTimeToUInt64(t2) - FileTimeToUInt64(t1), 2 * oneSecond);
        EXPECT_LT(FileTimeToUInt64(t1) - previous, oneDay);
        previous = FileTimeToUInt64(t2);
    }
}

TEST(DebugViewPlusPlusTest, InfoTipText)
{
    // a short message is shown as is, tabs are expanded
    EXPECT_EQ(MakeInfoTipText(L"short message"), L"short message");
    EXPECT_EQ(MakeInfoTipText(L"a\tb"), L"a   b");
    EXPECT_EQ(MakeInfoTipText(L""), L"");

    // a long message is wrapped, without losing characters
    const std::wstring text(250, L'x');
    const auto wrapped = MakeInfoTipText(text, 100, 1023);
    EXPECT_EQ((std::count(wrapped.begin(), wrapped.end(), L'\n')), 2);
    EXPECT_EQ((std::count(wrapped.begin(), wrapped.end(), L'x')), 250);
    EXPECT_EQ(wrapped.find(L'\n'), 100);

    // a text that fits exactly is not cut off
    EXPECT_EQ((MakeInfoTipText(std::wstring(30, L'x'), 100, 30)), (std::wstring(30, L'x')));

    // a text that is too long is cut off and ends with an ellipsis, within the limit of a tooltip
    const std::wstring huge(100000, L'y');
    const auto cut = MakeInfoTipText(huge);
    EXPECT_EQ(cut.size(), 1023);
    EXPECT_EQ(cut.substr(cut.size() - 3), L"...");
    EXPECT_EQ(cut.find(L'y'), 0);
}

// #284: a copy of a view can use the lines of the other view when the filters are the same
TEST(DebugViewPlusPlusTest, StringUtilCaseInsensitive)
{
    // equal
    EXPECT_TRUE((IEquals("/Min", "/min")));
    EXPECT_TRUE((IEquals(std::string("JSON"), "json")));
    EXPECT_TRUE((IEquals(std::wstring(L"/MIN"), L"/min")));
    EXPECT_TRUE((IEquals("", "")));
    EXPECT_FALSE((IEquals("abc", "abd")));
    EXPECT_FALSE((IEquals("abc", "abcd"))); // not a prefix
    EXPECT_FALSE((IEquals("abcd", "abc")));

    // a narrow and a wide string can be compared, the text is ASCII
    EXPECT_TRUE((IEquals(std::wstring(L".TXT"), ".txt")));
    EXPECT_TRUE((IEndsWith(std::wstring(L"c:\\logs\\Some.LOG"), ".log")));
    EXPECT_FALSE((IEndsWith(std::wstring(L"c:\\logs\\Some.LOG"), ".txt")));

    // only ASCII letters are folded, the other characters (and non-ASCII text in UTF-8) are compared as they are
    EXPECT_FALSE((IEquals("\xc3\x84", "\xc3\xa4"))); // A with diaeresis and a with diaeresis in UTF-8
    EXPECT_TRUE((IEquals("\xc3\x84x", "\xc3\x84X")));
    EXPECT_FALSE((IEquals("a[", "A{"))); // '[' and '{' are not the same, although they are 32 apart
    EXPECT_FALSE((IEquals("@", "`")));

    // starts with and ends with
    EXPECT_TRUE((IStartsWith(std::wstring(L"/Delay:100"), L"/delay:")));
    EXPECT_FALSE((IStartsWith(std::wstring(L"/del"), L"/delay:"))); // shorter than the prefix
    EXPECT_TRUE((IStartsWith("abc", "")));
    EXPECT_TRUE((IEndsWith("abc", "")));
    EXPECT_TRUE((IEndsWith("Test.DbConf", ".dbconf")));
    EXPECT_FALSE((IEndsWith("conf", ".dbconf")));
    EXPECT_TRUE((EndsWith("File Identification Header", "Header")));
    EXPECT_FALSE((EndsWith("File Identification Header", "header"))); // case sensitive
    EXPECT_FALSE((EndsWith("ab", "cab")));
    EXPECT_TRUE((EndsWith("ab", "")));

    // embedded 0 characters are just characters
    const std::string withNul("a\0B", 3);
    EXPECT_TRUE((IEquals(withNul, std::string("A\0b", 3))));
    EXPECT_FALSE((IEquals(withNul, std::string("A\0c", 3))));
    EXPECT_TRUE((IContains(withNul, std::string("\0b", 2))));
}

TEST(DebugViewPlusPlusTest, StringUtilFindAndContains)
{
    const std::string text = "Hello Hello world, HELLO";
    EXPECT_EQ((IFind(text, "hello")), 0u);
    EXPECT_EQ((IFind(text, "hello", 1)), 6u); // from a position
    EXPECT_EQ((IFind(text, "HELLO", 7)), 19u);
    EXPECT_EQ((IFind(text, "hello", 20)), std::string::npos);
    EXPECT_EQ((IFind(text, "WORLD")), 12u);
    EXPECT_EQ((IFind(text, "worlds")), std::string::npos);
    EXPECT_EQ((IFind(text, "o")), 4u);

    // the match can be at the end, and a pattern as long as the text works
    EXPECT_EQ((IFind("abcdef", "EF")), 4u);
    EXPECT_EQ((IFind("abc", "ABC")), 0u);
    EXPECT_EQ((IFind("abc", "abcd")), std::string::npos);
    EXPECT_EQ((IFind("", "a")), std::string::npos);

    // a partial match is not a match, the search goes on
    EXPECT_EQ((IFind("aab", "ab")), 1u);
    EXPECT_EQ((IFind("ababac", "abac")), 2u);

    // an empty pattern is found at the start position (like std::string::find), but not after the end
    EXPECT_EQ((IFind("abc", "")), 0u);
    EXPECT_EQ((IFind("abc", "", 3)), 3u);
    EXPECT_EQ((IFind("abc", "", 4)), std::string::npos);
    EXPECT_TRUE((IContains("abc", "")));

    EXPECT_TRUE((IContains(std::wstring(L"/TAB:name"), L"/tab:")));
    EXPECT_FALSE((IContains(std::wstring(L"/min"), L"/tab:")));
    EXPECT_TRUE((IContains(std::string("The ERROR is here"), std::string("error"))));

    // wide text with a narrow pattern, and wide characters that are not ASCII are not folded
    EXPECT_EQ((IFind(std::wstring(L"x.LOG"), ".log")), 1u);
    EXPECT_EQ((IFind(std::wstring(L"\u00c4"), L"\u00e4")), std::string::npos);
}

TEST(DebugViewPlusPlusTest, StringUtilLowerAndTrim)
{
    EXPECT_EQ(ToLower(std::string("Mixed CASE 123 \xc3\x84")), "mixed case 123 \xc3\x84"); // the UTF-8 bytes are unchanged
    EXPECT_EQ(ToLower(std::wstring(L"AbC")), L"abc");
    EXPECT_EQ(ToLower(std::string()), "");

    std::string message = "text with trailing space \r\n\t ";
    TrimRight(message, " \r\n\t");
    EXPECT_EQ(message, "text with trailing space");
    TrimRight(message, " \r\n\t"); // nothing more to remove
    EXPECT_EQ(message, "text with trailing space");

    std::string onlySpaces = " \r\n";
    TrimRight(onlySpaces, " \r\n\t");
    EXPECT_EQ(onlySpaces, "");

    std::string empty;
    TrimRight(empty, " ");
    EXPECT_EQ(empty, "");

    // the characters inside are kept, only the ends are trimmed
    EXPECT_EQ((TrimCopy(std::string("  a b \t"), " \r\n\t")), "a b");
    EXPECT_EQ((TrimCopy(std::string("a"), " ")), "a");
    EXPECT_EQ((TrimCopy(std::string("   "), " ")), "");
    EXPECT_EQ((TrimCopy(std::string(""), " ")), "");
    EXPECT_EQ((TrimCopy(std::wstring(L"\t x \n"), L" \t\n")), L"x");
}

// The files in testdata were written by the application (see testdata/README.md), before boost was removed. Files that users saved
// with older versions must stay readable, so these tests describe what is in them, whatever reads them.
std::string TestDataFile(const std::string& name)
{
    return std::string(TESTDATA_DIR) + "/" + name;
}

TEST(DebugViewPlusPlusTest, LegacyFilterFilesCanBeLoaded)
{
    for (const std::string file : {"filters_legacy.xml", "filters_legacy.json"})
    {
        SCOPED_TRACE(file);
        const auto text = ReadTextFile(TestDataFile(file));
        const auto data = file.ends_with(".json") ? FilterDataFromLegacyJson(text) : FilterDataFromLegacyXml(text);
        EXPECT_EQ(data.name, "Überblick: errors & warnings");

        const auto& messageFilters = data.filter.messageFilters;
        const auto& processFilters = data.filter.processFilters;
        ASSERT_EQ(messageFilters.size(), 10);
        ASSERT_EQ(processFilters.size(), 2);

        auto check = [](const Filter& f, const std::string& text, MatchType::type matchType, FilterType::type filterType, COLORREF bg, COLORREF fg, bool enable) {
            EXPECT_EQ(f.text, text);
            EXPECT_EQ(f.matchType, matchType);
            EXPECT_EQ(f.filterType, filterType);
            EXPECT_EQ(f.bgColor, bg);
            EXPECT_EQ(f.fgColor, fg);
            EXPECT_EQ(f.enable, enable);
        };
        const COLORREF white = RGB(255, 255, 255);
        const COLORREF black = RGB(0, 0, 0);
        check(messageFilters[0], "error", MatchType::Simple, FilterType::Include, white, black, true);
        check(messageFilters[1], "noise.*", MatchType::Regex, FilterType::Exclude, RGB(1, 2, 3), RGB(4, 5, 6), true);
        check(messageFilters[2], "a & b <c> \"quoted\" 'single'", MatchType::Wildcard, FilterType::Highlight, RGB(250, 240, 200), RGB(10, 20, 30), false);
        check(messageFilters[3], "id=(\\d+)", MatchType::RegexGroups, FilterType::Token, Colors::Auto, black, true);
        check(messageFilters[4], "CaseSensitive", MatchType::RegexCase, FilterType::Track, RGB(255, 255, 0), black, true);
        check(messageFilters[5], "fatal", MatchType::Simple, FilterType::Stop, RGB(255, 0, 0), white, true);
        check(messageFilters[6], "first only", MatchType::Simple, FilterType::Once, white, black, true);
        check(messageFilters[7], "DBGVIEWCLEAR", MatchType::Simple, FilterType::Clear, white, black, true);
        check(messageFilters[8], "beep", MatchType::Simple, FilterType::Beep, white, black, true);
        check(messageFilters[9], "bookmark me", MatchType::Simple, FilterType::Bookmark, white, black, true);
        check(processFilters[0], "app.exe", MatchType::Simple, FilterType::Include, white, black, true);
        check(processFilters[1], "helper", MatchType::Wildcard, FilterType::Highlight, RGB(200, 220, 255), black, true);
    }
}

TEST(DebugViewPlusPlusTest, FilterDataJsonRoundTrip)
{
    FilterData data;
    data.name = "Überblick \"quoted\" 'single' <&>";
    data.filter.messageFilters.push_back(MakeFilter("error", MatchType::Simple, FilterType::Include));
    data.filter.messageFilters.push_back(MakeFilter("a\\d+ \"q\" &<>", MatchType::Regex, FilterType::Exclude, RGB(1, 2, 3), RGB(4, 5, 6), false));
    data.filter.messageFilters.push_back(MakeFilter("auto", MatchType::RegexGroups, FilterType::Token, Colors::Auto, Colors::Auto));
    data.filter.processFilters.push_back(MakeFilter("app.exe", MatchType::Wildcard, FilterType::Highlight, RGB(250, 240, 200), RGB(0, 0, 0)));

    const auto text = FilterDataToJson(data);
    const auto result = FilterDataFromJson(text);
    EXPECT_EQ(result.name, data.name);
    ASSERT_EQ(result.filter.messageFilters.size(), 3);
    ASSERT_EQ(result.filter.processFilters.size(), 1);
    EXPECT_TRUE((SameDefinition(result.filter, data.filter)));
    EXPECT_EQ(result.filter.messageFilters[2].bgColor, Colors::Auto);
    EXPECT_EQ(result.filter.messageFilters[2].fgColor, Colors::Auto);
    EXPECT_FALSE(result.filter.messageFilters[1].enable);
    EXPECT_EQ(result.filter.messageFilters[1].text, data.filter.messageFilters[1].text);

    // every filter type and match type survives
    FilterData all;
    all.name = "all";
    for (int f = 0; f <= FilterTypeToInt(FilterType::Bookmark); ++f)
    {
        all.filter.messageFilters.push_back(MakeFilter("x", IntToMatchType(f % 5), IntToFilterType(f)));
    }
    EXPECT_EQ(all.filter.messageFilters.size(), 11);
    EXPECT_TRUE((SameDefinition(FilterDataFromJson(FilterDataToJson(all)).filter, all.filter)));
}

TEST(DebugViewPlusPlusTest, FilterDataJsonDefaultsAndErrors)
{
    // only the text and the types are required
    const auto data = FilterDataFromJson(R"({"name": "n", "messageFilters": [{"text": "t", "matchType": "Simple", "filterType": "Include"}]})");
    ASSERT_EQ(data.filter.messageFilters.size(), 1);
    EXPECT_TRUE(data.filter.messageFilters[0].enable);
    EXPECT_EQ(data.filter.messageFilters[0].bgColor, (RGB(255, 255, 255)));
    EXPECT_EQ(data.filter.messageFilters[0].fgColor, (RGB(0, 0, 0)));
    EXPECT_TRUE(data.filter.processFilters.empty());

    EXPECT_THROW(FilterDataFromJson(""), std::runtime_error);
    EXPECT_THROW(FilterDataFromJson("[1,2]"), std::runtime_error);
    EXPECT_THROW(FilterDataFromJson("{not json"), std::runtime_error);
    EXPECT_THROW(FilterDataFromJson(R"({"version": 99, "name": "n"})"), std::runtime_error);
    EXPECT_THROW(FilterDataFromJson(R"({"messageFilters": []})"), std::runtime_error); // no name
    EXPECT_THROW(FilterDataFromJson(R"({"name": "n", "messageFilters": [{"matchType": "Simple", "filterType": "Include"}]})"), std::runtime_error); // no text
    EXPECT_THROW(FilterDataFromJson(R"({"name": "n", "messageFilters": {"text": "t"}})"), std::runtime_error); // not an array
    EXPECT_THROW(FilterDataFromJson(R"({"name": "n", "messageFilters": [{"text": "t", "matchType": "Simple", "filterType": "Include", "backColor": {"r": 300, "g": 0, "b": 0}}]})"), std::runtime_error);
}

// the legacy readers and the files of the old version: what is in the golden files
TEST(DebugViewPlusPlusTest, LegacyFilterReadersKeepAutoColorAndSpecialCharacters)
{
    for (const std::string file : {"filters_legacy.xml", "filters_legacy.json"})
    {
        SCOPED_TRACE(file);
        const auto text = ReadTextFile(TestDataFile(file));
        ASSERT_FALSE(text.empty());

        const auto data = file.ends_with(".json") ? FilterDataFromLegacyJson(text) : FilterDataFromLegacyXml(text);
        EXPECT_EQ(data.name, "Überblick: errors & warnings");
        ASSERT_EQ(data.filter.messageFilters.size(), 10);
        ASSERT_EQ(data.filter.processFilters.size(), 2);
        EXPECT_EQ(data.filter.messageFilters[3].bgColor, Colors::Auto);
        EXPECT_EQ(data.filter.messageFilters[2].text, "a & b <c> \"quoted\" 'single'");
        EXPECT_FALSE(data.filter.messageFilters[2].enable);
    }
}

TEST(DebugViewPlusPlusTest, LegacyReadersHandleMalformedInput)
{
    EXPECT_THROW(FilterDataFromLegacyXml(""), std::runtime_error);
    EXPECT_THROW(FilterDataFromLegacyXml("<Filter><Name>x</Filter>"), std::runtime_error);
    EXPECT_THROW(FilterDataFromLegacyXml("<Other/>"), std::runtime_error);
    EXPECT_THROW(FilterDataFromLegacyXml("<Filter><Name>x</Name><MessageFilters/></Filter>"), std::runtime_error); // no ProcessFilters
    EXPECT_THROW(FilterDataFromLegacyJson(""), std::runtime_error);
    EXPECT_THROW(FilterDataFromLegacyJson("{\"Filter\": {"), std::runtime_error);

    // no filters at all is fine: an empty element, and the empty string property_tree wrote for it in JSON
    const auto xml = FilterDataFromLegacyXml("<?xml version=\"1.0\" encoding=\"utf-8\"?><Filter><Name>n</Name><MessageFilters/><ProcessFilters></ProcessFilters></Filter>");
    EXPECT_EQ(xml.name, "n");
    EXPECT_TRUE(xml.filter.messageFilters.empty());
    const auto json = FilterDataFromLegacyJson(R"({"Filter": {"Name": "n", "MessageFilters": "", "ProcessFilters": ""}})");
    EXPECT_EQ(json.name, "n");
    EXPECT_TRUE(json.filter.processFilters.empty());

    // text is kept as it is, also a single space; numbers and booleans may have white space around them
    const auto spaces = FilterDataFromLegacyXml(
        "<Filter><Name>n</Name><MessageFilters>"
        "<Filter><Enable> true </Enable><Text> </Text><MatchType>Simple</MatchType><FilterType>Include</FilterType>"
        "<BackColor><Red> 1</Red><Green>2 </Green><Blue>3</Blue></BackColor><TextColor Auto=\"true\"/></Filter>"
        "</MessageFilters><ProcessFilters/></Filter>");
    ASSERT_EQ(spaces.filter.messageFilters.size(), 1);
    EXPECT_EQ(spaces.filter.messageFilters[0].text, " ");
    EXPECT_EQ(spaces.filter.messageFilters[0].bgColor, (RGB(1, 2, 3)));
    EXPECT_EQ(spaces.filter.messageFilters[0].fgColor, Colors::Auto);
}

// Filter files are JSON. A file of an older version is converted when it is loaded, and the old file is kept as .archived
struct TempDirectory
{
    TempDirectory() :
        path(std::filesystem::temp_directory_path() / ("dvpp_test_" + std::to_string(GetCurrentProcessId()) + "_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())))
    {
        std::filesystem::create_directories(path);
    }

    ~TempDirectory()
    {
        std::error_code ignored;
        std::filesystem::remove_all(path, ignored);
    }

    std::filesystem::path path;
};

std::string ReadWholeFile(const std::filesystem::path& file)
{
    std::ifstream stream(file, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
}

TEST(DebugViewPlusPlusTest, LegacyFilterFileIsConvertedWhenLoaded)
{
    for (const std::string file : {"filters_legacy.xml", "filters_legacy.json"})
    {
        SCOPED_TRACE(file);
        TempDirectory dir;
        const auto source = dir.path / file;
        std::filesystem::copy_file(TestDataFile(file), source);
        const auto original = ReadWholeFile(source);
        const auto target = dir.path / "filters_legacy.json";

        std::string message;
        const auto data = LoadFilterFile(source, message);
        EXPECT_EQ(data.filter.messageFilters.size(), 10);
        EXPECT_FALSE(message.empty());

        // the old file is still there, byte for byte, under another name
        const auto archived = std::filesystem::path(source.string() + ".archived");
        ASSERT_TRUE(std::filesystem::exists(archived));
        EXPECT_EQ(ReadWholeFile(archived), original);

        // and the filters are JSON now, that loads without a conversion
        ASSERT_TRUE(std::filesystem::exists(target));
        EXPECT_EQ(DetectConfigFormat(ReadWholeFile(target)), ConfigFormat::Json);
        std::string secondMessage;
        const auto again = LoadFilterFile(target, secondMessage);
        EXPECT_TRUE(secondMessage.empty());
        EXPECT_EQ(again.name, data.name);
        EXPECT_TRUE((SameDefinition(again.filter, data.filter)));
        EXPECT_EQ(again.filter.messageFilters[3].bgColor, Colors::Auto);
    }
}

TEST(DebugViewPlusPlusTest, LegacyFilterFileIsNotConvertedOverAnExistingFile)
{
    TempDirectory dir;
    const auto source = dir.path / "filters.xml";
    const auto target = dir.path / "filters.json";
    std::filesystem::copy_file(TestDataFile("filters_legacy.xml"), source);
    { std::ofstream(target) << "precious"; }

    std::string message;
    const auto data = LoadFilterFile(source, message);
    EXPECT_EQ(data.filter.messageFilters.size(), 10); // still loaded
    EXPECT_FALSE(message.empty());
    EXPECT_TRUE(std::filesystem::exists(source)); // and nothing was renamed or overwritten
    EXPECT_FALSE(std::filesystem::exists(source.string() + ".archived"));
    EXPECT_EQ(ReadWholeFile(target), "precious");
}

TEST(DebugViewPlusPlusTest, ArchiveNamesAreNumberedAndTheLegacyContentIsNeverLost)
{
    TempDirectory dir;
    const auto file = dir.path / "x.dbconf";
    { std::ofstream(file) << "<first/>"; }
    { std::ofstream(dir.path / "x.dbconf.archived") << "<older/>"; }

    const auto message = ConvertLegacyFile(file, file, "{}");
    EXPECT_FALSE(message.empty());
    EXPECT_EQ(ReadWholeFile(file), "{}");
    EXPECT_EQ(ReadWholeFile(dir.path / "x.dbconf.archived"), "<older/>");
    EXPECT_EQ(ReadWholeFile(dir.path / "x.dbconf.archived2"), "<first/>");
    EXPECT_FALSE(std::filesystem::exists(dir.path / "x.dbconf.new"));

    // a target in a folder that does not exist: reported, and the legacy file stays where it is
    { std::ofstream(dir.path / "y.xml") << "<y/>"; }
    const auto failed = ConvertLegacyFile(dir.path / "y.xml", dir.path / "nowhere" / "y.json", "{}");
    EXPECT_FALSE(failed.empty());
    EXPECT_EQ(ReadWholeFile(dir.path / "y.xml"), "<y/>");
}

TEST(DebugViewPlusPlusTest, FilterFilesSaveAndLoad)
{
    TempDirectory dir;
    LogFilter filter;
    filter.messageFilters.push_back(MakeFilter("Überblick", MatchType::Simple, FilterType::Include, RGB(1, 2, 3), Colors::Auto));
    const auto file = dir.path / "mine.json";
    SaveFilterFile(file, "näme", filter);
    EXPECT_EQ(DetectConfigFormat(ReadWholeFile(file)), ConfigFormat::Json);

    std::string message;
    const auto data = LoadFilterFile(file, message);
    EXPECT_TRUE(message.empty());
    EXPECT_EQ(data.name, "näme");
    EXPECT_TRUE((SameDefinition(data.filter, filter)));

    EXPECT_THROW(LoadFilterFile(dir.path / "missing.json", message), std::runtime_error);
    { std::ofstream(dir.path / "other.json") << "{\"something\": 1}"; }
    EXPECT_THROW(LoadFilterFile(dir.path / "other.json", message), std::runtime_error);
    { std::ofstream(dir.path / "text.json") << "just text"; }
    EXPECT_THROW(LoadFilterFile(dir.path / "text.json", message), std::runtime_error);
}

// the session files: the golden files of the old version, read with the legacy reader
TEST(DebugViewPlusPlusTest, LegacySessionReaderReadsTheGoldenFiles)
{
    for (const std::string file : {"session_input.dbconf", "session_saved_by_app.dbconf"})
    {
        SCOPED_TRACE(file);
        const auto config = SessionConfigFromLegacyXml(ReadTextFile(TestDataFile(file)));
        EXPECT_FALSE(config.autoNewline);
        EXPECT_TRUE(config.linkViews);

        ASSERT_EQ(config.views.size(), 2);
        const auto& first = config.views[0];
        EXPECT_EQ(first.name, "Errors");
        EXPECT_TRUE(first.clockTime);
        EXPECT_FALSE(first.processColors);
        EXPECT_EQ(first.filters.messageFilters.size(), 10u);
        EXPECT_EQ(first.filters.processFilters.size(), 2u);
        EXPECT_EQ(first.filters.messageFilters[3].bgColor, Colors::Auto);
        EXPECT_EQ(first.filters.messageFilters[3].text, "id=(\\d+)");

        ASSERT_TRUE(first.columns.has_value());
        ASSERT_EQ(first.columns->size(), 7u);
        EXPECT_EQ((*first.columns)[1].index, 1);
        EXPECT_EQ(*(*first.columns)[1].width, 111);
        EXPECT_EQ(*(*first.columns)[3].enable, false);
        EXPECT_EQ(*(*first.columns)[6].width, 1234);

        const auto& second = config.views[1];
        EXPECT_EQ(second.name, "Überblick: errors & warnings");
        EXPECT_FALSE(second.clockTime);
        EXPECT_TRUE(second.processColors);
        EXPECT_TRUE(second.filters.messageFilters.empty());
        // the file the application saved has the columns of the second view, the hand written one does not
        EXPECT_EQ(second.columns.has_value(), (file == "session_saved_by_app.dbconf"));

        ASSERT_EQ(config.sources.size(), 2);
        EXPECT_EQ(config.sources[0].type, SourceType::Udp);
        EXPECT_EQ(config.sources[0].port, 2021);
        EXPECT_FALSE(config.sources[0].enabled);
        EXPECT_EQ(config.sources[0].description, "Test UDP source");
        EXPECT_EQ(config.sources[1].type, SourceType::DebugViewAgent);
        EXPECT_EQ(config.sources[1].address, "build-server.example.org");
    }
}

TEST(DebugViewPlusPlusTest, SessionConfigJsonRoundTrip)
{
    for (const std::string file : {"session_input.dbconf", "session_saved_by_app.dbconf"})
    {
        SCOPED_TRACE(file);
        const auto config = SessionConfigFromLegacyXml(ReadTextFile(TestDataFile(file)));
        const auto json = SessionConfigToJson(config);
        EXPECT_EQ(DetectConfigFormat(json), ConfigFormat::Json);

        const auto again = SessionConfigFromJson(json);
        EXPECT_EQ(SessionConfigToJson(again), json); // nothing is lost or changed
        ASSERT_EQ(again.views.size(), config.views.size());
        for (size_t i = 0; i < config.views.size(); ++i)
        {
            EXPECT_EQ(again.views[i].name, config.views[i].name);
            EXPECT_TRUE((SameDefinition(again.views[i].filters, config.views[i].filters)));
            EXPECT_EQ(again.views[i].columns.has_value(), config.views[i].columns.has_value());
        }
        ASSERT_EQ(again.sources.size(), config.sources.size());
        EXPECT_EQ(again.sources[1].address, config.sources[1].address);
    }
}

TEST(DebugViewPlusPlusTest, LegacySessionViewsAndSourcesAreOrderedByIndex)
{
    const auto config = SessionConfigFromLegacyXml(
        "<DebugViewPP><AutoNewline>true</AutoNewline><LinkViews>false</LinkViews><Views>"
        "<View><Index>1</Index><Name>second</Name><ClockTime>true</ClockTime><ProcessColors>false</ProcessColors><MessageFilters/><ProcessFilters/></View>"
        "<View><Index>0</Index><Name>first</Name><ClockTime>false</ClockTime><ProcessColors>true</ProcessColors><MessageFilters/><ProcessFilters/>"
        "<Columns><Column><Index>2</Index><Width>50</Width></Column><Column><Enable>true</Enable></Column></Columns></View>"
        "</Views><Sources>"
        "<Source><Index>1</Index><Enabled>true</Enabled><Description>b</Description><SourceType>File</SourceType><Address></Address><Port>0</Port></Source>"
        "<Source><Index>0</Index><Enabled>false</Enabled><Description>a</Description><SourceType>System</SourceType><Address></Address><Port>0</Port></Source>"
        "</Sources></DebugViewPP>");
    EXPECT_TRUE(config.autoNewline);
    EXPECT_FALSE(config.linkViews);
    ASSERT_EQ(config.views.size(), 2);
    EXPECT_EQ(config.views[0].name, "first");
    EXPECT_EQ(config.views[1].name, "second");
    EXPECT_FALSE(config.views[1].columns.has_value());

    // a column without an index is skipped, the fields that are missing stay unset
    ASSERT_TRUE(config.views[0].columns.has_value());
    ASSERT_EQ(config.views[0].columns->size(), 1u);
    EXPECT_EQ((*config.views[0].columns)[0].index, 2);
    EXPECT_EQ(*(*config.views[0].columns)[0].width, 50);
    EXPECT_FALSE((*config.views[0].columns)[0].enable.has_value());
    EXPECT_FALSE((*config.views[0].columns)[0].order.has_value());

    ASSERT_EQ(config.sources.size(), 2);
    EXPECT_EQ(config.sources[0].description, "a");
    EXPECT_EQ(config.sources[1].description, "b");

    // Sources is optional
    const auto noSources = SessionConfigFromLegacyXml(
        "<DebugViewPP><AutoNewline>true</AutoNewline><LinkViews>false</LinkViews><Views/></DebugViewPP>");
    EXPECT_TRUE(noSources.views.empty());
    EXPECT_TRUE(noSources.sources.empty());
}

TEST(DebugViewPlusPlusTest, SessionConfigErrors)
{
    EXPECT_THROW(SessionConfigFromLegacyXml(""), std::runtime_error);
    EXPECT_THROW(SessionConfigFromLegacyXml("<Other/>"), std::runtime_error);
    EXPECT_THROW(SessionConfigFromLegacyXml("<DebugViewPP><AutoNewline>maybe</AutoNewline><LinkViews>true</LinkViews><Views/></DebugViewPP>"), std::runtime_error);
    EXPECT_THROW(SessionConfigFromLegacyXml("<DebugViewPP><AutoNewline>true</AutoNewline><LinkViews>true</LinkViews></DebugViewPP>"), std::runtime_error); // no Views
    EXPECT_THROW(SessionConfigFromJson(""), std::runtime_error);
    EXPECT_THROW(SessionConfigFromJson("[]"), std::runtime_error);
    EXPECT_THROW(SessionConfigFromJson(R"({"version": 3})"), std::runtime_error);
    EXPECT_THROW(SessionConfigFromJson(R"({"views": {}})"), std::runtime_error);
    EXPECT_THROW(SessionConfigFromJson(R"({"views": [{"clockTime": true}]})"), std::runtime_error); // no name
    EXPECT_THROW(SessionConfigFromJson(R"({"views": [{"name": 5}]})"), std::runtime_error);
    EXPECT_THROW(SessionConfigFromJson(R"({"views": [{"name": "a", "columns": [{"width": 1}]}]})"), std::runtime_error); // column without index
    EXPECT_THROW(SessionConfigFromJson(R"({"sources": [{"description": "a", "type": "no such type"}]})"), std::exception);

    // the minimum is fine
    const auto minimal = SessionConfigFromJson("{}");
    EXPECT_TRUE(minimal.views.empty());
    const auto view = SessionConfigFromJson(R"({"views": [{"name": "a"}]})");
    ASSERT_EQ(view.views.size(), 1);
    EXPECT_TRUE(view.views[0].clockTime);
    EXPECT_FALSE(view.views[0].columns.has_value());
}

TEST(DebugViewPlusPlusTest, LegacySessionFileIsConvertedWhenLoaded)
{
    TempDirectory dir;
    const auto file = dir.path / "mine.dbconf";
    std::filesystem::copy_file(TestDataFile("session_saved_by_app.dbconf"), file);
    const auto original = ReadWholeFile(file);

    std::string message;
    const auto config = LoadSessionFile(file, message);
    EXPECT_EQ(config.views.size(), 2u);
    EXPECT_FALSE(message.empty());

    // the same name is JSON now, the XML is kept
    EXPECT_EQ(DetectConfigFormat(ReadWholeFile(file)), ConfigFormat::Json);
    ASSERT_TRUE(std::filesystem::exists(dir.path / "mine.dbconf.archived"));
    EXPECT_EQ(ReadWholeFile(dir.path / "mine.dbconf.archived"), original);

    std::string secondMessage;
    const auto again = LoadSessionFile(file, secondMessage);
    EXPECT_TRUE(secondMessage.empty());
    EXPECT_EQ(SessionConfigToJson(again), SessionConfigToJson(config));

    // saving and loading
    SaveSessionFile(dir.path / "saved.dbconf", config);
    EXPECT_EQ((SessionConfigToJson(LoadSessionFile(dir.path / "saved.dbconf", secondMessage))), SessionConfigToJson(config));
    EXPECT_THROW(LoadSessionFile(dir.path / "missing.dbconf", message), std::runtime_error);
    { std::ofstream(dir.path / "text.dbconf") << "not a configuration"; }
    EXPECT_THROW(LoadSessionFile(dir.path / "text.dbconf", message), std::runtime_error);
}

// the regular expressions of the filters: PCRE2
TEST(DebugViewPlusPlusTest, RegexSearch)
{
    EXPECT_TRUE(Regex("error", true).Search("An ERROR occurred"));
    EXPECT_FALSE(Regex("error", false).Search("An ERROR occurred")); // RegexCase
    EXPECT_TRUE(Regex("ERROR", false).Search("An ERROR occurred"));
    EXPECT_FALSE(Regex("^error", true).Search("An error occurred"));
    EXPECT_TRUE(Regex("occurred$", true).Search("An error occurred"));
    EXPECT_FALSE(Regex("occurred$", true).Search("An error occurred\n")); // like ECMAScript: $ is the end of the text
    EXPECT_TRUE(Regex(R"(\d+\.\d+)", true).Search("version 1.22"));
    EXPECT_TRUE(Regex("a.c", true).Search("abc"));
    EXPECT_FALSE(Regex("a.c", true).Search("a\nc"));
    EXPECT_TRUE(Regex("", true).Search("anything")); // the empty pattern matches everything
    EXPECT_TRUE(Regex().Search(""));
    EXPECT_TRUE(Regex().Search("anything"));
    EXPECT_TRUE(Regex("a|b", true).Search("xxb"));
    EXPECT_FALSE(Regex("x", true).Search(""));

    // the patterns of the Simple and Wildcard filters
    EXPECT_TRUE(Regex(MakePattern(MatchType::Simple, "a.b(c)[d]{e}+*?|^$\\"), true).Search("x a.b(c)[d]{e}+*?|^$\\ y"));
    EXPECT_FALSE(Regex(MakePattern(MatchType::Simple, "a.b"), true).Search("axb"));
    EXPECT_TRUE(Regex(MakePattern(MatchType::Wildcard, "error*done"), true).Search("error it is done"));
    EXPECT_TRUE(Regex(MakePattern(MatchType::Wildcard, "a?c"), true).Search("abc"));
    EXPECT_FALSE(Regex(MakePattern(MatchType::Wildcard, "a?c"), true).Search("ac"));

    // bytes that are not text
    EXPECT_TRUE(Regex("b", true).Search(std::string("a\0b", 3)));
    EXPECT_TRUE(Regex("caf", true).Search("caf\xE9 invalid utf-8"));
}

// #411
TEST(DebugViewPlusPlusTest, RegexLookbehindAndOtherPcre2Syntax)
{
    RegexMatch match;

    EXPECT_TRUE(Regex(R"((?<=id=)\d+)", true).Search("request id=4711 done", match));
    EXPECT_EQ(match.Group(), "4711");
    EXPECT_EQ(match.Position(), 11u);

    EXPECT_TRUE(Regex(R"((?<!id=)\d+)", true).Search("id=12 and 34", match));
    EXPECT_EQ(match.Group(), "2"); // the "1" follows "id=", the "2" does not
    EXPECT_FALSE(Regex(R"((?<=id=)\d+)", true).Search("no number: id=x"));

    EXPECT_TRUE(Regex(R"(foo(?=bar))", true).Search("foobar", match)); // lookahead
    EXPECT_EQ(match.Group(), "foo");
    EXPECT_FALSE(Regex(R"(foo(?!bar))", true).Search("foobar"));
    EXPECT_TRUE(Regex(R"((\w)\1)", true).Search("hello")); // backreference
    EXPECT_FALSE(Regex(R"((\w)\1)", true).Search("helo"));
    EXPECT_TRUE(Regex(R"((?<word>\w+) \k<word>)", true).Search("this is is a test")); // named group
    EXPECT_TRUE(Regex(R"(foo\Kbar)", true).Search("foobar", match));
    EXPECT_EQ(match.Group(), "bar");
    EXPECT_TRUE(Regex(R"((?i)ERROR(?-i)X)", false).Search("errorX"));
}

TEST(DebugViewPlusPlusTest, RegexGroups)
{
    RegexMatch match;
    Regex groups(R"((\w+)=(\d+)(;(x))?)", true);
    EXPECT_TRUE(groups.Search("name=42;x other", match));
    ASSERT_EQ(match.size(), 5u); // the whole match and 4 groups
    EXPECT_EQ(match.Group(0), "name=42;x");
    EXPECT_EQ(match.Group(1), "name");
    EXPECT_EQ(match.Group(2), "42");
    EXPECT_EQ(match.Group(3), ";x");
    EXPECT_EQ(match.Group(4), "x");
    EXPECT_FALSE(match.Matched(9));

    EXPECT_TRUE(groups.Search("name=42 other", match));
    ASSERT_EQ(match.size(), 5u); // groups that did not take part are there, and empty
    EXPECT_EQ(match.Group(0), "name=42");
    EXPECT_TRUE(match.Matched(2));
    EXPECT_FALSE(match.Matched(3));
    EXPECT_FALSE(match.Matched(4));
    EXPECT_EQ(match.Group(4), "");
    EXPECT_EQ(match.Length(4), 0u);
}

TEST(DebugViewPlusPlusTest, RegexSearchFromAnOffset)
{
    RegexMatch match;
    Regex lookbehind(R"((?<=a)b)", true);
    EXPECT_TRUE(lookbehind.Search("ab", match, 1)); // the 'a' before the offset is seen
    EXPECT_EQ(match.Position(), 1u);
    EXPECT_FALSE(Regex("^b", true).Search("ab", match, 1)); // and ^ is not at the offset
    EXPECT_TRUE(Regex(R"(\bb)", true).Search("a b", match, 2));
    EXPECT_FALSE(Regex("a", true).Search("a", match, 5)); // after the end
    EXPECT_TRUE(Regex("", true).Search("abc", match, 3));
    EXPECT_FALSE(Regex("", true).Search("abc", match, 4));
}

TEST(DebugViewPlusPlusTest, RegexForEachMatch)
{
    std::vector<std::string> found;
    Regex(R"(\d+)", true).ForEachMatch("a1 bb22 ccc333", [&](const RegexMatch& m) { found.push_back(std::string(m.Group())); });
    EXPECT_EQ(found, (std::vector<std::string>{"1", "22", "333"}));

    // matches without length are found once per position, and the search ends
    found.clear();
    Regex("x*", true).ForEachMatch("axxb", [&](const RegexMatch& m) { found.push_back(std::string(m.Group())); });
    EXPECT_EQ(found, (std::vector<std::string>{"", "xx", "", ""}));

    found.clear();
    Regex(R"((?<=\d)[a-z])", true).ForEachMatch("1a 2b c3d", [&](const RegexMatch& m) { found.push_back(std::string(m.Group())); });
    EXPECT_EQ(found, (std::vector<std::string>{"a", "b", "d"}));

    int count = 0;
    Regex("a", true).ForEachMatch("", [&](const RegexMatch&) { ++count; });
    EXPECT_EQ(count, 0);
}

TEST(DebugViewPlusPlusTest, RegexErrors)
{
    EXPECT_THROW(Regex("(", true), RegexError);
    EXPECT_THROW(Regex("[a-", true), RegexError);
    EXPECT_THROW(Regex("*a", true), RegexError);
    EXPECT_THROW(Regex(R"((?<=a+)b)", true), RegexError); // lookbehind must have a fixed length
    EXPECT_THROW(Regex(R"(\1)", true), RegexError);

    try
    {
        Regex("ab(cd", true);
        FAIL() << "no exception";
    }
    catch (const RegexError& error)
    {
        EXPECT_NE(std::string(error.what()).find("missing closing parenthesis"), std::string::npos) << error.what();
        EXPECT_EQ(error.offset(), 5u);
    }
    EXPECT_NO_THROW(Regex("a{", true)); // a '{' that is not a quantifier is a character
}

TEST(DebugViewPlusPlusTest, RegexCopiesAndThreads)
{
    Regex original(R"(a+b)", true);
    Regex copy = original;
    Regex assigned;
    assigned = original;
    EXPECT_TRUE(copy.Search("xaab"));
    EXPECT_TRUE(assigned.Search("xaab"));
    EXPECT_FALSE(original.Search("xaac"));

    std::atomic<int> matches{0};
    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t)
    {
        threads.emplace_back([&] {
            RegexMatch match;
            for (int i = 0; i < 20000; ++i)
            {
                if (original.Search("xxaaab", match) && match.Group() == "aaab")
                {
                    ++matches;
                }
            }
        });
    }
    for (auto& thread : threads)
    {
        thread.join();
    }
    EXPECT_EQ(matches, 80000);
}

TEST(DebugViewPlusPlusTest, RegexThatBacktracksWithoutEndIsNoMatch)
{
    // (a+)+b on a long text of a's takes exponential time, the match limit ends it
    const auto start = std::chrono::steady_clock::now();
    EXPECT_FALSE(Regex("^(a+)+b", true).Search(std::string(60, 'a') + "c"));
    EXPECT_LT(std::chrono::steady_clock::now() - start, std::chrono::seconds(5));
}

TEST(DebugViewPlusPlusTest, FilterSameDefinition)
{
    LogFilter a;
    a.messageFilters.push_back(Filter("error", MatchType::Simple, FilterType::Include));
    a.messageFilters.push_back(Filter("noise.*", MatchType::Regex, FilterType::Exclude, RGB(1, 2, 3), RGB(4, 5, 6)));
    a.processFilters.push_back(Filter("my.exe", MatchType::Simple, FilterType::Highlight, RGB(255, 0, 0), RGB(0, 0, 0)));

    LogFilter same = a;
    EXPECT_TRUE((SameDefinition(a, same)));
    EXPECT_TRUE((SameDefinition(LogFilter(), LogFilter())));

    // the state of a filter is not a difference, it is not part of the definition
    same.messageFilters[0].matched = true;
    EXPECT_TRUE((SameDefinition(a, same)));

    // everything that decides what is shown or how is
    auto changed = [&](auto modify) {
        LogFilter b = a;
        modify(b);
        return !SameDefinition(a, b);
    };
    EXPECT_TRUE(changed([](LogFilter& f) { f.messageFilters[0].text = "errors"; }));
    EXPECT_TRUE(changed([](LogFilter& f) { f.messageFilters[0].filterType = FilterType::Exclude; }));
    EXPECT_TRUE(changed([](LogFilter& f) { f.messageFilters[1].matchType = MatchType::Simple; }));
    EXPECT_TRUE((changed([](LogFilter& f) { f.messageFilters[1].bgColor = RGB(9, 9, 9); })));
    EXPECT_TRUE((changed([](LogFilter& f) { f.messageFilters[1].fgColor = RGB(9, 9, 9); })));
    EXPECT_TRUE(changed([](LogFilter& f) { f.messageFilters[0].enable = false; }));
    EXPECT_TRUE(changed([](LogFilter& f) { f.messageFilters.pop_back(); }));
    EXPECT_TRUE(changed([](LogFilter& f) { f.processFilters.clear(); }));
    EXPECT_TRUE((changed([](LogFilter& f) { std::swap(f.messageFilters[0], f.messageFilters[1]); }))); // the order can matter
}

// #278: undo and redo of changes of the filters
TEST(DebugViewPlusPlusTest, UndoRedoStackTest)
{
    UndoRedoStack<std::string> history;
    std::string state = "none";
    EXPECT_FALSE(history.CanUndo());
    EXPECT_FALSE(history.CanRedo());
    EXPECT_FALSE(history.Undo(state)); // nothing to undo, nothing changes
    EXPECT_EQ(state, "none");
    EXPECT_FALSE(history.Redo(state));

    // three changes
    history.Push(state);
    state = "exclude A";
    history.Push(state);
    state = "exclude A, include B";
    history.Push(state);
    state = "exclude A, include B, highlight C";

    // undo them one by one
    EXPECT_TRUE(history.Undo(state));
    EXPECT_EQ(state, ("exclude A, include B"));
    EXPECT_TRUE(history.Undo(state));
    EXPECT_EQ(state, "exclude A");
    EXPECT_TRUE(history.CanRedo());

    // and redo one
    EXPECT_TRUE(history.Redo(state));
    EXPECT_EQ(state, ("exclude A, include B"));

    // a new change forgets what could be redone
    history.Push(state);
    state = "exclude A, include B, token D";
    EXPECT_FALSE(history.CanRedo());
    EXPECT_TRUE(history.Undo(state));
    EXPECT_EQ(state, ("exclude A, include B"));
    EXPECT_TRUE(history.Undo(state));
    EXPECT_EQ(state, "exclude A");
    EXPECT_TRUE(history.Undo(state));
    EXPECT_EQ(state, "none");
    EXPECT_FALSE(history.CanUndo());

    // only the last changes are remembered
    UndoRedoStack<int> limited(3);
    int value = 0;
    for (int i = 1; i <= 10; ++i)
    {
        limited.Push(value);
        value = i;
    }
    int undone = 0;
    while (limited.Undo(value))
    {
        ++undone;
    }
    EXPECT_EQ(undone, 3);
    EXPECT_EQ(value, 7); // the state before the last three changes

    history.Clear();
    EXPECT_FALSE(history.CanUndo());
}

// #368: the colors of the dark theme
TEST(DebugViewPlusPlusTest, DarkThemeColors)
{
    auto brightness = [](COLORREF c) { return (GetRValue(c) * 299 + GetGValue(c) * 587 + GetBValue(c) * 114) / 1000; };

    // the light theme has the system colors, colors are not changed
    Colors::SetDarkTheme(false);
    EXPECT_FALSE(Colors::IsDarkTheme());
    const COLORREF lightBackGround = Colors::BackGround;
    const COLORREF lightText = Colors::Text;
    EXPECT_EQ(lightBackGround, ::GetSysColor(COLOR_WINDOW));
    EXPECT_EQ((Colors::Adapt(RGB(200, 230, 250))), (RGB(200, 230, 250)));
    EXPECT_EQ((Colors::TextOn(RGB(200, 230, 250))), lightText);

    Colors::SetDarkTheme(true);
    EXPECT_TRUE(Colors::IsDarkTheme());
    EXPECT_LT(brightness(Colors::BackGround), 80);
    EXPECT_GT(brightness(Colors::Text), 160);
    EXPECT_GT(brightness(Colors::ItemHighlightText), brightness(Colors::ItemHighlight)); // selected lines can be read

    // light colors (random colors for processes and matches are light) are made dark, the text on them is light
    const COLORREF pastel = RGB(250, 240, 200);
    const COLORREF adapted = Colors::Adapt(pastel);
    EXPECT_LT(brightness(adapted), 120);
    EXPECT_GT(GetRValue(adapted), GetBValue(adapted)); // the hue is kept
    EXPECT_GT(brightness(Colors::TextOn(adapted)), 160);

    // the text color contrasts with the background, also for the colors that are not adapted (colors of filters)
    EXPECT_EQ((Colors::TextOn(RGB(255, 255, 200))), (RGB(0, 0, 0)));
    EXPECT_GT((brightness(Colors::TextOn(RGB(20, 20, 20)))), 200);
    EXPECT_NE(brightness(Colors::TextOn(Colors::Highlight)), brightness(Colors::Highlight)); // not unreadable

    // back to the system colors
    Colors::SetDarkTheme(false);
    EXPECT_EQ(Colors::BackGround, lightBackGround);
    EXPECT_EQ(Colors::Text, lightText);
}

// sizes are scaled for the monitor (#321)
TEST(DebugViewPlusPlusTest, DpiScaling)
{
    EXPECT_EQ((Win32::ScaleToDpi(16, 96)), 16); // 100%
    EXPECT_EQ((Win32::ScaleToDpi(16, 120)), 20); // 125%
    EXPECT_EQ((Win32::ScaleToDpi(16, 144)), 24); // 150%
    EXPECT_EQ((Win32::ScaleToDpi(100, 192)), 200); // 200%
    EXPECT_EQ((Win32::ScaleToDpi(0, 144)), 0);

    // moving from one monitor to another, and back, gives the same size
    EXPECT_EQ((Win32::ScaleBetweenDpi(60, 96, 144)), 90);
    EXPECT_EQ((Win32::ScaleBetweenDpi(90, 144, 96)), 60);
    EXPECT_EQ((Win32::ScaleBetweenDpi(-12, 96, 144)), -18); // the height of a font is negative
    EXPECT_EQ((Win32::ScaleBetweenDpi(60, 0, 144)), 60); // an unknown DPI does not scale

    // there is always a DPI, also without a window
    EXPECT_GE(Win32::GetDpiForWindow(nullptr), 48u);
    EXPECT_GE(Win32::GetDpiForWindow(::GetDesktopWindow()), 48u);
}

// SetPrivilege reports if the privilege could be changed, it is not an error when the token does not have the privilege (#412)
TEST(DebugViewPlusPlusTest, SetPrivilegeReportsMissingPrivilege)
{
    // every account has this privilege, and it is enabled by default
    EXPECT_TRUE((Win32::SetPrivilege(SE_CHANGE_NOTIFY_NAME, true)));

    // only the system has this privilege, an administrator does not
    EXPECT_FALSE((Win32::SetPrivilege(SE_TCB_NAME, true)));
}

TEST(DebugViewPlusPlusTest, HandleTest)
{
    static const HANDLE nullHandle = nullptr;

    HANDLE rawHandle = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId());
    Win32::Handle handle(rawHandle);
    {
        EXPECT_EQ(handle.get(), rawHandle);
        Win32::Handle handle1(std::move(handle));
        EXPECT_EQ(handle.get(), nullHandle);
        EXPECT_EQ(handle1.get(), rawHandle);
        {
            Win32::Handle handle2(std::move(handle1));
            handle2.release();
            EXPECT_EQ(handle2.get(), nullHandle);
        }
        EXPECT_EQ(handle1.get(), nullHandle);
    }

    {
        Win32::Handle handle1;
        {
            HANDLE zero = nullptr;
            Win32::Handle zeroHandle(zero);
            EXPECT_EQ(zeroHandle.get(), nullHandle);
            handle1 = std::move(zeroHandle);
            EXPECT_EQ(zeroHandle.get(), nullHandle);
            EXPECT_EQ(handle1.get(), nullHandle);
        }
        // this scope ensures having a Handle leave scope that had its guts ripped out by std::move
        // will not cause nullpointers or exeptions
    }
}

TEST(DebugViewPlusPlusTest, LineBufferTest1)
{
    TestLineBuffer buffer(64);
    FILETIME ft;
    ft.dwLowDateTime = 42;
    ft.dwHighDateTime = 43;
    buffer.Add(42.0, ft, nullptr, "test", nullptr);

    auto lines = buffer.GetLines();
    auto& line = lines[0];
    EXPECT_EQ(line.time, 42.0);
    EXPECT_EQ(line.systemTime.dwLowDateTime, DWORD(42));
    EXPECT_EQ(line.systemTime.dwHighDateTime, DWORD(43));
    EXPECT_TRUE(buffer.Empty());
}

TEST(DebugViewPlusPlusTest, LineBufferTest2) // this test overflows a GUI test runner with log-output, locking it up temporarily
{
    TestLineBuffer buffer(600);
    Timer timer;
    std::cout << "LineBufferTest2 running..." << std::endl;
    for (int j = 0; j < 100; ++j)
    {
        // SCOPED_TRACE(::testing::Message() << "j: " << j << "\n");

        for (int i = 0; i < 17; ++i)
        {
            // SCOPED_TRACE(::testing::Message() << "i: " << i << "\n");
            FILETIME ft;
            ft.dwLowDateTime = 43;
            ft.dwHighDateTime = 44;
            buffer.Add(42.0, ft, nullptr, "test", nullptr);
        }

        auto lines = buffer.GetLines();
        EXPECT_EQ(lines.size(), 17);
        for (auto& line : lines)
        {
            EXPECT_EQ(line.time, 42.0);
            EXPECT_EQ(line.systemTime.dwLowDateTime, DWORD(43));
            EXPECT_EQ(line.systemTime.dwHighDateTime, DWORD(44));
        }
    }
}

TEST(DebugViewPlusPlusTest, IndexedStorageRandomAccess)
{
    using namespace indexedstorage;

    size_t testSize = 10000;
    auto testMax = testSize - 1;

    std::mt19937 generator;
    std::uniform_int_distribution<size_t> distribution(0, testMax);
    SnappyStorage s;
    for (size_t i = 0; i < testSize; ++i)
        s.Add(GetTestString(i));

    // random access can be somewhat slow in debug-mode, this is expected behaviour
    bool failed = false;
    for (size_t i = 0; i < testSize / 10; ++i)
    {
        size_t j = distribution(generator); // generates number in the range 0..testMax
        if (s[j] != GetTestString(j))
        {
            failed = true;
            break;
        };
    }
    EXPECT_FALSE(failed);
}

TEST(DebugViewPlusPlusTest, IndexedStorageCompression)
{
    using namespace indexedstorage;

    // the memory allocator will mess up test results is < 64 kb is allocated
    // make sure SnappyStorage allocates at least ~500kb for reproducable results

    // this test is indicative only, on average the SnappyStorage should allocate at most 50% of memory compared to a normal vector.
    // since GetTestString returns an overly simpe-to-compress string, it will appear to perform insanely good.

    size_t testSize = 100000;
    VectorStorage v;
    SnappyStorage s;

    size_t m0 = ProcessInfo::GetPrivateBytes();

    for (size_t i = 0; i < testSize; ++i)
        v.Add(GetTestString(i));

    size_t m1 = ProcessInfo::GetPrivateBytes();
    size_t usedByVector = m1 - m0;
    SCOPED_TRACE(::testing::Message() << "VectorStorage requires: " << usedByVector / 1024 << " kB");

    for (size_t i = 0; i < testSize; ++i)
        s.Add(GetTestString(i));

    size_t m2 = ProcessInfo::GetPrivateBytes();
    size_t usedBySnappy = m2 - m1;

    SCOPED_TRACE(::testing::Message() << "SnappyStorage requires: " << usedBySnappy / 1024 << " kB (" << (100 * usedBySnappy) / usedByVector << "%)");
    EXPECT_GT(size_t(0.50 * usedByVector), usedBySnappy);
}

// execute as:
// "DebugviewppTest.exe" --gtest_filter=*LogSourcesReceiveMessages
TEST(DebugViewPlusPlusTest, IndexedStorageDropFront)
{
    using namespace indexedstorage;
    SnappyStorage storage;
    for (int i = 0; i < 1000; ++i)
    {
        storage.Add(std::to_string(i));
    }
    EXPECT_EQ(storage[0], "0"); // block 0 is the block that is read last, it must not be used after it was dropped

    // the block size is 400, so there are 2 complete blocks (0..799), the others are in the block that is being written
    EXPECT_EQ(storage.DropFront(100), 0u);   // not a complete block
    EXPECT_EQ(storage.DropFront(500), 400u); // one block
    EXPECT_EQ(storage[400], "400");
    EXPECT_EQ(storage[799], "799");
    EXPECT_EQ(storage[999], "999");

    EXPECT_EQ(storage.DropFront(100000), 400u); // only the complete blocks that are left
    EXPECT_EQ(storage[800], "800");
    EXPECT_EQ(storage[999], "999");

    // adding continues with the next index
    EXPECT_EQ(storage.Add("next"), 1000u);
    EXPECT_EQ(storage[1000], "next");
}

TEST(DebugViewPlusPlusTest, LogFileHistorySize)
{
    FILETIME time = {};

    // unlimited by default
    LogFile unlimited;
    for (int i = 0; i < 5000; ++i)
    {
        unlimited.Add(Message(i, time, 1, "process", std::to_string(i)));
    }
    EXPECT_EQ(unlimited.BeginIndex(), 0);
    EXPECT_EQ(unlimited.EndIndex(), 5000);
    EXPECT_EQ(unlimited.Count(), 5000);

    // with a history size the oldest messages are removed, the index of the others stays the same
    LogFile logFile;
    logFile.SetHistorySize(1000);
    for (int i = 0; i < 5000; ++i)
    {
        logFile.Add(Message(i, time, 1, "process", std::to_string(i)));
    }
    EXPECT_EQ(logFile.EndIndex(), 5000);
    EXPECT_GE(logFile.Count(), 1000);
    EXPECT_LT(logFile.Count(), 1000 + 400); // messages are removed in blocks of 400
    EXPECT_GT(logFile.BeginIndex(), 0);
    EXPECT_EQ(logFile.BeginIndex(), logFile.EndIndex() - logFile.Count());
    EXPECT_EQ(logFile[logFile.BeginIndex()].text, std::to_string(logFile.BeginIndex()));
    EXPECT_EQ(logFile[4999].text, "4999");
    EXPECT_EQ(logFile[4500].text, "4500");

    // a lower history size removes messages at once
    unlimited.SetHistorySize(500);
    EXPECT_EQ(unlimited.EndIndex(), 5000);
    EXPECT_GE(unlimited.Count(), 500);
    EXPECT_LT(unlimited.Count(), 500 + 400);
    EXPECT_EQ(unlimited[4999].text, "4999");

    // Clear starts again
    logFile.Clear();
    EXPECT_EQ(logFile.BeginIndex(), 0);
    EXPECT_EQ(logFile.EndIndex(), 0);
    logFile.Add(Message(0, time, 1, "process", "again"));
    EXPECT_EQ(logFile[0].text, "again");
}

TEST(DebugViewPlusPlusTest, LogSourcesReceiveMessages)
{
    ActiveExecutorClient executor;
    LogSources logsources(executor, false);
    LogSource* logsource;
    executor.Call([&] { logsource = logsources.AddTestSource(); });

    Timer timer;
    logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "message 1");
    logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "message 2");
    logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "message 3");
    SCOPED_TRACE("3 lines added.");

    Lines lines;
    executor.Call([&] { lines = logsources.GetLines(); });
    SCOPED_TRACE(::testing::Message() << "received: " << lines.size() << " lines.");

    EXPECT_EQ(lines.size(), 3);

    for (auto& line : lines)
    {
        SCOPED_TRACE(::testing::Message() << "line: " << line.message);
    }

    const int testsize = 1000;
    SCOPED_TRACE(::testing::Message() << "Write " << testsize << " lines...");
    for (int i = 0; i < testsize; ++i)
    {
        logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "TESTSTRING 1234\n");
    }

    Lines morelines;
    executor.Call([&] { morelines = logsources.GetLines(); });

    SCOPED_TRACE(::testing::Message() << "received: " << morelines.size() << " lines.");
    EXPECT_EQ(morelines.size(), testsize);
}

TEST(DebugViewPlusPlusTest, LogSourcesCharacterPreservation)
{
    ActiveExecutorClient executor;
    LogSources logsources(executor, false);
    LogSource* logsource;
    executor.Call([&] { logsource = logsources.AddTestSource(); });

    Timer timer;
    logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "TrailingSpace ");
    logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "TrailingTab\t");

    Lines lines;
    executor.Call([&] { lines = logsources.GetLines(); });
    EXPECT_EQ(lines.size(), 2);
    EXPECT_EQ(lines[0].message, "TrailingSpace "); // space preserved
    EXPECT_EQ(lines[1].message, "TrailingTab\t");  // tab preserved
}

TEST(DebugViewPlusPlusTest, LogSourcesTabHandling)
{
    ActiveExecutorClient executor;
    LogSources logsources(executor, false);
    LogSource* logsource;
    executor.Call([&] { logsource = logsources.AddTestSource(); });

    Timer timer;
    logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "\tTabPrefix");
    logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "\t\tTwoTabsPrefixed");

    Lines lines;
    executor.Call([&] { lines = logsources.GetLines(); });
    EXPECT_EQ(lines.size(), 2);
    EXPECT_EQ(lines[0].message, "\tTabPrefix");         // space preserved
    EXPECT_EQ(lines[1].message, "\t\tTwoTabsPrefixed"); // space preserved
}

TEST(DebugViewPlusPlusTest, LogSourcesNewLineHandling)
{
    ActiveExecutorClient executor;
    LogSources logsources(executor, false);
    LogSource* logsource;
    executor.Call([&] { logsource = logsources.AddTestSource(); });

    Timer timer;
    logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "NewLinePostfix\n");
    logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "NewLineCarriageReturnPostfix\n\r");
    logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "CarriageReturnNewLinePostfix\r\n");

    Lines lines;
    executor.Call([&] { lines = logsources.GetLines(); });
    EXPECT_EQ(lines.size(), 3);
    EXPECT_EQ(lines[0].message, "NewLinePostfix");
    EXPECT_EQ(lines[1].message, "NewLineCarriageReturnPostfix");
    EXPECT_EQ(lines[2].message, "CarriageReturnNewLinePostfix");
}

std::wstring GetExecutePath()
{
    auto path = absolute(std::filesystem::path(Win32::GetCommandLineArguments()[0]));
    return path.remove_filename().c_str();
}

TEST(DebugViewPlusPlusTest, LogSourceLoopback)
{
    using namespace std::chrono_literals;

    Lines lines;
    auto executor = std::make_unique<ActiveExecutorClient>();
    {
        LogSources logsources(*executor, true);
        executor->Call([&] { logsources.AddDBWinReader(false); });
        executor->Call([&] { logsources.AddTestSource(); });
        executor->Call([&] { logsources.SetAutoNewLine(true); });

        executor->Call([&] { logsources.AddMessage("Test message 1"); });
        executor->Call([&] { logsources.AddMessage("Test message 2"); });
        std::this_thread::sleep_for(200ms);
        executor->Call([&] { lines = logsources.GetLines(); });

        // Abort will remove the source and no messages can be received from remove sources
        executor->Call([&] { logsources.Abort(); });
    }
    executor.reset();

    // the DBWIN reader is active too, other programs may have written to the shared buffer: only count the messages of this test
    const auto count = std::count_if(lines.begin(), lines.end(), [](const Line& line) { return line.message.starts_with("Test message"); });
    EXPECT_EQ(count, 2);
}

// The DBWIN_BUFFER is shared by the whole machine, other programs (Teams, game launchers, ...) send their messages to it as well,
// and those end up between the messages of the test. Only look at the messages of the process that is expected to send them.
Lines OnlyFromProcess(Lines lines, DWORD pid)
{
    lines.erase(std::remove_if(lines.begin(), lines.end(), [pid](const Line& line) { return line.pid != pid; }), lines.end());
    return lines;
}

TEST(DebugViewPlusPlusTest, LogSourceDBWinReader)
{
    using namespace std::chrono_literals;

    auto dbgMsgSrc = std::filesystem::path(GetExecutePath()) / "DbgMsgSrc.exe";
    EXPECT_TRUE(std::filesystem::exists(dbgMsgSrc));
    auto executor = std::make_unique<ActiveExecutorClient>();
    Lines lines;
    DWORD pid = 0;
    {
        LogSources logsources(*executor, true);
        executor->Call([&] { logsources.SetAutoNewLine(true); });
        executor->Call([&] { logsources.AddDBWinReader(false); });
        Win32::Process process(dbgMsgSrc.wstring(), L"-n");
        process.Wait();
        pid = process.GetProcessId();
        executor->Call([&] { logsources.Abort(); });
        executor->Call([&] { lines = logsources.GetLines(); });
    }
    executor.reset();

    lines = OnlyFromProcess(std::move(lines), pid);
    EXPECT_EQ(lines.size(), 1);
}

TEST(DebugViewPlusPlusTest, LogSourceDBWinReader150KLines)
{
    using namespace std::chrono_literals;

    constexpr int amountOfTestLines = 150000;
    auto executor = std::make_unique<ActiveExecutorClient>();
    Lines lines;
    {
        LogSources logsources(*executor, true);
        executor->Call([&] { logsources.SetAutoNewLine(true); });
        executor->Call([&] { logsources.AddDBWinReader(false); });

        for (int i = 0; i < amountOfTestLines; ++i)
        {
            std::string message = stringbuilder() << i << '\n';
            OutputDebugStringA(message.c_str());
        }
        executor->Call([&] { logsources.Abort(); });
        executor->Call([&] { lines = logsources.GetLines(); });
    }
    executor.reset();

    lines = OnlyFromProcess(std::move(lines), ::GetCurrentProcessId());
    for (int i = 0; i < amountOfTestLines; ++i)
    {
        std::string message = stringbuilder() << i;
        if (lines[i].message != message)
        {
            std::print("'{}' is not '{}'\n", lines[i].message, message);
        }
        EXPECT_EQ(lines[i].message, message);
    }
    EXPECT_EQ(lines.size(), amountOfTestLines);
}

TEST(DebugViewPlusPlusTest, LogSourceDBWinReader150KLongerLines)
{
    using namespace std::chrono_literals;

    std::string test_string = "ThisIsALongerTestMessageThisIsALongerTestMessageThisIsALongerTestMessageThisIsALongerTestMessage";
    constexpr int amountOfTestLines = 150000;
    auto executor = std::make_unique<ActiveExecutorClient>();
    Lines lines;
    {
        LogSources logsources(*executor, true);
        executor->Call([&] { logsources.SetAutoNewLine(true); });
        executor->Call([&] { logsources.AddDBWinReader(false); });

        for (int i = 0; i < amountOfTestLines; ++i)
        {
            std::string message = stringbuilder() << i << test_string << test_string << '\n';
            OutputDebugStringA(message.c_str());
        }
        executor->Call([&] { logsources.Abort(); });
        executor->Call([&] { lines = logsources.GetLines(); });
    }
    executor.reset();

    lines = OnlyFromProcess(std::move(lines), ::GetCurrentProcessId());
    for (int i = 0; i < amountOfTestLines; ++i)
    {
        std::string message = stringbuilder() << i << test_string << test_string;
        if (lines[i].message != message)
        {
            std::print("'{}' is not '{}'\n", lines[i].message, message);
        }
        EXPECT_EQ(lines[i].message, message);
    }
    EXPECT_EQ(lines.size(), amountOfTestLines);
}

std::string CreateTestFile()
{
    Timer timer;
    LogFile logFile;
    logFile.Add(Message(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "test message 1"));
    logFile.Add(Message(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "test message 2"));
    logFile.Add(Message(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "test message 3"));
    logFile.Add(Message(0, Win32::GetSystemTimeAsFileTime(), 0, "processname", "message 4 (zero time message)"));
    return SaveLogFile(logFile);
}

std::string AppendToTestFile()
{
    Timer timer;
    LogFile logFile;
    logFile.Add(Message(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "test message appended"));
    return AppendLogFile(logFile);
}

std::vector<size_t> GetNewLineOffsets(const std::string& filename)
{
    std::vector<size_t> result;
    std::ifstream file(filename);
    Line line;
    while (ReadLogFileMessage(file, line))
        result.push_back(static_cast<size_t>(file.tellg()));
    return result;
}

void RemoveLinesFromFile(const std::string& filename, int linesToRemove)
{
    auto offsets = GetNewLineOffsets(filename);
    auto offset = offsets[offsets.size() - 1 - linesToRemove];
    Win32::HFile(GetTestFileName()).resize(offset);
}

// Polls until expectedCount lines have arrived (or the timeout expires), then waits one
// more settle interval and reads again, so that unexpected extra lines (duplicates after a
// resync, for example) are returned too and get caught by the caller's size check.
Lines WaitForLines(ActiveExecutorClient& executor, LogSources& logsources, size_t expectedCount,
                   std::chrono::milliseconds timeout = std::chrono::milliseconds(2000),
                   std::chrono::milliseconds interval = std::chrono::milliseconds(50),
                   std::chrono::milliseconds settle = std::chrono::milliseconds(100))
{
    Lines accumulated;
    auto readMore = [&] {
        Lines batch;
        executor.Call([&] { batch = logsources.GetLines(); });
        accumulated.insert(accumulated.end(),
                           std::make_move_iterator(batch.begin()),
                           std::make_move_iterator(batch.end()));
    };

    const auto startTime = std::chrono::steady_clock::now();
    for (;;)
    {
        readMore();
        if (accumulated.size() >= expectedCount)
            break;

        if (std::chrono::steady_clock::now() - startTime >= timeout)
            return accumulated;

        std::this_thread::sleep_for(interval);
    }

    std::this_thread::sleep_for(settle);
    readMore();
    return accumulated;
}

// #401: a process that sends a message and terminates immediately is only recognizable by its commandline,
// it is logged with the termination message.
TEST(DebugViewPlusPlusTest, LogSourceDBWinReaderLogsCommandLineOnTermination)
{
    using namespace std::chrono_literals;

    auto dbgMsgSrc = std::filesystem::path(GetExecutePath()) / "DbgMsgSrc.exe";
    ASSERT_TRUE(std::filesystem::exists(dbgMsgSrc));
    auto executor = std::make_unique<ActiveExecutorClient>();
    std::string terminationMessage;
    {
        LogSources logsources(*executor, true);
        executor->Call([&] { logsources.SetAutoNewLine(true); });
        executor->Call([&] { logsources.AddDBWinReader(false); });
        Win32::Process(dbgMsgSrc.wstring(), L"-n").Wait();

        for (int attempt = 0; attempt < 40 && terminationMessage.empty(); ++attempt)
        {
            for (const auto& line : WaitForLines(*executor, logsources, 1, 100ms))
            {
                if (line.message.find("has terminated") != std::string::npos)
                {
                    terminationMessage = line.message;
                }
            }
        }
        executor->Call([&] { logsources.Abort(); });
    }
    executor.reset();

    SCOPED_TRACE(terminationMessage);
    EXPECT_NE(terminationMessage.find("command line: "), std::string::npos);
    EXPECT_NE(terminationMessage.find("DbgMsgSrc.exe"), std::string::npos);
    EXPECT_NE(terminationMessage.find(" -n"), std::string::npos);
}

// #349: in session 0 (services) the local and the Global namespace are the same, so a second DBWinReader fails because
// DBWIN_BUFFER already exists. The failure must not break the reader that exists already.
TEST(DebugViewPlusPlusTest, LogSourceSecondDBWinReaderForSameNamespaceFails)
{
    auto executor = std::make_unique<ActiveExecutorClient>();
    LogSources logsources(*executor, true);
    executor->Call([&] { logsources.SetAutoNewLine(true); });
    executor->Call([&] { logsources.AddDBWinReader(false); });

    bool failed = false;
    executor->Call([&] {
        try
        {
            logsources.AddDBWinReader(false);
        }
        catch (const std::runtime_error&)
        {
            failed = true;
        }
    });
    EXPECT_TRUE(failed);

    OutputDebugStringA("still received\n");
    auto lines = WaitForLines(*executor, logsources, 1);
    EXPECT_TRUE((std::any_of(lines.begin(), lines.end(), [](const Line& line) { return line.message == "still received"; })));
    executor->Call([&] { logsources.Abort(); });
}

// A source that was added is only known to LogSources when the listen thread has picked it up, wait for that. Nothing is sent
// to the process as long as the result is 0, so this does not send the Enter more than once.
int SendEnterToProcessReaders(ActiveExecutorClient& executor, LogSources& logsources)
{
    using namespace std::chrono_literals;
    int sent = 0;
    for (int attempt = 0; attempt < 50 && sent == 0; ++attempt)
    {
        executor.Call([&] { sent = logsources.SendEnterToProcessReaders(); });
        if (sent == 0)
        {
            std::this_thread::sleep_for(20ms);
        }
    }
    return sent;
}

// #293: a process that waits for an Enter (or a batch file with 'pause') must be able to get one
TEST(DebugViewPlusPlusTest, LogSourceProcessReaderSendsEnterToStdin)
{
    using namespace std::chrono_literals;

    auto executor = std::make_unique<ActiveExecutorClient>();
    LogSources logsources(*executor, true);
    executor->Call([&] { logsources.SetAutoNewLine(true); });

    // 'set /p' waits until a line is entered
    executor->Call([&] { logsources.AddProcessReader(L"cmd.exe", L"/Q /C \"set /p dummy=& echo got-the-enter\""); });

    EXPECT_EQ((SendEnterToProcessReaders(*executor, logsources)), 1);

    bool received = false;
    for (int attempt = 0; attempt < 40 && !received; ++attempt)
    {
        for (const auto& line : WaitForLines(*executor, logsources, 1, 100ms))
        {
            received |= line.message.find("got-the-enter") != std::string::npos;
        }
    }
    EXPECT_TRUE(received);
    executor->Call([&] { logsources.Abort(); });
}

// every Enter satisfies exactly one 'pause'
TEST(DebugViewPlusPlusTest, LogSourceProcessReaderSingleEnterSatisfiesSinglePause)
{
    using namespace std::chrono_literals;

    const auto batchFile = std::filesystem::temp_directory_path() / "debugviewpp_pause_test.cmd";
    {
        std::ofstream fs(batchFile);
        fs << "echo \"start\"\n"
           << "\n"
           << "pause\n"
           << "\n"
           << "echo \"after pause\n"
           << "\n"
           << "pause\n"
           << "\n"
           << "echo \"got 2x enter\"\n"
           << "\n"
           << "pause\n";
    }
    auto removeBatchFile = make_guard([&] {
        std::error_code ec;
        std::filesystem::remove(batchFile, ec);
    });

    auto executor = std::make_unique<ActiveExecutorClient>();
    LogSources logsources(*executor, true);
    executor->Call([&] { logsources.SetAutoNewLine(true); });
    executor->Call([&] { logsources.AddProcessReader(L"cmd.exe", L"/Q /C \"" + batchFile.wstring() + L"\""); });

    std::vector<std::string> messages;
    auto hasMessage = [&](const std::string& text) {
        return std::any_of(messages.begin(), messages.end(), [&](const std::string& m) { return m.find(text) != std::string::npos; });
    };
    // collects the output for the given time, or until the text appears
    auto collect = [&](const std::string& until, std::chrono::milliseconds maxTime) {
        const auto deadline = std::chrono::steady_clock::now() + maxTime;
        while (std::chrono::steady_clock::now() < deadline && (until.empty() || !hasMessage(until)))
        {
            for (const auto& line : WaitForLines(*executor, logsources, 1, 100ms))
            {
                messages.push_back(line.message);
            }
        }
    };
    auto sendEnter = [&] { return SendEnterToProcessReaders(*executor, logsources); };

    collect("start", 5s);
    EXPECT_TRUE(hasMessage("start"));
    EXPECT_FALSE(hasMessage("after pause")); // waiting at the first pause

    // the first Enter ends the first pause only
    EXPECT_EQ(sendEnter(), 1);
    collect("after pause", 5s);
    EXPECT_TRUE(hasMessage("after pause"));
    collect("", 1500ms); // give it the chance to continue past the second pause, when it should not
    EXPECT_FALSE(hasMessage("got 2x enter"));

    // the second Enter ends the second pause
    EXPECT_EQ(sendEnter(), 1);
    collect("got 2x enter", 5s);
    EXPECT_TRUE(hasMessage("got 2x enter"));

    // the third ends the last pause, so the process ends
    sendEnter();
    collect("", 500ms);
    executor->Call([&] { logsources.Abort(); });
}

TEST(DebugViewPlusPlusTest, LogSourceDBWinReaderDelay)
{
    using namespace std::chrono_literals;

    constexpr int messageCount = 6;
    constexpr auto delay = 50ms;

    auto executor = std::make_unique<ActiveExecutorClient>();
    LogSources logsources(*executor, true);
    executor->Call([&] { logsources.SetAutoNewLine(true); });
    DBWinReader* pReader = nullptr;
    executor->Call([&] { pReader = logsources.AddDBWinReader(false); });
    ASSERT_NE(pReader, nullptr);

    EXPECT_EQ(pReader->GetDelay().count(), 0);
    pReader->SetDelay(delay);
    EXPECT_EQ(pReader->GetDelay().count(), delay.count());

    // OutputDebugString() can only deliver a message once the previous one is released by the reader, the first message
    // is not delayed, so N messages take at least (N - 1) delays.
    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < messageCount; ++i)
    {
        OutputDebugStringA("delayed message\n");
    }
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
    EXPECT_GE(elapsed.count(), ((messageCount - 1) * delay - 10ms).count());

    // the messages themselves are all delivered (other programs may send messages too, only count those of this process)
    Lines lines;
    for (int attempt = 0; attempt < 40 && OnlyFromProcess(lines, ::GetCurrentProcessId()).size() < messageCount; ++attempt)
    {
        auto received = WaitForLines(*executor, logsources, 1, 100ms);
        lines.insert(lines.end(), received.begin(), received.end());
    }
    EXPECT_EQ((OnlyFromProcess(lines, ::GetCurrentProcessId()).size()), messageCount);

    // no delay: the sender is not slowed down anymore
    pReader->SetDelay(0ms);
    EXPECT_EQ(pReader->GetDelay().count(), 0);
    const auto startUndelayed = std::chrono::steady_clock::now();
    for (int i = 0; i < messageCount; ++i)
    {
        OutputDebugStringA("undelayed message\n");
    }
    const auto elapsedUndelayed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startUndelayed);
    EXPECT_LT(elapsedUndelayed.count(), ((messageCount - 1) * delay).count());
    WaitForLines(*executor, logsources, messageCount);

    executor->Call([&] { logsources.Abort(); });
}

// A Dbgview agent can be specified by hostname as well as by IP address (#363), this fake agent implements just enough
// of the protocol (answering the performance frequency request) for the reader to consider itself connected.
TEST(DebugViewPlusPlusTest, LogSourceDbgviewReaderConnectsByHostname)
{
    Win32::TcpListener acceptor(2020);
    std::thread agent([&] {
        Win32::TcpStream socket;
        if (!acceptor.Accept(socket))
        {
            return;
        }

        Read<DWORD>(socket); // the request for the performance frequency
        Write<DWORD>(socket, DWORD(10'000'000));

        // keep the connection open until the reader disconnects
        char buffer[64];
        while (socket.read(buffer, sizeof(buffer)) || socket.gcount() > 0)
        {
        }
    });

    // also releases the agent when the reader never connected, or when the test fails with an exception
    struct StopAgent
    {
        Win32::TcpListener& acceptor;
        std::thread& agent;
        ~StopAgent()
        {
            acceptor.Close();
            agent.join();
        }
    } stopAgent{acceptor, agent};

    auto executor = std::make_unique<ActiveExecutorClient>();
    Lines lines;
    {
        LogSources logsources(*executor, true);
        executor->Call([&] { logsources.SetAutoNewLine(true); });
        executor->Call([&] { logsources.AddDbgviewReader("localhost"); }); // a name, not an IP address
        lines = WaitForLines(*executor, logsources, 2);
        executor->Call([&] { logsources.Abort(); });
    }
    executor.reset();

    bool connected = false;
    for (const auto& line : lines)
    {
        connected |= line.message.find("Connected to Dbgview Agent at localhost") != std::string::npos;
    }
    EXPECT_TRUE(connected);
}

TEST(DebugViewPlusPlusTest, TcpStreamConnectsByNameAndExchangesData)
{
    Win32::TcpListener listener(2031);
    std::string received;
    std::thread server([&] {
        Win32::TcpStream stream;
        if (listener.Accept(stream))
        {
            std::getline(stream, received);
            stream << "pong\n";
        }
    });

    Win32::TcpStream client;
    EXPECT_TRUE((client.Connect("localhost", 2031))); // a name, not an IP address
    client << "ping\n";
    std::string reply;
    std::getline(client, reply);
    server.join();

    EXPECT_EQ(received, "ping");
    EXPECT_EQ(reply, "pong");

    // the server is gone: the end of the stream
    char c;
    client.read(&c, 1);
    EXPECT_TRUE(client.eof());
}

TEST(DebugViewPlusPlusTest, TcpStreamConnectFailureHasMessage)
{
    Win32::TcpStream client;
    EXPECT_FALSE((client.Connect("127.0.0.1", 2032))); // nobody listens
    EXPECT_FALSE(client);
    EXPECT_FALSE(client.ErrorMessage().empty());

    Win32::TcpStream unknown;
    EXPECT_FALSE((unknown.Connect("no-such-host.invalid", 2032)));
    EXPECT_FALSE(unknown.ErrorMessage().empty());
}

TEST(DebugViewPlusPlusTest, TcpStreamCloseWakesUpBlockedRead)
{
    using namespace std::chrono_literals;

    Win32::TcpListener listener(2033);
    Win32::TcpStream server;
    std::thread accepter([&] { listener.Accept(server); });

    Win32::TcpStream client;
    ASSERT_TRUE((client.Connect("127.0.0.1", 2033)));
    accepter.join();

    std::atomic<bool> readReturned{false};
    std::thread reader([&] {
        char c;
        client.read(&c, 1); // the server never sends anything
        readReturned = true;
    });

    std::this_thread::sleep_for(200ms);
    EXPECT_FALSE(readReturned);
    client.Close();
    reader.join();
    EXPECT_TRUE(readReturned);
    EXPECT_TRUE(client.eof());
}

TEST(DebugViewPlusPlusTest, TcpListenerCloseWakesUpAccept)
{
    Win32::TcpListener listener(2034);
    Win32::TcpStream stream;
    bool accepted = true;
    std::thread accepter([&] { accepted = listener.Accept(stream); });
    listener.Close();
    accepter.join();
    EXPECT_FALSE(accepted);
}

TEST(DebugViewPlusPlusTest, SendBroadcastDoesNotThrow)
{
    EXPECT_NO_THROW(Win32::SendBroadcast(2035, "broadcast test\n"));
}

TEST(DebugViewPlusPlusTest, LogSourceAnyFileReader)
{
    using namespace std::chrono_literals;
    auto executor = std::make_unique<ActiveExecutorClient>();
    LogSources logsources(*executor, true);
    auto filename = CreateTestFile();
    executor->Call([&] { logsources.SetAutoNewLine(true); });
    executor->Call([&] { logsources.AddAnyFileReader(WStr(filename), true); });

    auto lines = WaitForLines(*executor, logsources, 5);
    EXPECT_EQ(lines.size(), 5);
}

TEST(DebugViewPlusPlusTest, LogSourceAnyFileReaderResychronizeShrinkingFile)
{
    using namespace std::chrono_literals;
    auto executor = std::make_unique<ActiveExecutorClient>();
    LogSources logsources(*executor, true);
    auto filename = CreateTestFile();
    executor->Call([&] { logsources.SetAutoNewLine(true); });
    executor->Call([&] { logsources.AddAnyFileReader(WStr(filename), true); });

    auto lines = WaitForLines(*executor, logsources, 5);
    EXPECT_EQ(lines.size(), 5);

    RemoveLinesFromFile(GetTestFileName(), 2);
    auto shrinkLines = WaitForLines(*executor, logsources, 1);
    EXPECT_EQ(shrinkLines.size(), 1);
    if (!shrinkLines.empty())
    {
        EXPECT_NE(shrinkLines.at(0).message.find("file shrank"), std::string::npos);
    }

    AppendToTestFile();
    AppendToTestFile();
    AppendToTestFile();

    auto appendLines = WaitForLines(*executor, logsources, 3);
    EXPECT_EQ(appendLines.size(), 3);
}

// a file that is still empty (for example one that was just created by the program that is going to write to it) is
// tailed until content appears, like any other file.
TEST(DebugViewPlusPlusTest, LogSourceAnyFileReaderEmptyFile)
{
    using namespace std::chrono_literals;

    auto executor = std::make_unique<ActiveExecutorClient>();
    LogSources logsources(*executor, true);
    auto filename = GetTestFileName();
    {
        std::ofstream fs(filename, std::ofstream::trunc); // empty
    }
    executor->Call([&] { logsources.SetAutoNewLine(true); });
    AnyFileReader* pReader = nullptr;
    executor->Call([&] { pReader = logsources.AddAnyFileReader(WStr(filename), true); });
    ASSERT_NE(pReader, nullptr);

    // nothing to read yet
    std::this_thread::sleep_for(300ms);
    Lines lines;
    executor->Call([&] { lines = logsources.GetLines(); });
    for (const auto& line : lines)
    {
        EXPECT_EQ(line.message.find("Unable"), std::string::npos);
    }

    {
        std::ofstream fs(filename, std::ofstream::app);
        fs << "first line\n";
        fs << "second line\n";
    }

    // the messages of the reader itself (started tailing... ) are part of the lines too
    auto received = WaitForLines(*executor, logsources, 2);
    std::vector<std::string> messages;
    for (const auto& line : lines)
    {
        messages.push_back(line.message);
    }
    for (const auto& line : received)
    {
        messages.push_back(line.message);
    }
    EXPECT_EQ((std::count(messages.begin(), messages.end(), "first line")), 1);
    EXPECT_EQ((std::count(messages.begin(), messages.end(), "second line")), 1);
    executor->Call([&] { logsources.Abort(); });
}

// when the first line turns out to be the header of a DebugView++ logfile, the file is read as such
TEST(DebugViewPlusPlusTest, LogSourceAnyFileReaderEmptyFileBecomesDebugViewLogfile)
{
    using namespace std::chrono_literals;

    auto executor = std::make_unique<ActiveExecutorClient>();
    LogSources logsources(*executor, true);
    auto filename = GetTestFileName();
    {
        std::ofstream fs(filename, std::ofstream::trunc); // empty
    }
    executor->Call([&] { logsources.SetAutoNewLine(true); });
    executor->Call([&] { logsources.AddAnyFileReader(WStr(filename), true); });

    CreateTestFile(); // writes a DebugView++ logfile, starting with the identification header, into the file

    Lines lines;
    auto hasLastMessage = [&] {
        return std::any_of(lines.begin(), lines.end(), [](const Line& line) { return line.message == "message 4 (zero time message)"; });
    };
    for (int attempt = 0; attempt < 40 && !hasLastMessage(); ++attempt)
    {
        auto received = WaitForLines(*executor, logsources, 1, 100ms);
        lines.insert(lines.end(), received.begin(), received.end());
    }

    std::vector<std::string> messages;
    for (const auto& line : lines)
    {
        messages.push_back(line.message);
    }
    EXPECT_EQ((std::count(messages.begin(), messages.end(), "test message 1")), 1);
    EXPECT_EQ((std::count(messages.begin(), messages.end(), "test message 3")), 1);
    EXPECT_EQ((std::count(messages.begin(), messages.end(), "message 4 (zero time message)")), 1);
    // the header is not shown as a message
    for (const auto& message : messages)
    {
        EXPECT_EQ(message.find("File Identification Header"), std::string::npos);
    }
    executor->Call([&] { logsources.Abort(); });
}

std::string GetTestFileAsString()
{
    std::ifstream file(GetTestFileName());
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

TEST(DebugViewPlusPlusTest, DISABLED_LogSourceAnyFileReaderRewriteByteByByte)
{
    using namespace std::chrono_literals;
    auto executor = std::make_unique<ActiveExecutorClient>();
    LogSources logsources(*executor, true);
    auto filename = CreateTestFile();
    executor->Call([&] { logsources.SetAutoNewLine(true); });
    executor->Call([&] { logsources.AddAnyFileReader(WStr(filename), true); });
    std::this_thread::sleep_for(200ms);

    {
        Lines lines;
        executor->Call([&] { lines = logsources.GetLines(); });
        EXPECT_EQ(lines.size(), 5);
    }
    auto content = GetTestFileAsString();

    {
        std::ofstream fs;
        fs.open(GetTestFileName(), std::ofstream::trunc);
        fs.close();
    }
    std::this_thread::sleep_for(100ms);
    for (char c : content)
    {
        std::ofstream fs;
        fs.open(GetTestFileName(), std::ofstream::app);
        fs << c;
        // just flushing the fstream doesn't force a trigger of Find....ChangeNotification, but closing the fstream does the trick
        fs.flush();
        fs.close();
        Sleep(20);
    }

    std::this_thread::sleep_for(200ms);
    Lines lines;
    executor->Call([&] { lines = logsources.GetLines(); });
    for (auto& line : lines)
    {
        EXPECT_EQ(line.message.find("xception"), std::string::npos);
        // std::cout << line.message << std::endl;
    }
    EXPECT_EQ(lines.size(), 6);
    EXPECT_EQ(lines.at(0).message.back(), '0'); // line ends in '...offset 0'
    EXPECT_EQ(lines.at(1).message, ("File Identification Header, DebugView++ Format Version 1"));
    EXPECT_EQ(lines.at(2).message, "test message 1");
    EXPECT_EQ(lines.at(3).message, "test message 2");
    EXPECT_EQ(lines.at(4).message, "test message 3");
    EXPECT_EQ(lines.at(5).message, "message 4 (zero time message)");
}

std::string CreateAsciiTestFile()
{
    auto filename = GetTestFileName();
    std::ofstream fs;
    fs.open(filename, std::ofstream::trunc);
    fs << "This is the first line\n";
    fs << "\n"; // empty line
    fs << "This is the last line\n";
    return filename;
}

TEST(DebugViewPlusPlusTest, LogSourceAnyFileReaderEmptyLines)
{
    using namespace std::chrono_literals;
    auto executor = std::make_unique<ActiveExecutorClient>();
    LogSources logsources(*executor, true);
    auto filename = CreateAsciiTestFile();
    executor->Call([&] { logsources.SetAutoNewLine(true); });
    executor->Call([&] { logsources.AddAnyFileReader(WStr(filename), true); });

    {
        auto lines = WaitForLines(*executor, logsources, 4);
        EXPECT_EQ(lines.size(), 4);
        EXPECT_NE(lines.at(0).message.find("tailing"), std::string::npos);
        EXPECT_NE(lines.at(1).message.find("first"), std::string::npos);
        EXPECT_EQ(lines.at(2).message, "") << "The expected newline is gone!";
        EXPECT_NE(lines.at(3).message.find("last"), std::string::npos);
        for (auto& line : lines)
        {
            EXPECT_EQ(line.message.find("xception"), std::string::npos);
            std::cout << line.message << std::endl;
        }
    }
}

TEST(DebugViewPlusPlusTest, LogSourceLoopbackOrdering)
{
    using namespace std::chrono_literals;
    auto executor = std::make_unique<ActiveExecutorClient>();
    LogSources logsources(*executor, true);
    auto filename = CreateAsciiTestFile();
    executor->Call([&] { logsources.SetAutoNewLine(true); });
    executor->Call([&] { logsources.AddMessage("Loopback message 1"); });
    executor->Call([&] { logsources.AddMessage("Loopback message 2"); });
    executor->Call([&] { logsources.AddMessage("Loopback message 3"); });
    executor->Call([&] { logsources.AddAnyFileReader(WStr(filename), true); });

    auto lines = WaitForLines(*executor, logsources, 7);

    EXPECT_EQ(lines.at(0).message, "Loopback message 1");
    EXPECT_EQ(lines.at(1).message, "Loopback message 2");
    EXPECT_EQ(lines.at(2).message, "Loopback message 3");
    EXPECT_NE(lines.at(3).message.find("tailing"), std::string::npos);
    EXPECT_NE(lines.at(4).message.find("first"), std::string::npos);
    EXPECT_EQ(lines.at(5).message, "") << "The expected newline is gone!";
    EXPECT_NE(lines.at(6).message.find("last"), std::string::npos);
    for (auto& line : lines)
    {
        std::cout << line.message << std::endl;
    }
}

#pragma warning(disable : 4996)

#ifndef _HAS_CXX17

std::string CreateUTF16LETestFile(int linecount)
{
    auto filename = GetTestFileName();
    std::wfstream fs;
    fs.imbue(std::locale(fs.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::little_endian>));
    fs.open(filename, std::ios::binary | std::ios::trunc | std::ios::in | std::ios::out);
    for (int i = 0; i < linecount; ++i)
    {
        fs << "This is line number [" << i + 1 << "]\n";
    }
    fs.close();
    return filename;
}

TEST(DebugViewPlusPlusTest, LoadUTF16LE)
{
    using namespace std::chrono_literals;
    auto executor = std::make_unique<ActiveExecutorClient>();
    LogSources logsources(*executor, true);
    int lineCount = 65000;
    auto filename = CreateUTF16LETestFile(lineCount);
    executor->Call([&] { logsources.SetAutoNewLine(true); });
    executor->Call([&] { logsources.AddAnyFileReader(WStr(filename), true); });

    // assumption: should be done in 4 seconds, even in debug mode
    size_t totalLines = 0;
    for (int i = 0; i < 20; ++i)
    {
        std::this_thread::sleep_for(200ms);
        Lines lines;
        executor->Call([&] { lines = logsources.GetLines(); });
        totalLines += lines.size();
    }

    // (lineCount + 1) because the first line looks like: "Started tailing ....txt identified as '...'
    EXPECT_EQ(totalLines, (lineCount + 1));
}

#endif

// add test simulating MFC application behaviour (pressing pause/unpause lots of times during significant incomming messages)

} // namespace debugviewpp
} // namespace fusion
