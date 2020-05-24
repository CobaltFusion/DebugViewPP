// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <windows.h>
#include <WinSock.h>
#include <psapi.h>
#include <sys/timeb.h>
#include "Win32/Win32Lib.h"
#include "dbgstream.h"
#include "Timer.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "psapi.lib")

namespace fusion {

namespace Win32 {

class WinsockInitialization : noncopyable
{
public:
    explicit WinsockInitialization(int major = 2, int minor = 2)
    {
        WSADATA wsaData = {0};
        int rc = WSAStartup(MAKEWORD(major, minor), &wsaData);
        if (rc != 0)
            ThrowWin32Error(rc, "WSAStartup");
    }

    ~WinsockInitialization()
    {
        WSACleanup();
    }
};

void WSAThrowLastError(const std::string& what)
{
    Win32::ThrowWin32Error(WSAGetLastError(), what);
}

} // namespace Win32

namespace DbgMsgSrc {

void DbgMsgSrc(double freq)
{
    unsigned t0 = GetTickCount();
    unsigned count = 0;
    double msgPerTick = freq * 1e-3;
    for (;;)
    {
        Sleep(10);
        while (count < (GetTickCount() - t0) * msgPerTick)
        {
            if (count % 10 == 0)
                cdbg << "Een, twee, drie, vier, vijf, zes, zeven, acht, negen, tien, elf, twaalf, dertien, veertien, vijftien, zestien, zeventien, achttien, negentien, twintig, eenentwintig, tweeentwintig, drieentwintig, vierentwintig, vijfentwintig, zesentwintig, zevenentwintig, achtentwintig, negenentwintig, dertig.\n";
            else
                cdbg << "Message #" << count << "\n";
            ++count;
        }
    }
}

void DbgMsgTest()
{
    OutputDebugStringA("Message 1 without newline");
    OutputDebugStringA("Message 2 without newline");
    OutputDebugStringA("Message 3 with newline\n");
    OutputDebugStringA("Message with\nembedded newline\n");
    OutputDebugStringA("This should look like a table in a non-proportionally spaced font like 'Courier'");
    OutputDebugStringA("Columnn1\tColumnn2\tColumnn3\tColumnn4\tColumnn5");
    OutputDebugStringA("1\t\t2\t\t3\t\t4\t\t5");
    OutputDebugStringA("A\t\tB\t\tC\t\tD\t\tE");
    OutputDebugStringA("11\t\t12\t\t13\t\t14\t\t15");
    OutputDebugStringA("21\t\t22\t\t23A\t\t24\t\t25");

    OutputDebugStringA(" This line has 1 space prefixed");
    OutputDebugStringA("  This line has 2 space prefixed");
    OutputDebugStringA("   This line has 3 space prefixed");
    OutputDebugStringA("    This line has 4 space prefixed");

    OutputDebugStringA("HighLighting test: Double-click the word 'lines' in each of the following lines and make sure that word is highlighted.");

    OutputDebugStringA("\t123\t \t123\t \t123\t \t123\t This lines starts with tab");
    OutputDebugStringA(" \t123\t \t123\t \t123\t \t123\t This lines starts with 1 space + tab");
    OutputDebugStringA("  \t123\t  \t123\t  \t123\t  \t123\t This lines starts with 2 spaces + tab");
    OutputDebugStringA("   \t123\t   \t123\t   \t123\t   \t123\t This lines starts with 3 spaces + tab");
    OutputDebugStringA("    \t123\t    \t123\t    \t123\t    \t123\t This lines starts with 4 spaces + tab");
    OutputDebugStringA("2LongLine: Very Long Message that ends in a single newline. Very Long Message that ends in a single newline. Very Long Message that ends in a single newline. Very Long Message that ends in a single newline. Very Long Message that ends in a single newline. Very Long Message that ends in a single newline. Very Long Message that ends in a single newline. Very Long Message that ends in a single newline. Very Long Message that ends in a single newline. Very Long Message that ends in a single newline. Very Long Message that ends in a single newline. Very Long Message that ends in a single newline. Very Long Message that ends in a single newline. Very Long Message that ends in a single newline. Message ends HERE.\n");

    OutputDebugStringA("Message without newline (tid: 9000)");
    OutputDebugStringA("Message without newline  (tid: 9001)");
    OutputDebugStringA("Message without newline    (tid: 9002)");
    OutputDebugStringA("Message without newline      (tid: 9003)");
    OutputDebugStringA("Message without newline  (tid: 9001)");
    OutputDebugStringA("Message without newline    (tid: 9002)");
    OutputDebugStringA("Message without newline      (tid: 9003)");
    OutputDebugStringA("Message without newline      (tid: 9003)");
    OutputDebugStringA("Message without newline    (tid: 9002)");
    OutputDebugStringA("Message without newline  (tid: 9001)");
    OutputDebugStringA("Message without newline (tid: 9000)");
    OutputDebugStringA("Message without newline (address: 0x01020304)");
    OutputDebugStringA("Message without newline  (address: 0xDEADBEEF)");
    OutputDebugStringA("Message without newline   (address: 0xFEEDBEEF)");
    OutputDebugStringA("Message without newline    (address: 0xB105F00D)");
    OutputDebugStringA("Message without newline     (address: 0x0DEFACED)");
    OutputDebugStringA("Message without newline     (address: 0xBAADF00D)");
    OutputDebugStringA("Message without newline    (address: 0xCAFEBABE)");
    OutputDebugStringA("Message without newline   (address: 0xDEADC0DE)");
    OutputDebugStringA("Message without newline (address: 0xDEADC0DE)");
}

void SocketTest()
{
    OutputDebugStringA("1");
    OutputDebugStringA("22");
    OutputDebugStringA("333");
    OutputDebugStringA("4444");
    OutputDebugStringA("55555");
    OutputDebugStringA("666666");
    OutputDebugStringA("1010101010");
}

void SendMessageTestNotepad(std::string msg)
{
    HWND notepad = FindWindowA(NULL, "Untitled - Notepad");
    HWND edit = FindWindowExA(notepad, NULL, "EDIT", NULL);
    SendMessageA(edit, EM_REPLACESEL, TRUE, (LPARAM)msg.data());
}

void SendMessageDebugViewpp(std::string msg)
{
    HWND debugviewpp = FindWindowA(NULL, "[Capture Win32] - DebugView++");
    SendMessageA(debugviewpp, EM_REPLACESEL, GetCurrentProcessId(), (LPARAM)msg.data());
}

void DbgMsgClearTest()
{
    char buffer[200];

    int line = 1;
    for (int i = 0; i < 400; ++i)
    {
        sprintf_s(buffer, "Message %d", line);
        OutputDebugStringA(buffer);
        ++line;
    }

    OutputDebugStringA("    ----> DBGVIEWCLEAR\n");

    for (int i = 0; i < 10; ++i)
    {
        sprintf_s(buffer, "Message %d", line);
        OutputDebugStringA(buffer);
        ++line;
    }
}

int getMilliCount()
{
    timeb tb;
    ftime(&tb);
    return tb.millitm + (tb.time & 0xfffff) * 1000;
}

void testLongString()
{
    int length = 15000;
    std::ostringstream ss;
    ss << "1234567890ABCDEF-%s {0} \t\r\n";
    ss << length;
    std::string test = ss.str();
    for (size_t i = 0; i < length - test.size() - 1; ++i)
        ss << "X";

    ss << "\n";
    test = ss.str();

    OutputDebugStringA(test.c_str());
}

std::vector<std::string> GetFileContent(const std::string& filename)
{
    std::fstream fs;
    fs.open(filename, std::fstream::in);

    if (!fs)
    {
        std::cout << "\"" << filename << "\" not found!\n";
        return {};
    }

    std::cout << "Reading " << filename << "...\n";
    std::vector<std::string> lines;
    std::string line;
    while (getline(fs, line))
        lines.push_back(line);
    fs.close();
    return lines;
}

void TestODS(const std::string& filename)
{
    auto lines = GetFileContent(filename);
    std::cout << "writing... " << lines.size() << " lines\n";
    Timer timer;
    auto t1 = timer.now();
    for (const auto& s : lines)
    {
        OutputDebugStringA(s.c_str());
    }
    auto elepsed = timer.now() - t1;
    std::cout << "OutputDebugStringA " << lines.size() << " lines, took: " << static_cast<int>(Timer::ToMs(elepsed)) << " ms\n";
}

void TestSendMessage(const std::string& filename)
{
    auto lines = GetFileContent(filename);
    std::cout << "writing... " << lines.size() << " lines\n";
    Timer timer;
    auto t1 = timer.now();
    for (const auto& s : lines)
    {
        SendMessageDebugViewpp(s);
    }
    auto elepsed = timer.now() - t1;
    std::cout << "SendMessage " << lines.size() << " lines, took: " << static_cast<int>(Timer::ToMs(elepsed)) << " ms\n";
}

void EndlessTest()
{
    int length = 40; //260
    std::ostringstream ss;
    ss << "123456:jan-7890_ABC__DEF-te m_test";
    ss << length;
    std::string test = ss.str();
    for (size_t i = 0; i < length - test.size() - 1; ++i)
        ss << "X";

    ss << "\n";
    test = ss.str();

    //testLongString();

    for (;;)
    {
        long t1 = getMilliCount();

        for (int a = 0; a < 5; ++a)
        {
            OutputDebugStringA("    ## before me ##\n");

            // write exactly 2MB to the debug buffer;
            for (int i = 0; i < 9; ++i)
                OutputDebugStringA(test.c_str());

            long t2 = getMilliCount();
            OutputDebugStringA("    ## tracking me ##\n");
            std::cout << "took: " << t2 - t1 << " ms\n";
            Sleep(50);
        }
        //OutputDebugStringA("    ----> DBGVIEWCLEAR\n");
    }
}

void SeparateProcessTest()
{
    std::cerr << "SeparateProcessTest\n";
    for (;;)
    {
        auto result = ShellExecute(nullptr, L"open", L"DbgMsgSrc.exe", L"-n", nullptr, SW_HIDE);
        if (result <= HINSTANCE(32))
        {
            std::cerr << "error starting DbgMsgSrc.exe\n";
            break;
        }
        Sleep(100);
    }
}

void CoutCerrTest()
{
    for (int i = 1; i <= 5; ++i)
    {
        std::cout << "========= cycle " << i << "/5 ========\n";
        std::cout << "Message on cout 1\n";
        std::cout << "Message on cout 2\n";
        std::cerr << "Message on cerr 1\n";
        std::cerr << "Message on cerr 2\n";
        std::cout << "Message on cout 3\n";
        std::cerr << "Message on cerr 3\n";
        std::cout << "Message on cout 4\n";
        std::cerr << "Message on cerr 4\n";
        std::cout << "Message on cout 5\n";
        std::cerr << "Message on cerr 5\n";
        Sleep(2000);
    }
}

void UdpTest(const std::string& address, int port)
{
    Win32::WinsockInitialization wsaInit;

    auto s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET)
        Win32::WSAThrowLastError("socket");

    sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = inet_addr(address.c_str());

    std::string message = "DbgMsgSrc UDP message";
    auto rc = sendto(s, message.data(), static_cast<int>(message.size()), 0, reinterpret_cast<const sockaddr*>(&sa), sizeof(sa));
    if (rc == SOCKET_ERROR)
        Win32::WSAThrowLastError("sendto");

    closesocket(s);
}

void CoutCerrTest2()
{
    std::cout << "One message on cout with newline\n";
    //std::cerr << "One message on cerr with newline\n";
}


void PrintUsage()
{
    std::cout << "Usage: DbgMsgSrc <opt>\n"
                 "\n"
                 "  -1 read 'titan_crash_debugview_43mb.log' and output it through OutputDebugStringA\n"
                 "  -2 <filename> read <filename> and output it through OutputDebugStringA\n"
                 "  -3 run endless test\n"
                 "  -s run -n repeatedly (10x / second) in separate processes)\n"
                 "  -w Send OutputDebugStringA 'WithoutNewLine'\n"
                 "  -n Send OutputDebugStringA 'WithNewLine\\n'\n"
                 "  -e Send empty OutputDebugStringA message (does not trigger DBwinMutex!)\n"
                 // about -4:
                 // we cannot guarantee what happens if 1x OutputDebugStringA is send
                 // the process might already be gone be time we handle the message
                 // however, if 2 messages are send, the process is guarenteed to be alive
                 // at least after receiving the first message but before setting the m_dbWinBufferReady flag..
                 // (because before setting the flag the traced process is still waiting for the flag)
                 // this means sending 2 messages and dieing ASAP afterwards is the worst-case we can still handle reliablely.
                 "  -4 Send 2x OutputDebugStringA 'WithNewLine\\n' (process handle cache test)\n"
                 "  -5 Send OutputDebugStringA '1\\n2\\n3\\n'\n"
                 "  -6 Send OutputDebugStringA '1 ' '2 ' '3\\n' in separate messages\n"
                 "  -7 DbgMsgTest, sends 5 different test lines, using different newlines styles\n"
                 "  -8 <frequency> DbgMsgSrc, Send OutputDebugStringA test lines with the specified frequency\n"
                 "  -9 DBGVIEWCLEAR test\n"
                 "  -A cout/cerr test\n"
                 "  -u <address> <port> Send UDP messsages to address:port\n";
    "  -b Send 4 lines large enough to rule out any small string optimizations (50 chars)\n";
    "  -c Send 2 lines with utf-8 encoded unicode (chinese characters)\n";
}

int Main(int argc, char* argv[])
{
    std::cout << "DbgMsgSrc, pid: " << GetCurrentProcessId() << std::endl;

    //HANDLE handle1 = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId());
    //HANDLE handle2 = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId());
    //HANDLE handle3 = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId());
    //printf(" %p, %p, %p\n", handle1, handle2, handle3);

    for (int i = 1; i < argc; ++i)
    {
        std::string arg(argv[i]);
        if (arg == "-1")
        {
            TestODS("titan_crash_debugview_43mb.log");
            return 0;
        }
        if (arg == "-1n")
        {
            TestSendMessage("titan_crash_debugview_43mb.log");
            return 0;
        }
        else if (arg == "-2")
        {
            if (i + 1 < argc)
            {
                TestODS(argv[i + 1]);
            }
            else
            {
                PrintUsage();
                return -1;
            }
            return 0;
        }
        else if (arg == "-3")
        {
            EndlessTest();
            return 0;
        }
        else if (arg == "-s") // run separate process test
        {
            SeparateProcessTest();
            return 0;
        }
        else if (arg == "-w")
        {
            std::cout << "Send OutputDebugStringA 'WithoutNewLine ' (15 bytes)\n";
            OutputDebugStringA("WithoutNewLine ");
            return 0;
        }
        else if (arg == "-n")
        {
            std::cout << "Send OutputDebugStringA 'With-a-NewLine \\n' (16 bytes)\n";
            OutputDebugStringA("With-a-NewLine \n");
            return 0;
        }
        else if (arg == "-e")
        {
            std::cout << "Send empty OutputDebugStringA message (0 bytes)\n";
            OutputDebugStringA(""); //empty message
            return 0;
        }
        else if (arg == "-4")
        {
            std::cout << "Send 2x OutputDebugStringA 'WithNewLine\\n (24 bytes)'\n";
            OutputDebugStringA("WithNewLine\n");
            OutputDebugStringA("WithNewLine\n");
            return 0;
        }
        else if (arg == "-5")
        {
            std::cout << "Send OutputDebugStringA '1\\n2\\n3\\n' (6 bytes)\n";
            OutputDebugStringA("1\n2\n3\n");
            return 0;
        }
        else if (arg == "-6")
        {
            std::cout << "Send OutputDebugStringA '1 ' '2 ' '3\\n' in separate messages (6 bytes)\n";
            OutputDebugStringA("1 ");
            OutputDebugStringA("2 ");
            OutputDebugStringA("3\n");
            return 0;
        }
        else if (arg == "-7")
        {
            DbgMsgTest();
            return 7;
        }
        else if (arg == "-8")
        {
            if (i + 1 < argc)
            {
                DbgMsgSrc(std::stoi(argv[i + 1]));
            }
            else
            {
                PrintUsage();
                return -1;
            }
            return 0;
        }
        else if (arg == "-9")
        {
            DbgMsgClearTest();
            return 0;
        }
        else if (arg == "-A")
        {
            CoutCerrTest();
            return 0;
        }
        else if (arg == "-u")
        {
            if (i + 2 < argc)
            {
                UdpTest(argv[i + 1], std::stoi(argv[i + 2]));
                return 0;
            }
            PrintUsage();
            return -1;
        }
        else if (arg == "-B")
        {
            CoutCerrTest2();
            return 0;
        }
        else if (arg == "-C")
        {
            SocketTest();
            return 0;
        }
        else if (arg == "-D")
        {
            OutputDebugStringA("test1\n");
            OutputDebugStringA("\n");
            OutputDebugStringA("\n\n");
            OutputDebugStringA("test2\n\n");
            OutputDebugStringA("test3\n");
            return 0;
        }
        else if (arg == "-b")
        {
            OutputDebugStringA("test long string with enough chars to exceed small string optimizations (SSO)\n"); // 79 chars + '\n' + '0';
            OutputDebugStringA("test long string with enough chars to exceed small string optimizations (SSO)\n"); // 79 chars + '\n' + '0';
            return 0;
        }
        else if (arg == "-c")
        {
            OutputDebugStringW(L"writing \u82F1\u8BED\n");                                // ??
            OutputDebugStringW(L"some \u4E2D\u6587 and more \u4E2D\u6587\u4FE1\u606F\n"); // ????
            return 0;
        }
        else if (arg == "-no")
        {
            TestSendMessage("test long string with enough chars to exceed small string optimizations (SSO)\n");
            TestSendMessage("test long string with enough chars to exceed small string optimizations (SSO)\n");
            return 0;
        }
        else
        {
            TestODS(arg);
            return 0;
        }
    }
    PrintUsage();
    return EXIT_SUCCESS;
}

} // namespace DbgMsgSrc
} // namespace fusion

int main(int argc, char* argv[])
try
{
    return fusion::DbgMsgSrc::Main(argc, argv);
}
catch (std::exception& ex)
{
    std::cerr << ex.what() << std::endl;
    return EXIT_FAILURE;
}
