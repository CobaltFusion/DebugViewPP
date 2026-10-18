// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_TEST_MODULE DebugView++ Lib Unit Test

#include <boost/test/unit_test_gui.hpp>
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
#include "DebugViewppLib/SessionConfig.h"
#include "CobaltFusion/UndoRedoStack.h"

namespace fusion {
namespace debugviewpp {

BOOST_AUTO_TEST_SUITE(DebugViewPlusPlusTest)

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

BOOST_AUTO_TEST_CASE(TimeZone)
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

    BOOST_TEST_MESSAGE("UTC");
    {
        ScopedTimezoneBias(0);
        auto result = LoadLogFile(SaveLogFile(logFile));
        BOOST_TEST(AreEqual(result, logFile));
    }
    BOOST_TEST_MESSAGE("(UTC-8) Fremond");
    {
        ScopedTimezoneBias(-480);
        auto result = LoadLogFile(SaveLogFile(logFile));
        BOOST_TEST(AreEqual(result, logFile));
    }
    BOOST_TEST_MESSAGE("(UTC+8:30) Pyongyang Standard Time");
    {
        ScopedTimezoneBias(+510);
        auto result = LoadLogFile(SaveLogFile(logFile));
        BOOST_TEST(AreEqual(result, logFile));
    }
    BOOST_TEST_MESSAGE("test boundry case (UTC-12) Baker- / Howland Island (uninhabited islands belonging to the United States)");
    {
        ScopedTimezoneBias(-720);
        auto result = LoadLogFile(SaveLogFile(logFile));
        BOOST_TEST(AreEqual(result, logFile));
    }
    BOOST_TEST_MESSAGE("test boundry case (UTC+14) Kiritimati (Christmas Island, also uninhabited, part of the Kiribati Line Islands)");
    {
        ScopedTimezoneBias(+720);
        auto result = LoadLogFile(SaveLogFile(logFile));
        BOOST_TEST(AreEqual(result, logFile));
    }

    // test crossing timezones
    BOOST_TEST_MESSAGE("test sending a logfile into the past (UTC -> UTC-8)");
    {
        auto filename = SaveLogFile(logFile);
        ScopedTimezoneBias(-480);
        auto result = LoadLogFile(filename);
        BOOST_TEST(AreEqual(result, logFile));
    }

    // test crossing timezones
    BOOST_TEST_MESSAGE("test sending a logfile into the future (UTC-8 -> UTC)");
    {
        std::string filename;
        {
            ScopedTimezoneBias(-480);
            filename = SaveLogFile(logFile);
        }
        auto result = LoadLogFile(filename);
        BOOST_TEST(AreEqual(result, logFile));
    }
}

uint64_t FileTimeToUInt64(const FILETIME& ft)
{
    return (static_cast<uint64_t>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
}

BOOST_AUTO_TEST_CASE(USTimeConverterMidnightRollover)
{
    constexpr uint64_t oneSecond = 10'000'000;
    constexpr uint64_t oneDay = 24 * 60 * 60 * oneSecond;

    USTimeConverter converter;
    FILETIME before = {};
    FILETIME after = {};
    BOOST_REQUIRE(converter.ReadLocalTimeUSRegion("11:59:59 PM", before));
    BOOST_REQUIRE(converter.ReadLocalTimeUSRegion("12:00:01 AM", after));

    // two seconds later, on the next day
    BOOST_TEST(FileTimeToUInt64(after) - FileTimeToUInt64(before) == 2 * oneSecond);

    // the next days again, past the end of a month (not too long, to stay clear of daylight saving time changes)
    auto previous = FileTimeToUInt64(after);
    for (int day = 0; day < 40; ++day)
    {
        FILETIME t1 = {};
        FILETIME t2 = {};
        BOOST_REQUIRE(converter.ReadLocalTimeUSRegion("11:59:59 PM", t1));
        BOOST_REQUIRE(converter.ReadLocalTimeUSRegion("12:00:01 AM", t2));
        BOOST_TEST(FileTimeToUInt64(t1) > previous);
        BOOST_TEST(FileTimeToUInt64(t2) - FileTimeToUInt64(t1) == 2 * oneSecond);
        BOOST_TEST(FileTimeToUInt64(t1) - previous < oneDay);
        previous = FileTimeToUInt64(t2);
    }
}

BOOST_AUTO_TEST_CASE(InfoTipText)
{
    // a short message is shown as is, tabs are expanded
    BOOST_TEST((MakeInfoTipText(L"short message") == L"short message"));
    BOOST_TEST((MakeInfoTipText(L"a\tb") == L"a   b"));
    BOOST_TEST((MakeInfoTipText(L"") == L""));

    // a long message is wrapped, without losing characters
    const std::wstring text(250, L'x');
    const auto wrapped = MakeInfoTipText(text, 100, 1023);
    BOOST_TEST(std::count(wrapped.begin(), wrapped.end(), L'\n') == 2);
    BOOST_TEST(std::count(wrapped.begin(), wrapped.end(), L'x') == 250);
    BOOST_TEST(wrapped.find(L'\n') == 100);

    // a text that fits exactly is not cut off
    BOOST_TEST((MakeInfoTipText(std::wstring(30, L'x'), 100, 30) == std::wstring(30, L'x')));

    // a text that is too long is cut off and ends with an ellipsis, within the limit of a tooltip
    const std::wstring huge(100000, L'y');
    const auto cut = MakeInfoTipText(huge);
    BOOST_TEST(cut.size() == 1023);
    BOOST_TEST((cut.substr(cut.size() - 3) == L"..."));
    BOOST_TEST(cut.find(L'y') == 0);
}

// #284: a copy of a view can use the lines of the other view when the filters are the same
BOOST_AUTO_TEST_CASE(StringUtilCaseInsensitive)
{
    // equal
    BOOST_TEST(IEquals("/Min", "/min"));
    BOOST_TEST(IEquals(std::string("JSON"), "json"));
    BOOST_TEST(IEquals(std::wstring(L"/MIN"), L"/min"));
    BOOST_TEST(IEquals("", ""));
    BOOST_TEST(!IEquals("abc", "abd"));
    BOOST_TEST(!IEquals("abc", "abcd")); // not a prefix
    BOOST_TEST(!IEquals("abcd", "abc"));

    // a narrow and a wide string can be compared, the text is ASCII
    BOOST_TEST(IEquals(std::wstring(L".TXT"), ".txt"));
    BOOST_TEST(IEndsWith(std::wstring(L"c:\\logs\\Some.LOG"), ".log"));
    BOOST_TEST(!IEndsWith(std::wstring(L"c:\\logs\\Some.LOG"), ".txt"));

    // only ASCII letters are folded, the other characters (and non-ASCII text in UTF-8) are compared as they are
    BOOST_TEST(!IEquals("\xc3\x84", "\xc3\xa4")); // A with diaeresis and a with diaeresis in UTF-8
    BOOST_TEST(IEquals("\xc3\x84x", "\xc3\x84X"));
    BOOST_TEST(!IEquals("a[", "A{")); // '[' and '{' are not the same, although they are 32 apart
    BOOST_TEST(!IEquals("@", "`"));

    // starts with and ends with
    BOOST_TEST(IStartsWith(std::wstring(L"/Delay:100"), L"/delay:"));
    BOOST_TEST(!IStartsWith(std::wstring(L"/del"), L"/delay:")); // shorter than the prefix
    BOOST_TEST(IStartsWith("abc", ""));
    BOOST_TEST(IEndsWith("abc", ""));
    BOOST_TEST(IEndsWith("Test.DbConf", ".dbconf"));
    BOOST_TEST(!IEndsWith("conf", ".dbconf"));
    BOOST_TEST(EndsWith("File Identification Header", "Header"));
    BOOST_TEST(!EndsWith("File Identification Header", "header")); // case sensitive
    BOOST_TEST(!EndsWith("ab", "cab"));
    BOOST_TEST(EndsWith("ab", ""));

    // embedded 0 characters are just characters
    const std::string withNul("a\0B", 3);
    BOOST_TEST(IEquals(withNul, std::string("A\0b", 3)));
    BOOST_TEST(!IEquals(withNul, std::string("A\0c", 3)));
    BOOST_TEST(IContains(withNul, std::string("\0b", 2)));
}

BOOST_AUTO_TEST_CASE(StringUtilFindAndContains)
{
    const std::string text = "Hello Hello world, HELLO";
    BOOST_TEST(IFind(text, "hello") == 0u);
    BOOST_TEST(IFind(text, "hello", 1) == 6u); // from a position
    BOOST_TEST(IFind(text, "HELLO", 7) == 19u);
    BOOST_TEST(IFind(text, "hello", 20) == std::string::npos);
    BOOST_TEST(IFind(text, "WORLD") == 12u);
    BOOST_TEST(IFind(text, "worlds") == std::string::npos);
    BOOST_TEST(IFind(text, "o") == 4u);

    // the match can be at the end, and a pattern as long as the text works
    BOOST_TEST(IFind("abcdef", "EF") == 4u);
    BOOST_TEST(IFind("abc", "ABC") == 0u);
    BOOST_TEST(IFind("abc", "abcd") == std::string::npos);
    BOOST_TEST(IFind("", "a") == std::string::npos);

    // a partial match is not a match, the search goes on
    BOOST_TEST(IFind("aab", "ab") == 1u);
    BOOST_TEST(IFind("ababac", "abac") == 2u);

    // an empty pattern is found at the start position (like std::string::find), but not after the end
    BOOST_TEST(IFind("abc", "") == 0u);
    BOOST_TEST(IFind("abc", "", 3) == 3u);
    BOOST_TEST(IFind("abc", "", 4) == std::string::npos);
    BOOST_TEST(IContains("abc", ""));

    BOOST_TEST(IContains(std::wstring(L"/TAB:name"), L"/tab:"));
    BOOST_TEST(!IContains(std::wstring(L"/min"), L"/tab:"));
    BOOST_TEST(IContains(std::string("The ERROR is here"), std::string("error")));

    // wide text with a narrow pattern, and wide characters that are not ASCII are not folded
    BOOST_TEST(IFind(std::wstring(L"x.LOG"), ".log") == 1u);
    BOOST_TEST(IFind(std::wstring(L"\u00c4"), L"\u00e4") == std::string::npos);
}

BOOST_AUTO_TEST_CASE(StringUtilLowerAndTrim)
{
    BOOST_TEST(ToLower(std::string("Mixed CASE 123 \xc3\x84")) == "mixed case 123 \xc3\x84"); // the UTF-8 bytes are unchanged
    BOOST_TEST((ToLower(std::wstring(L"AbC")) == L"abc"));
    BOOST_TEST(ToLower(std::string()) == "");

    std::string message = "text with trailing space \r\n\t ";
    TrimRight(message, " \r\n\t");
    BOOST_TEST(message == "text with trailing space");
    TrimRight(message, " \r\n\t"); // nothing more to remove
    BOOST_TEST(message == "text with trailing space");

    std::string onlySpaces = " \r\n";
    TrimRight(onlySpaces, " \r\n\t");
    BOOST_TEST(onlySpaces == "");

    std::string empty;
    TrimRight(empty, " ");
    BOOST_TEST(empty == "");

    // the characters inside are kept, only the ends are trimmed
    BOOST_TEST(TrimCopy(std::string("  a b \t"), " \r\n\t") == "a b");
    BOOST_TEST(TrimCopy(std::string("a"), " ") == "a");
    BOOST_TEST(TrimCopy(std::string("   "), " ") == "");
    BOOST_TEST(TrimCopy(std::string(""), " ") == "");
    BOOST_TEST((TrimCopy(std::wstring(L"\t x \n"), L" \t\n") == L"x"));
}

// The files in testdata were written by the application (see testdata/README.md), before boost was removed. Files that users saved
// with older versions must stay readable, so these tests describe what is in them, whatever reads them.
std::string TestDataFile(const std::string& name)
{
    return std::string(TESTDATA_DIR) + "/" + name;
}

BOOST_AUTO_TEST_CASE(LegacyFilterFilesCanBeLoaded)
{
    for (const std::string file : {"filters_legacy.xml", "filters_legacy.json"})
    {
        BOOST_TEST_MESSAGE(file);
        const auto text = ReadTextFile(TestDataFile(file));
        const auto data = file.ends_with(".json") ? FilterDataFromLegacyJson(text) : FilterDataFromLegacyXml(text);
        BOOST_TEST(data.name == "Überblick: errors & warnings");

        const auto& messageFilters = data.filter.messageFilters;
        const auto& processFilters = data.filter.processFilters;
        BOOST_REQUIRE(messageFilters.size() == 10);
        BOOST_REQUIRE(processFilters.size() == 2);

        auto check = [](const Filter& f, const std::string& text, MatchType::type matchType, FilterType::type filterType, COLORREF bg, COLORREF fg, bool enable) {
            BOOST_TEST(f.text == text);
            BOOST_TEST(f.matchType == matchType);
            BOOST_TEST(f.filterType == filterType);
            BOOST_TEST(f.bgColor == bg);
            BOOST_TEST(f.fgColor == fg);
            BOOST_TEST(f.enable == enable);
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

BOOST_AUTO_TEST_CASE(FilterDataJsonRoundTrip)
{
    FilterData data;
    data.name = "Überblick \"quoted\" 'single' <&>";
    data.filter.messageFilters.push_back(MakeFilter("error", MatchType::Simple, FilterType::Include));
    data.filter.messageFilters.push_back(MakeFilter("a\\d+ \"q\" &<>", MatchType::Regex, FilterType::Exclude, RGB(1, 2, 3), RGB(4, 5, 6), false));
    data.filter.messageFilters.push_back(MakeFilter("auto", MatchType::RegexGroups, FilterType::Token, Colors::Auto, Colors::Auto));
    data.filter.processFilters.push_back(MakeFilter("app.exe", MatchType::Wildcard, FilterType::Highlight, RGB(250, 240, 200), RGB(0, 0, 0)));

    const auto text = FilterDataToJson(data);
    const auto result = FilterDataFromJson(text);
    BOOST_TEST(result.name == data.name);
    BOOST_REQUIRE(result.filter.messageFilters.size() == 3);
    BOOST_REQUIRE(result.filter.processFilters.size() == 1);
    BOOST_TEST(SameDefinition(result.filter, data.filter));
    BOOST_TEST(result.filter.messageFilters[2].bgColor == Colors::Auto);
    BOOST_TEST(result.filter.messageFilters[2].fgColor == Colors::Auto);
    BOOST_TEST(!result.filter.messageFilters[1].enable);
    BOOST_TEST(result.filter.messageFilters[1].text == data.filter.messageFilters[1].text);

    // every filter type and match type survives
    FilterData all;
    all.name = "all";
    for (int f = 0; f <= FilterTypeToInt(FilterType::Bookmark); ++f)
    {
        all.filter.messageFilters.push_back(MakeFilter("x", IntToMatchType(f % 5), IntToFilterType(f)));
    }
    BOOST_TEST(all.filter.messageFilters.size() == 11);
    BOOST_TEST(SameDefinition(FilterDataFromJson(FilterDataToJson(all)).filter, all.filter));
}

BOOST_AUTO_TEST_CASE(FilterDataJsonDefaultsAndErrors)
{
    // only the text and the types are required
    const auto data = FilterDataFromJson(R"({"name": "n", "messageFilters": [{"text": "t", "matchType": "Simple", "filterType": "Include"}]})");
    BOOST_REQUIRE(data.filter.messageFilters.size() == 1);
    BOOST_TEST(data.filter.messageFilters[0].enable);
    BOOST_TEST(data.filter.messageFilters[0].bgColor == RGB(255, 255, 255));
    BOOST_TEST(data.filter.messageFilters[0].fgColor == RGB(0, 0, 0));
    BOOST_TEST(data.filter.processFilters.empty());

    BOOST_CHECK_THROW(FilterDataFromJson(""), std::runtime_error);
    BOOST_CHECK_THROW(FilterDataFromJson("[1,2]"), std::runtime_error);
    BOOST_CHECK_THROW(FilterDataFromJson("{not json"), std::runtime_error);
    BOOST_CHECK_THROW(FilterDataFromJson(R"({"version": 99, "name": "n"})"), std::runtime_error);
    BOOST_CHECK_THROW(FilterDataFromJson(R"({"messageFilters": []})"), std::runtime_error); // no name
    BOOST_CHECK_THROW(FilterDataFromJson(R"({"name": "n", "messageFilters": [{"matchType": "Simple", "filterType": "Include"}]})"), std::runtime_error); // no text
    BOOST_CHECK_THROW(FilterDataFromJson(R"({"name": "n", "messageFilters": {"text": "t"}})"), std::runtime_error); // not an array
    BOOST_CHECK_THROW(FilterDataFromJson(R"({"name": "n", "messageFilters": [{"text": "t", "matchType": "Simple", "filterType": "Include", "backColor": {"r": 300, "g": 0, "b": 0}}]})"), std::runtime_error);
}

// the legacy readers and the files of the old version: what is in the golden files
BOOST_AUTO_TEST_CASE(LegacyFilterReadersKeepAutoColorAndSpecialCharacters)
{
    for (const std::string file : {"filters_legacy.xml", "filters_legacy.json"})
    {
        BOOST_TEST_MESSAGE(file);
        const auto text = ReadTextFile(TestDataFile(file));
        BOOST_REQUIRE(!text.empty());

        const auto data = file.ends_with(".json") ? FilterDataFromLegacyJson(text) : FilterDataFromLegacyXml(text);
        BOOST_TEST(data.name == "Überblick: errors & warnings");
        BOOST_REQUIRE(data.filter.messageFilters.size() == 10);
        BOOST_REQUIRE(data.filter.processFilters.size() == 2);
        BOOST_TEST(data.filter.messageFilters[3].bgColor == Colors::Auto);
        BOOST_TEST(data.filter.messageFilters[2].text == "a & b <c> \"quoted\" 'single'");
        BOOST_TEST(!data.filter.messageFilters[2].enable);
    }
}

BOOST_AUTO_TEST_CASE(LegacyReadersHandleMalformedInput)
{
    BOOST_CHECK_THROW(FilterDataFromLegacyXml(""), std::runtime_error);
    BOOST_CHECK_THROW(FilterDataFromLegacyXml("<Filter><Name>x</Filter>"), std::runtime_error);
    BOOST_CHECK_THROW(FilterDataFromLegacyXml("<Other/>"), std::runtime_error);
    BOOST_CHECK_THROW(FilterDataFromLegacyXml("<Filter><Name>x</Name><MessageFilters/></Filter>"), std::runtime_error); // no ProcessFilters
    BOOST_CHECK_THROW(FilterDataFromLegacyJson(""), std::runtime_error);
    BOOST_CHECK_THROW(FilterDataFromLegacyJson("{\"Filter\": {"), std::runtime_error);

    // no filters at all is fine: an empty element, and the empty string property_tree wrote for it in JSON
    const auto xml = FilterDataFromLegacyXml("<?xml version=\"1.0\" encoding=\"utf-8\"?><Filter><Name>n</Name><MessageFilters/><ProcessFilters></ProcessFilters></Filter>");
    BOOST_TEST(xml.name == "n");
    BOOST_TEST(xml.filter.messageFilters.empty());
    const auto json = FilterDataFromLegacyJson(R"({"Filter": {"Name": "n", "MessageFilters": "", "ProcessFilters": ""}})");
    BOOST_TEST(json.name == "n");
    BOOST_TEST(json.filter.processFilters.empty());

    // text is kept as it is, also a single space; numbers and booleans may have white space around them
    const auto spaces = FilterDataFromLegacyXml(
        "<Filter><Name>n</Name><MessageFilters>"
        "<Filter><Enable> true </Enable><Text> </Text><MatchType>Simple</MatchType><FilterType>Include</FilterType>"
        "<BackColor><Red> 1</Red><Green>2 </Green><Blue>3</Blue></BackColor><TextColor Auto=\"true\"/></Filter>"
        "</MessageFilters><ProcessFilters/></Filter>");
    BOOST_REQUIRE(spaces.filter.messageFilters.size() == 1);
    BOOST_TEST(spaces.filter.messageFilters[0].text == " ");
    BOOST_TEST(spaces.filter.messageFilters[0].bgColor == RGB(1, 2, 3));
    BOOST_TEST(spaces.filter.messageFilters[0].fgColor == Colors::Auto);
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

BOOST_AUTO_TEST_CASE(LegacyFilterFileIsConvertedWhenLoaded)
{
    for (const std::string file : {"filters_legacy.xml", "filters_legacy.json"})
    {
        BOOST_TEST_MESSAGE(file);
        TempDirectory dir;
        const auto source = dir.path / file;
        std::filesystem::copy_file(TestDataFile(file), source);
        const auto original = ReadWholeFile(source);
        const auto target = dir.path / "filters_legacy.json";

        std::string message;
        const auto data = LoadFilterFile(source, message);
        BOOST_TEST(data.filter.messageFilters.size() == 10);
        BOOST_TEST(!message.empty());

        // the old file is still there, byte for byte, under another name
        const auto archived = std::filesystem::path(source.string() + ".archived");
        BOOST_REQUIRE(std::filesystem::exists(archived));
        BOOST_TEST(ReadWholeFile(archived) == original);

        // and the filters are JSON now, that loads without a conversion
        BOOST_REQUIRE(std::filesystem::exists(target));
        BOOST_TEST((DetectConfigFormat(ReadWholeFile(target)) == ConfigFormat::Json));
        std::string secondMessage;
        const auto again = LoadFilterFile(target, secondMessage);
        BOOST_TEST(secondMessage.empty());
        BOOST_TEST(again.name == data.name);
        BOOST_TEST(SameDefinition(again.filter, data.filter));
        BOOST_TEST(again.filter.messageFilters[3].bgColor == Colors::Auto);
    }
}

BOOST_AUTO_TEST_CASE(LegacyFilterFileIsNotConvertedOverAnExistingFile)
{
    TempDirectory dir;
    const auto source = dir.path / "filters.xml";
    const auto target = dir.path / "filters.json";
    std::filesystem::copy_file(TestDataFile("filters_legacy.xml"), source);
    { std::ofstream(target) << "precious"; }

    std::string message;
    const auto data = LoadFilterFile(source, message);
    BOOST_TEST(data.filter.messageFilters.size() == 10); // still loaded
    BOOST_TEST(!message.empty());
    BOOST_TEST(std::filesystem::exists(source)); // and nothing was renamed or overwritten
    BOOST_TEST(!std::filesystem::exists(source.string() + ".archived"));
    BOOST_TEST(ReadWholeFile(target) == "precious");
}

BOOST_AUTO_TEST_CASE(ArchiveNamesAreNumberedAndTheLegacyContentIsNeverLost)
{
    TempDirectory dir;
    const auto file = dir.path / "x.dbconf";
    { std::ofstream(file) << "<first/>"; }
    { std::ofstream(dir.path / "x.dbconf.archived") << "<older/>"; }

    const auto message = ConvertLegacyFile(file, file, "{}");
    BOOST_TEST(!message.empty());
    BOOST_TEST(ReadWholeFile(file) == "{}");
    BOOST_TEST(ReadWholeFile(dir.path / "x.dbconf.archived") == "<older/>");
    BOOST_TEST(ReadWholeFile(dir.path / "x.dbconf.archived2") == "<first/>");
    BOOST_TEST(!std::filesystem::exists(dir.path / "x.dbconf.new"));

    // a target in a folder that does not exist: reported, and the legacy file stays where it is
    { std::ofstream(dir.path / "y.xml") << "<y/>"; }
    const auto failed = ConvertLegacyFile(dir.path / "y.xml", dir.path / "nowhere" / "y.json", "{}");
    BOOST_TEST(!failed.empty());
    BOOST_TEST(ReadWholeFile(dir.path / "y.xml") == "<y/>");
}

BOOST_AUTO_TEST_CASE(FilterFilesSaveAndLoad)
{
    TempDirectory dir;
    LogFilter filter;
    filter.messageFilters.push_back(MakeFilter("Überblick", MatchType::Simple, FilterType::Include, RGB(1, 2, 3), Colors::Auto));
    const auto file = dir.path / "mine.json";
    SaveFilterFile(file, "näme", filter);
    BOOST_TEST((DetectConfigFormat(ReadWholeFile(file)) == ConfigFormat::Json));

    std::string message;
    const auto data = LoadFilterFile(file, message);
    BOOST_TEST(message.empty());
    BOOST_TEST(data.name == "näme");
    BOOST_TEST(SameDefinition(data.filter, filter));

    BOOST_CHECK_THROW(LoadFilterFile(dir.path / "missing.json", message), std::runtime_error);
    { std::ofstream(dir.path / "other.json") << "{\"something\": 1}"; }
    BOOST_CHECK_THROW(LoadFilterFile(dir.path / "other.json", message), std::runtime_error);
    { std::ofstream(dir.path / "text.json") << "just text"; }
    BOOST_CHECK_THROW(LoadFilterFile(dir.path / "text.json", message), std::runtime_error);
}

// the session files: the golden files of the old version, read with the legacy reader
BOOST_AUTO_TEST_CASE(LegacySessionReaderReadsTheGoldenFiles)
{
    for (const std::string file : {"session_input.dbconf", "session_saved_by_app.dbconf"})
    {
        BOOST_TEST_MESSAGE(file);
        const auto config = SessionConfigFromLegacyXml(ReadTextFile(TestDataFile(file)));
        BOOST_TEST(!config.autoNewline);
        BOOST_TEST(config.linkViews);

        BOOST_REQUIRE(config.views.size() == 2);
        const auto& first = config.views[0];
        BOOST_TEST(first.name == "Errors");
        BOOST_TEST(first.clockTime);
        BOOST_TEST(!first.processColors);
        BOOST_TEST(first.filters.messageFilters.size() == 10u);
        BOOST_TEST(first.filters.processFilters.size() == 2u);
        BOOST_TEST(first.filters.messageFilters[3].bgColor == Colors::Auto);
        BOOST_TEST(first.filters.messageFilters[3].text == "id=(\\d+)");

        BOOST_REQUIRE(first.columns.has_value());
        BOOST_REQUIRE(first.columns->size() == 7u);
        BOOST_TEST((*first.columns)[1].index == 1);
        BOOST_TEST(*(*first.columns)[1].width == 111);
        BOOST_TEST(*(*first.columns)[3].enable == false);
        BOOST_TEST(*(*first.columns)[6].width == 1234);

        const auto& second = config.views[1];
        BOOST_TEST(second.name == "Überblick: errors & warnings");
        BOOST_TEST(!second.clockTime);
        BOOST_TEST(second.processColors);
        BOOST_TEST(second.filters.messageFilters.empty());
        // the file the application saved has the columns of the second view, the hand written one does not
        BOOST_TEST(second.columns.has_value() == (file == "session_saved_by_app.dbconf"));

        BOOST_REQUIRE(config.sources.size() == 2);
        BOOST_TEST((config.sources[0].type == SourceType::Udp));
        BOOST_TEST(config.sources[0].port == 2021);
        BOOST_TEST(!config.sources[0].enabled);
        BOOST_TEST(config.sources[0].description == "Test UDP source");
        BOOST_TEST((config.sources[1].type == SourceType::DebugViewAgent));
        BOOST_TEST(config.sources[1].address == "build-server.example.org");
    }
}

BOOST_AUTO_TEST_CASE(SessionConfigJsonRoundTrip)
{
    for (const std::string file : {"session_input.dbconf", "session_saved_by_app.dbconf"})
    {
        BOOST_TEST_MESSAGE(file);
        const auto config = SessionConfigFromLegacyXml(ReadTextFile(TestDataFile(file)));
        const auto json = SessionConfigToJson(config);
        BOOST_TEST((DetectConfigFormat(json) == ConfigFormat::Json));

        const auto again = SessionConfigFromJson(json);
        BOOST_TEST(SessionConfigToJson(again) == json); // nothing is lost or changed
        BOOST_REQUIRE(again.views.size() == config.views.size());
        for (size_t i = 0; i < config.views.size(); ++i)
        {
            BOOST_TEST(again.views[i].name == config.views[i].name);
            BOOST_TEST(SameDefinition(again.views[i].filters, config.views[i].filters));
            BOOST_TEST(again.views[i].columns.has_value() == config.views[i].columns.has_value());
        }
        BOOST_REQUIRE(again.sources.size() == config.sources.size());
        BOOST_TEST(again.sources[1].address == config.sources[1].address);
    }
}

BOOST_AUTO_TEST_CASE(LegacySessionViewsAndSourcesAreOrderedByIndex)
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
    BOOST_TEST(config.autoNewline);
    BOOST_TEST(!config.linkViews);
    BOOST_REQUIRE(config.views.size() == 2);
    BOOST_TEST(config.views[0].name == "first");
    BOOST_TEST(config.views[1].name == "second");
    BOOST_TEST(!config.views[1].columns.has_value());

    // a column without an index is skipped, the fields that are missing stay unset
    BOOST_REQUIRE(config.views[0].columns.has_value());
    BOOST_REQUIRE(config.views[0].columns->size() == 1u);
    BOOST_TEST((*config.views[0].columns)[0].index == 2);
    BOOST_TEST(*(*config.views[0].columns)[0].width == 50);
    BOOST_TEST(!(*config.views[0].columns)[0].enable.has_value());
    BOOST_TEST(!(*config.views[0].columns)[0].order.has_value());

    BOOST_REQUIRE(config.sources.size() == 2);
    BOOST_TEST(config.sources[0].description == "a");
    BOOST_TEST(config.sources[1].description == "b");

    // Sources is optional
    const auto noSources = SessionConfigFromLegacyXml(
        "<DebugViewPP><AutoNewline>true</AutoNewline><LinkViews>false</LinkViews><Views/></DebugViewPP>");
    BOOST_TEST(noSources.views.empty());
    BOOST_TEST(noSources.sources.empty());
}

BOOST_AUTO_TEST_CASE(SessionConfigErrors)
{
    BOOST_CHECK_THROW(SessionConfigFromLegacyXml(""), std::runtime_error);
    BOOST_CHECK_THROW(SessionConfigFromLegacyXml("<Other/>"), std::runtime_error);
    BOOST_CHECK_THROW(SessionConfigFromLegacyXml("<DebugViewPP><AutoNewline>maybe</AutoNewline><LinkViews>true</LinkViews><Views/></DebugViewPP>"), std::runtime_error);
    BOOST_CHECK_THROW(SessionConfigFromLegacyXml("<DebugViewPP><AutoNewline>true</AutoNewline><LinkViews>true</LinkViews></DebugViewPP>"), std::runtime_error); // no Views
    BOOST_CHECK_THROW(SessionConfigFromJson(""), std::runtime_error);
    BOOST_CHECK_THROW(SessionConfigFromJson("[]"), std::runtime_error);
    BOOST_CHECK_THROW(SessionConfigFromJson(R"({"version": 3})"), std::runtime_error);
    BOOST_CHECK_THROW(SessionConfigFromJson(R"({"views": {}})"), std::runtime_error);
    BOOST_CHECK_THROW(SessionConfigFromJson(R"({"views": [{"clockTime": true}]})"), std::runtime_error); // no name
    BOOST_CHECK_THROW(SessionConfigFromJson(R"({"views": [{"name": 5}]})"), std::runtime_error);
    BOOST_CHECK_THROW(SessionConfigFromJson(R"({"views": [{"name": "a", "columns": [{"width": 1}]}]})"), std::runtime_error); // column without index
    BOOST_CHECK_THROW(SessionConfigFromJson(R"({"sources": [{"description": "a", "type": "no such type"}]})"), std::exception);

    // the minimum is fine
    const auto minimal = SessionConfigFromJson("{}");
    BOOST_TEST(minimal.views.empty());
    const auto view = SessionConfigFromJson(R"({"views": [{"name": "a"}]})");
    BOOST_REQUIRE(view.views.size() == 1);
    BOOST_TEST(view.views[0].clockTime);
    BOOST_TEST(!view.views[0].columns.has_value());
}

BOOST_AUTO_TEST_CASE(LegacySessionFileIsConvertedWhenLoaded)
{
    TempDirectory dir;
    const auto file = dir.path / "mine.dbconf";
    std::filesystem::copy_file(TestDataFile("session_saved_by_app.dbconf"), file);
    const auto original = ReadWholeFile(file);

    std::string message;
    const auto config = LoadSessionFile(file, message);
    BOOST_TEST(config.views.size() == 2u);
    BOOST_TEST(!message.empty());

    // the same name is JSON now, the XML is kept
    BOOST_TEST((DetectConfigFormat(ReadWholeFile(file)) == ConfigFormat::Json));
    BOOST_REQUIRE(std::filesystem::exists(dir.path / "mine.dbconf.archived"));
    BOOST_TEST(ReadWholeFile(dir.path / "mine.dbconf.archived") == original);

    std::string secondMessage;
    const auto again = LoadSessionFile(file, secondMessage);
    BOOST_TEST(secondMessage.empty());
    BOOST_TEST(SessionConfigToJson(again) == SessionConfigToJson(config));

    // saving and loading
    SaveSessionFile(dir.path / "saved.dbconf", config);
    BOOST_TEST(SessionConfigToJson(LoadSessionFile(dir.path / "saved.dbconf", secondMessage)) == SessionConfigToJson(config));
    BOOST_CHECK_THROW(LoadSessionFile(dir.path / "missing.dbconf", message), std::runtime_error);
    { std::ofstream(dir.path / "text.dbconf") << "not a configuration"; }
    BOOST_CHECK_THROW(LoadSessionFile(dir.path / "text.dbconf", message), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(FilterSameDefinition)
{
    LogFilter a;
    a.messageFilters.push_back(Filter("error", MatchType::Simple, FilterType::Include));
    a.messageFilters.push_back(Filter("noise.*", MatchType::Regex, FilterType::Exclude, RGB(1, 2, 3), RGB(4, 5, 6)));
    a.processFilters.push_back(Filter("my.exe", MatchType::Simple, FilterType::Highlight, RGB(255, 0, 0), RGB(0, 0, 0)));

    LogFilter same = a;
    BOOST_TEST(SameDefinition(a, same));
    BOOST_TEST(SameDefinition(LogFilter(), LogFilter()));

    // the state of a filter is not a difference, it is not part of the definition
    same.messageFilters[0].matched = true;
    BOOST_TEST(SameDefinition(a, same));

    // everything that decides what is shown or how is
    auto changed = [&](auto modify) {
        LogFilter b = a;
        modify(b);
        return !SameDefinition(a, b);
    };
    BOOST_TEST(changed([](LogFilter& f) { f.messageFilters[0].text = "errors"; }));
    BOOST_TEST(changed([](LogFilter& f) { f.messageFilters[0].filterType = FilterType::Exclude; }));
    BOOST_TEST(changed([](LogFilter& f) { f.messageFilters[1].matchType = MatchType::Simple; }));
    BOOST_TEST(changed([](LogFilter& f) { f.messageFilters[1].bgColor = RGB(9, 9, 9); }));
    BOOST_TEST(changed([](LogFilter& f) { f.messageFilters[1].fgColor = RGB(9, 9, 9); }));
    BOOST_TEST(changed([](LogFilter& f) { f.messageFilters[0].enable = false; }));
    BOOST_TEST(changed([](LogFilter& f) { f.messageFilters.pop_back(); }));
    BOOST_TEST(changed([](LogFilter& f) { f.processFilters.clear(); }));
    BOOST_TEST(changed([](LogFilter& f) { std::swap(f.messageFilters[0], f.messageFilters[1]); })); // the order can matter
}

// #278: undo and redo of changes of the filters
BOOST_AUTO_TEST_CASE(UndoRedoStackTest)
{
    UndoRedoStack<std::string> history;
    std::string state = "none";
    BOOST_TEST(!history.CanUndo());
    BOOST_TEST(!history.CanRedo());
    BOOST_TEST(!history.Undo(state)); // nothing to undo, nothing changes
    BOOST_TEST(state == "none");
    BOOST_TEST(!history.Redo(state));

    // three changes
    history.Push(state);
    state = "exclude A";
    history.Push(state);
    state = "exclude A, include B";
    history.Push(state);
    state = "exclude A, include B, highlight C";

    // undo them one by one
    BOOST_TEST(history.Undo(state));
    BOOST_TEST(state == "exclude A, include B");
    BOOST_TEST(history.Undo(state));
    BOOST_TEST(state == "exclude A");
    BOOST_TEST(history.CanRedo());

    // and redo one
    BOOST_TEST(history.Redo(state));
    BOOST_TEST(state == "exclude A, include B");

    // a new change forgets what could be redone
    history.Push(state);
    state = "exclude A, include B, token D";
    BOOST_TEST(!history.CanRedo());
    BOOST_TEST(history.Undo(state));
    BOOST_TEST(state == "exclude A, include B");
    BOOST_TEST(history.Undo(state));
    BOOST_TEST(state == "exclude A");
    BOOST_TEST(history.Undo(state));
    BOOST_TEST(state == "none");
    BOOST_TEST(!history.CanUndo());

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
    BOOST_TEST(undone == 3);
    BOOST_TEST(value == 7); // the state before the last three changes

    history.Clear();
    BOOST_TEST(!history.CanUndo());
}

// #368: the colors of the dark theme
BOOST_AUTO_TEST_CASE(DarkThemeColors)
{
    auto brightness = [](COLORREF c) { return (GetRValue(c) * 299 + GetGValue(c) * 587 + GetBValue(c) * 114) / 1000; };

    // the light theme has the system colors, colors are not changed
    Colors::SetDarkTheme(false);
    BOOST_TEST(!Colors::IsDarkTheme());
    const COLORREF lightBackGround = Colors::BackGround;
    const COLORREF lightText = Colors::Text;
    BOOST_TEST(lightBackGround == ::GetSysColor(COLOR_WINDOW));
    BOOST_TEST(Colors::Adapt(RGB(200, 230, 250)) == RGB(200, 230, 250));
    BOOST_TEST(Colors::TextOn(RGB(200, 230, 250)) == lightText);

    Colors::SetDarkTheme(true);
    BOOST_TEST(Colors::IsDarkTheme());
    BOOST_TEST(brightness(Colors::BackGround) < 80);
    BOOST_TEST(brightness(Colors::Text) > 160);
    BOOST_TEST(brightness(Colors::ItemHighlightText) > brightness(Colors::ItemHighlight)); // selected lines can be read

    // light colors (random colors for processes and matches are light) are made dark, the text on them is light
    const COLORREF pastel = RGB(250, 240, 200);
    const COLORREF adapted = Colors::Adapt(pastel);
    BOOST_TEST(brightness(adapted) < 120);
    BOOST_TEST(GetRValue(adapted) > GetBValue(adapted)); // the hue is kept
    BOOST_TEST(brightness(Colors::TextOn(adapted)) > 160);

    // the text color contrasts with the background, also for the colors that are not adapted (colors of filters)
    BOOST_TEST(Colors::TextOn(RGB(255, 255, 200)) == RGB(0, 0, 0));
    BOOST_TEST(brightness(Colors::TextOn(RGB(20, 20, 20))) > 200);
    BOOST_TEST(brightness(Colors::TextOn(Colors::Highlight)) != brightness(Colors::Highlight)); // not unreadable

    // back to the system colors
    Colors::SetDarkTheme(false);
    BOOST_TEST(Colors::BackGround == lightBackGround);
    BOOST_TEST(Colors::Text == lightText);
}

// sizes are scaled for the monitor (#321)
BOOST_AUTO_TEST_CASE(DpiScaling)
{
    BOOST_TEST(Win32::ScaleToDpi(16, 96) == 16); // 100%
    BOOST_TEST(Win32::ScaleToDpi(16, 120) == 20); // 125%
    BOOST_TEST(Win32::ScaleToDpi(16, 144) == 24); // 150%
    BOOST_TEST(Win32::ScaleToDpi(100, 192) == 200); // 200%
    BOOST_TEST(Win32::ScaleToDpi(0, 144) == 0);

    // moving from one monitor to another, and back, gives the same size
    BOOST_TEST(Win32::ScaleBetweenDpi(60, 96, 144) == 90);
    BOOST_TEST(Win32::ScaleBetweenDpi(90, 144, 96) == 60);
    BOOST_TEST(Win32::ScaleBetweenDpi(-12, 96, 144) == -18); // the height of a font is negative
    BOOST_TEST(Win32::ScaleBetweenDpi(60, 0, 144) == 60); // an unknown DPI does not scale

    // there is always a DPI, also without a window
    BOOST_TEST(Win32::GetDpiForWindow(nullptr) >= 48u);
    BOOST_TEST(Win32::GetDpiForWindow(::GetDesktopWindow()) >= 48u);
}

// SetPrivilege reports if the privilege could be changed, it is not an error when the token does not have the privilege (#412)
BOOST_AUTO_TEST_CASE(SetPrivilegeReportsMissingPrivilege)
{
    // every account has this privilege, and it is enabled by default
    BOOST_TEST(Win32::SetPrivilege(SE_CHANGE_NOTIFY_NAME, true));

    // only the system has this privilege, an administrator does not
    BOOST_TEST(!Win32::SetPrivilege(SE_TCB_NAME, true));
}

BOOST_AUTO_TEST_CASE(HandleTest)
{
    static const HANDLE nullHandle = nullptr;

    HANDLE rawHandle = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId());
    Win32::Handle handle(rawHandle);
    {
        BOOST_TEST(handle.get() == rawHandle);
        Win32::Handle handle1(std::move(handle));
        BOOST_TEST(handle.get() == nullHandle);
        BOOST_TEST(handle1.get() == rawHandle);
        {
            Win32::Handle handle2(std::move(handle1));
            handle2.release();
            BOOST_TEST(handle2.get() == nullHandle);
        }
        BOOST_TEST(handle1.get() == nullHandle);
    }

    {
        Win32::Handle handle1;
        {
            HANDLE zero = nullptr;
            Win32::Handle zeroHandle(zero);
            BOOST_TEST(zeroHandle.get() == nullHandle);
            handle1 = std::move(zeroHandle);
            BOOST_TEST(zeroHandle.get() == nullHandle);
            BOOST_TEST(handle1.get() == nullHandle);
        }
        // this scope ensures having a Handle leave scope that had its guts ripped out by std::move
        // will not cause nullpointers or exeptions
    }
}

BOOST_AUTO_TEST_CASE(LineBufferTest1)
{
    TestLineBuffer buffer(64);
    FILETIME ft;
    ft.dwLowDateTime = 42;
    ft.dwHighDateTime = 43;
    buffer.Add(42.0, ft, nullptr, "test", nullptr);

    auto lines = buffer.GetLines();
    auto& line = lines[0];
    BOOST_TEST(line.time == 42.0);
    BOOST_TEST(line.systemTime.dwLowDateTime == DWORD(42));
    BOOST_TEST(line.systemTime.dwHighDateTime == DWORD(43));
    BOOST_TEST(buffer.Empty());
}

BOOST_AUTO_TEST_CASE(LineBufferTest2) // test overflows boosttestui with log-output locking its GUI up temporarily
{
    TestLineBuffer buffer(600);
    Timer timer;
    std::cout << "LineBufferTest2 running..." << std::endl;
    for (int j = 0; j < 100; ++j)
    {
        // BOOST_TEST_MESSAGE("j: " << j << "\n");

        for (int i = 0; i < 17; ++i)
        {
            // BOOST_TEST_MESSAGE("i: " << i << "\n");
            FILETIME ft;
            ft.dwLowDateTime = 43;
            ft.dwHighDateTime = 44;
            buffer.Add(42.0, ft, nullptr, "test", nullptr);
        }

        auto lines = buffer.GetLines();
        BOOST_TEST(lines.size() == 17);
        for (auto& line : lines)
        {
            BOOST_TEST(line.time == 42.0);
            BOOST_TEST(line.systemTime.dwLowDateTime == DWORD(43));
            BOOST_TEST(line.systemTime.dwHighDateTime == DWORD(44));
        }
    }
}

BOOST_AUTO_TEST_CASE(IndexedStorageRandomAccess)
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
    BOOST_TEST(!failed);
}

BOOST_AUTO_TEST_CASE(IndexedStorageCompression)
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
    BOOST_TEST_MESSAGE("VectorStorage requires: " << usedByVector / 1024 << " kB");

    for (size_t i = 0; i < testSize; ++i)
        s.Add(GetTestString(i));

    size_t m2 = ProcessInfo::GetPrivateBytes();
    size_t usedBySnappy = m2 - m1;

    BOOST_TEST_MESSAGE("SnappyStorage requires: " << usedBySnappy / 1024 << " kB (" << (100 * usedBySnappy) / usedByVector << "%)");
    BOOST_TEST(size_t(0.50 * usedByVector) > usedBySnappy);
}

// execute as:
// "DebugView++Test.exe" --log_level=test_suite --run_test=*/LogSourcesReceiveMessages
BOOST_AUTO_TEST_CASE(IndexedStorageDropFront)
{
    using namespace indexedstorage;
    SnappyStorage storage;
    for (int i = 0; i < 1000; ++i)
    {
        storage.Add(std::to_string(i));
    }
    BOOST_TEST(storage[0] == "0"); // block 0 is the block that is read last, it must not be used after it was dropped

    // the block size is 400, so there are 2 complete blocks (0..799), the others are in the block that is being written
    BOOST_TEST(storage.DropFront(100) == 0u);   // not a complete block
    BOOST_TEST(storage.DropFront(500) == 400u); // one block
    BOOST_TEST(storage[400] == "400");
    BOOST_TEST(storage[799] == "799");
    BOOST_TEST(storage[999] == "999");

    BOOST_TEST(storage.DropFront(100000) == 400u); // only the complete blocks that are left
    BOOST_TEST(storage[800] == "800");
    BOOST_TEST(storage[999] == "999");

    // adding continues with the next index
    BOOST_TEST(storage.Add("next") == 1000u);
    BOOST_TEST(storage[1000] == "next");
}

BOOST_AUTO_TEST_CASE(LogFileHistorySize)
{
    FILETIME time = {};

    // unlimited by default
    LogFile unlimited;
    for (int i = 0; i < 5000; ++i)
    {
        unlimited.Add(Message(i, time, 1, "process", std::to_string(i)));
    }
    BOOST_TEST(unlimited.BeginIndex() == 0);
    BOOST_TEST(unlimited.EndIndex() == 5000);
    BOOST_TEST(unlimited.Count() == 5000);

    // with a history size the oldest messages are removed, the index of the others stays the same
    LogFile logFile;
    logFile.SetHistorySize(1000);
    for (int i = 0; i < 5000; ++i)
    {
        logFile.Add(Message(i, time, 1, "process", std::to_string(i)));
    }
    BOOST_TEST(logFile.EndIndex() == 5000);
    BOOST_TEST(logFile.Count() >= 1000);
    BOOST_TEST(logFile.Count() < 1000 + 400); // messages are removed in blocks of 400
    BOOST_TEST(logFile.BeginIndex() > 0);
    BOOST_TEST(logFile.BeginIndex() == logFile.EndIndex() - logFile.Count());
    BOOST_TEST(logFile[logFile.BeginIndex()].text == std::to_string(logFile.BeginIndex()));
    BOOST_TEST(logFile[4999].text == "4999");
    BOOST_TEST(logFile[4500].text == "4500");

    // a lower history size removes messages at once
    unlimited.SetHistorySize(500);
    BOOST_TEST(unlimited.EndIndex() == 5000);
    BOOST_TEST(unlimited.Count() >= 500);
    BOOST_TEST(unlimited.Count() < 500 + 400);
    BOOST_TEST(unlimited[4999].text == "4999");

    // Clear starts again
    logFile.Clear();
    BOOST_TEST(logFile.BeginIndex() == 0);
    BOOST_TEST(logFile.EndIndex() == 0);
    logFile.Add(Message(0, time, 1, "process", "again"));
    BOOST_TEST(logFile[0].text == "again");
}

BOOST_AUTO_TEST_CASE(LogSourcesReceiveMessages)
{
    ActiveExecutorClient executor;
    LogSources logsources(executor, false);
    LogSource* logsource;
    executor.Call([&] { logsource = logsources.AddTestSource(); });

    Timer timer;
    logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "message 1");
    logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "message 2");
    logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "message 3");
    BOOST_TEST_MESSAGE("3 lines added.");

    Lines lines;
    executor.Call([&] { lines = logsources.GetLines(); });
    BOOST_TEST_MESSAGE("received: " << lines.size() << " lines.");

    BOOST_TEST(lines.size() == 3);

    for (auto& line : lines)
    {
        BOOST_TEST_MESSAGE("line: " << line.message);
    }

    const int testsize = 1000;
    BOOST_TEST_MESSAGE("Write " << testsize << " lines...");
    for (int i = 0; i < testsize; ++i)
    {
        logsource->Add(timer.Get(), Win32::GetSystemTimeAsFileTime(), 0, "processname", "TESTSTRING 1234\n");
    }

    Lines morelines;
    executor.Call([&] { morelines = logsources.GetLines(); });

    BOOST_TEST_MESSAGE("received: " << morelines.size() << " lines.");
    BOOST_TEST(morelines.size() == testsize);
}

BOOST_AUTO_TEST_CASE(LogSourcesCharacterPreservation)
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
    BOOST_TEST(lines.size() == 2);
    BOOST_TEST(lines[0].message == "TrailingSpace "); // space preserved
    BOOST_TEST(lines[1].message == "TrailingTab\t");  // tab preserved
}

BOOST_AUTO_TEST_CASE(LogSourcesTabHandling)
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
    BOOST_TEST(lines.size() == 2);
    BOOST_TEST(lines[0].message == "\tTabPrefix");         // space preserved
    BOOST_TEST(lines[1].message == "\t\tTwoTabsPrefixed"); // space preserved
}

BOOST_AUTO_TEST_CASE(LogSourcesNewLineHandling)
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
    BOOST_TEST(lines.size() == 3);
    BOOST_TEST(lines[0].message == "NewLinePostfix");
    BOOST_TEST(lines[1].message == "NewLineCarriageReturnPostfix");
    BOOST_TEST(lines[2].message == "CarriageReturnNewLinePostfix");
}

std::wstring GetExecutePath()
{
    auto path = absolute(std::filesystem::path(Win32::GetCommandLineArguments()[0]));
    return path.remove_filename().c_str();
}

BOOST_AUTO_TEST_CASE(LogSourceLoopback)
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
    BOOST_TEST(count == 2);
}

// The DBWIN_BUFFER is shared by the whole machine, other programs (Teams, game launchers, ...) send their messages to it as well,
// and those end up between the messages of the test. Only look at the messages of the process that is expected to send them.
Lines OnlyFromProcess(Lines lines, DWORD pid)
{
    lines.erase(std::remove_if(lines.begin(), lines.end(), [pid](const Line& line) { return line.pid != pid; }), lines.end());
    return lines;
}

BOOST_AUTO_TEST_CASE(LogSourceDBWinReader)
{
    using namespace std::chrono_literals;

    auto dbgMsgSrc = std::filesystem::path(GetExecutePath()) / "DbgMsgSrc.exe";
    BOOST_TEST(std::filesystem::exists(dbgMsgSrc));
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
    BOOST_TEST(lines.size() == 1);
}

BOOST_AUTO_TEST_CASE(LogSourceDBWinReader150KLines)
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
        BOOST_TEST(lines[i].message == message);
    }
    BOOST_TEST(lines.size() == amountOfTestLines);
}

BOOST_AUTO_TEST_CASE(LogSourceDBWinReader150KLongerLines)
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
        BOOST_TEST(lines[i].message == message);
    }
    BOOST_TEST(lines.size() == amountOfTestLines);
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
BOOST_AUTO_TEST_CASE(LogSourceDBWinReaderLogsCommandLineOnTermination)
{
    using namespace std::chrono_literals;

    auto dbgMsgSrc = std::filesystem::path(GetExecutePath()) / "DbgMsgSrc.exe";
    BOOST_REQUIRE(std::filesystem::exists(dbgMsgSrc));
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

    BOOST_TEST_MESSAGE(terminationMessage);
    BOOST_TEST(terminationMessage.find("command line: ") != std::string::npos);
    BOOST_TEST(terminationMessage.find("DbgMsgSrc.exe") != std::string::npos);
    BOOST_TEST(terminationMessage.find(" -n") != std::string::npos);
}

// #349: in session 0 (services) the local and the Global namespace are the same, so a second DBWinReader fails because
// DBWIN_BUFFER already exists. The failure must not break the reader that exists already.
BOOST_AUTO_TEST_CASE(LogSourceSecondDBWinReaderForSameNamespaceFails)
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
    BOOST_TEST(failed);

    OutputDebugStringA("still received\n");
    auto lines = WaitForLines(*executor, logsources, 1);
    BOOST_TEST(std::any_of(lines.begin(), lines.end(), [](const Line& line) { return line.message == "still received"; }));
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
BOOST_AUTO_TEST_CASE(LogSourceProcessReaderSendsEnterToStdin)
{
    using namespace std::chrono_literals;

    auto executor = std::make_unique<ActiveExecutorClient>();
    LogSources logsources(*executor, true);
    executor->Call([&] { logsources.SetAutoNewLine(true); });

    // 'set /p' waits until a line is entered
    executor->Call([&] { logsources.AddProcessReader(L"cmd.exe", L"/Q /C \"set /p dummy=& echo got-the-enter\""); });

    BOOST_TEST(SendEnterToProcessReaders(*executor, logsources) == 1);

    bool received = false;
    for (int attempt = 0; attempt < 40 && !received; ++attempt)
    {
        for (const auto& line : WaitForLines(*executor, logsources, 1, 100ms))
        {
            received |= line.message.find("got-the-enter") != std::string::npos;
        }
    }
    BOOST_TEST(received);
    executor->Call([&] { logsources.Abort(); });
}

// every Enter satisfies exactly one 'pause'
BOOST_AUTO_TEST_CASE(LogSourceProcessReaderSingleEnterSatisfiesSinglePause)
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
    BOOST_TEST(hasMessage("start"));
    BOOST_TEST(!hasMessage("after pause")); // waiting at the first pause

    // the first Enter ends the first pause only
    BOOST_TEST(sendEnter() == 1);
    collect("after pause", 5s);
    BOOST_TEST(hasMessage("after pause"));
    collect("", 1500ms); // give it the chance to continue past the second pause, when it should not
    BOOST_TEST(!hasMessage("got 2x enter"));

    // the second Enter ends the second pause
    BOOST_TEST(sendEnter() == 1);
    collect("got 2x enter", 5s);
    BOOST_TEST(hasMessage("got 2x enter"));

    // the third ends the last pause, so the process ends
    sendEnter();
    collect("", 500ms);
    executor->Call([&] { logsources.Abort(); });
}

BOOST_AUTO_TEST_CASE(LogSourceDBWinReaderDelay)
{
    using namespace std::chrono_literals;

    constexpr int messageCount = 6;
    constexpr auto delay = 50ms;

    auto executor = std::make_unique<ActiveExecutorClient>();
    LogSources logsources(*executor, true);
    executor->Call([&] { logsources.SetAutoNewLine(true); });
    DBWinReader* pReader = nullptr;
    executor->Call([&] { pReader = logsources.AddDBWinReader(false); });
    BOOST_REQUIRE(pReader != nullptr);

    BOOST_TEST(pReader->GetDelay().count() == 0);
    pReader->SetDelay(delay);
    BOOST_TEST(pReader->GetDelay().count() == delay.count());

    // OutputDebugString() can only deliver a message once the previous one is released by the reader, the first message
    // is not delayed, so N messages take at least (N - 1) delays.
    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < messageCount; ++i)
    {
        OutputDebugStringA("delayed message\n");
    }
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
    BOOST_TEST(elapsed.count() >= ((messageCount - 1) * delay - 10ms).count());

    // the messages themselves are all delivered (other programs may send messages too, only count those of this process)
    Lines lines;
    for (int attempt = 0; attempt < 40 && OnlyFromProcess(lines, ::GetCurrentProcessId()).size() < messageCount; ++attempt)
    {
        auto received = WaitForLines(*executor, logsources, 1, 100ms);
        lines.insert(lines.end(), received.begin(), received.end());
    }
    BOOST_TEST(OnlyFromProcess(lines, ::GetCurrentProcessId()).size() == messageCount);

    // no delay: the sender is not slowed down anymore
    pReader->SetDelay(0ms);
    BOOST_TEST(pReader->GetDelay().count() == 0);
    const auto startUndelayed = std::chrono::steady_clock::now();
    for (int i = 0; i < messageCount; ++i)
    {
        OutputDebugStringA("undelayed message\n");
    }
    const auto elapsedUndelayed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startUndelayed);
    BOOST_TEST(elapsedUndelayed.count() < ((messageCount - 1) * delay).count());
    WaitForLines(*executor, logsources, messageCount);

    executor->Call([&] { logsources.Abort(); });
}

// A Dbgview agent can be specified by hostname as well as by IP address (#363), this fake agent implements just enough
// of the protocol (answering the performance frequency request) for the reader to consider itself connected.
BOOST_AUTO_TEST_CASE(LogSourceDbgviewReaderConnectsByHostname)
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
    BOOST_TEST(connected);
}

BOOST_AUTO_TEST_CASE(TcpStreamConnectsByNameAndExchangesData)
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
    BOOST_TEST(client.Connect("localhost", 2031)); // a name, not an IP address
    client << "ping\n";
    std::string reply;
    std::getline(client, reply);
    server.join();

    BOOST_TEST(received == "ping");
    BOOST_TEST(reply == "pong");

    // the server is gone: the end of the stream
    char c;
    client.read(&c, 1);
    BOOST_TEST(client.eof());
}

BOOST_AUTO_TEST_CASE(TcpStreamConnectFailureHasMessage)
{
    Win32::TcpStream client;
    BOOST_TEST(!client.Connect("127.0.0.1", 2032)); // nobody listens
    BOOST_TEST(!client);
    BOOST_TEST(!client.ErrorMessage().empty());

    Win32::TcpStream unknown;
    BOOST_TEST(!unknown.Connect("no-such-host.invalid", 2032));
    BOOST_TEST(!unknown.ErrorMessage().empty());
}

BOOST_AUTO_TEST_CASE(TcpStreamCloseWakesUpBlockedRead)
{
    using namespace std::chrono_literals;

    Win32::TcpListener listener(2033);
    Win32::TcpStream server;
    std::thread accepter([&] { listener.Accept(server); });

    Win32::TcpStream client;
    BOOST_REQUIRE(client.Connect("127.0.0.1", 2033));
    accepter.join();

    std::atomic<bool> readReturned{false};
    std::thread reader([&] {
        char c;
        client.read(&c, 1); // the server never sends anything
        readReturned = true;
    });

    std::this_thread::sleep_for(200ms);
    BOOST_TEST(!readReturned);
    client.Close();
    reader.join();
    BOOST_TEST(readReturned);
    BOOST_TEST(client.eof());
}

BOOST_AUTO_TEST_CASE(TcpListenerCloseWakesUpAccept)
{
    Win32::TcpListener listener(2034);
    Win32::TcpStream stream;
    bool accepted = true;
    std::thread accepter([&] { accepted = listener.Accept(stream); });
    listener.Close();
    accepter.join();
    BOOST_TEST(!accepted);
}

BOOST_AUTO_TEST_CASE(SendBroadcastDoesNotThrow)
{
    BOOST_CHECK_NO_THROW(Win32::SendBroadcast(2035, "broadcast test\n"));
}

BOOST_AUTO_TEST_CASE(LogSourceAnyFileReader)
{
    using namespace std::chrono_literals;
    auto executor = std::make_unique<ActiveExecutorClient>();
    LogSources logsources(*executor, true);
    auto filename = CreateTestFile();
    executor->Call([&] { logsources.SetAutoNewLine(true); });
    executor->Call([&] { logsources.AddAnyFileReader(WStr(filename), true); });

    auto lines = WaitForLines(*executor, logsources, 5);
    BOOST_TEST(lines.size() == 5);
}

BOOST_AUTO_TEST_CASE(LogSourceAnyFileReaderResychronizeShrinkingFile)
{
    using namespace std::chrono_literals;
    auto executor = std::make_unique<ActiveExecutorClient>();
    LogSources logsources(*executor, true);
    auto filename = CreateTestFile();
    executor->Call([&] { logsources.SetAutoNewLine(true); });
    executor->Call([&] { logsources.AddAnyFileReader(WStr(filename), true); });

    auto lines = WaitForLines(*executor, logsources, 5);
    BOOST_TEST(lines.size() == 5);

    RemoveLinesFromFile(GetTestFileName(), 2);
    auto shrinkLines = WaitForLines(*executor, logsources, 1);
    BOOST_TEST(shrinkLines.size() == 1);
    if (!shrinkLines.empty())
    {
        BOOST_TEST(shrinkLines.at(0).message.find("file shrank") != std::string::npos);
    }

    AppendToTestFile();
    AppendToTestFile();
    AppendToTestFile();

    auto appendLines = WaitForLines(*executor, logsources, 3);
    BOOST_TEST(appendLines.size() == 3);
}

// a file that is still empty (for example one that was just created by the program that is going to write to it) is
// tailed until content appears, like any other file.
BOOST_AUTO_TEST_CASE(LogSourceAnyFileReaderEmptyFile)
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
    BOOST_REQUIRE(pReader != nullptr);

    // nothing to read yet
    std::this_thread::sleep_for(300ms);
    Lines lines;
    executor->Call([&] { lines = logsources.GetLines(); });
    for (const auto& line : lines)
    {
        BOOST_TEST(line.message.find("Unable") == std::string::npos);
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
    BOOST_TEST(std::count(messages.begin(), messages.end(), "first line") == 1);
    BOOST_TEST(std::count(messages.begin(), messages.end(), "second line") == 1);
    executor->Call([&] { logsources.Abort(); });
}

// when the first line turns out to be the header of a DebugView++ logfile, the file is read as such
BOOST_AUTO_TEST_CASE(LogSourceAnyFileReaderEmptyFileBecomesDebugViewLogfile)
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
    BOOST_TEST(std::count(messages.begin(), messages.end(), "test message 1") == 1);
    BOOST_TEST(std::count(messages.begin(), messages.end(), "test message 3") == 1);
    BOOST_TEST(std::count(messages.begin(), messages.end(), "message 4 (zero time message)") == 1);
    // the header is not shown as a message
    for (const auto& message : messages)
    {
        BOOST_TEST(message.find("File Identification Header") == std::string::npos);
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

BOOST_AUTO_TEST_CASE(LogSourceAnyFileReaderRewriteByteByByte, *boost::unit_test::disabled())
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
        BOOST_TEST(lines.size() == 5);
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
        BOOST_TEST(line.message.find("xception") == std::string::npos);
        // std::cout << line.message << std::endl;
    }
    BOOST_TEST(lines.size() == 6);
    BOOST_TEST(lines.at(0).message.back() == '0'); // line ends in '...offset 0'
    BOOST_TEST(lines.at(1).message == "File Identification Header, DebugView++ Format Version 1");
    BOOST_TEST(lines.at(2).message == "test message 1");
    BOOST_TEST(lines.at(3).message == "test message 2");
    BOOST_TEST(lines.at(4).message == "test message 3");
    BOOST_TEST(lines.at(5).message == "message 4 (zero time message)");
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

BOOST_AUTO_TEST_CASE(LogSourceAnyFileReaderEmptyLines)
{
    using namespace std::chrono_literals;
    auto executor = std::make_unique<ActiveExecutorClient>();
    LogSources logsources(*executor, true);
    auto filename = CreateAsciiTestFile();
    executor->Call([&] { logsources.SetAutoNewLine(true); });
    executor->Call([&] { logsources.AddAnyFileReader(WStr(filename), true); });

    {
        auto lines = WaitForLines(*executor, logsources, 4);
        BOOST_TEST(lines.size() == 4);
        BOOST_TEST(lines.at(0).message.find("tailing") != std::string::npos);
        BOOST_TEST(lines.at(1).message.find("first") != std::string::npos);
        BOOST_TEST(lines.at(2).message == "", "The expected newline is gone!");
        BOOST_TEST(lines.at(3).message.find("last") != std::string::npos);
        for (auto& line : lines)
        {
            BOOST_TEST(line.message.find("xception") == std::string::npos);
            std::cout << line.message << std::endl;
        }
    }
}

BOOST_AUTO_TEST_CASE(LogSourceLoopbackOrdering)
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

    BOOST_TEST(lines.at(0).message == "Loopback message 1");
    BOOST_TEST(lines.at(1).message == "Loopback message 2");
    BOOST_TEST(lines.at(2).message == "Loopback message 3");
    BOOST_TEST(lines.at(3).message.find("tailing") != std::string::npos);
    BOOST_TEST(lines.at(4).message.find("first") != std::string::npos);
    BOOST_TEST(lines.at(5).message == "", "The expected newline is gone!");
    BOOST_TEST(lines.at(6).message.find("last") != std::string::npos);
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

BOOST_AUTO_TEST_CASE(LoadUTF16LE)
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
    BOOST_TEST(totalLines == (lineCount + 1));
}

#endif

// add test simulating MFC application behaviour (pressing pause/unpause lots of times during significant incomming messages)

BOOST_AUTO_TEST_SUITE_END()

} // namespace debugviewpp
} // namespace fusion
