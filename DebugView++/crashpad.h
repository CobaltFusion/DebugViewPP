// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)

// Repository at: https://github.com/djeedjay/DebugViewPP/

#pragma once

#include "../Libraries/crashpad/include/client/crashpad_client.h"

#pragma comment(lib, "crashpad_util.lib")
#pragma comment(lib, "base.lib")
#pragma comment(lib, "crashpad_client.lib")

static bool initializeCrashPad()
{
    using namespace crashpad;
    CrashpadClient client;

    std::map<std::string, std::string> annotations;
    std::vector<std::string> arguments;

    /*
    * ENSURE THIS VALUE IS CORRECT.
    *
    * This is the directory you will use to store and queue crash data.
    */
    std::wstring db_path(L"crashpad\\db");

    /*
    * ENSURE THIS VALUE IS CORRECT.
    *
    * Crashpad has the ability to support crashes both in-process and out-of-process.
    * The out-of-process handler is significantly more robust than traditional in-process
    * crash handlers. This path may be relative.
    */
    std::wstring handler_path(L"crashpad_handler.com");

    /*
    * YOU MUST CHANGE THIS VALUE.
    *
    * This should point to your server dump submission port (labeled as "http/writer"
    * in the listener configuration pane. Preferrably, the SSL enabled port should
    * be used. If Backtrace is hosting your instance, the default port is 6098.
    */
    std::string url("https://nowind.sp.backtrace.io:6098");
    /*
    * YOU MUST CHANGE THIS VALUE.
    *
    * Set this to the submission token under the project page.
    * Learn more at https://documentation.backtrace.io/coronerd_setup/#tokens
    */
    annotations["token"] = "14b153e8b2e7bb8b4df91536eb30899bb939d177a4ecfd36fe336a1e1b6531fd";

    /*
    * THE FOLLOWING ANNOTATIONS MUST BE SET.
    *
    * Backtrace supports many file formats. Set format to minidump so it knows
    * how to process the incoming dump.
    */
    annotations["format"] = "minidump";

    annotations["g_debugstate"] = "My extra info: 42!";


    /*
    * REMOVE THIS FOR ACTUAL BUILD.
    *
    * We disable crashpad rate limiting for this example.
    */
    arguments.push_back("--no-rate-limit");

    base::FilePath db(db_path);
    base::FilePath handler(handler_path);

    bool rc = client.StartHandler(handler,
                                  db,
                                  db,
                                  url,
                                  annotations,
                                  arguments,
                                  true,
                                  true);
    if (rc == false)
        return false;

    /* Optional, wait for Crashpad to initialize. */
    rc = client.WaitForHandlerStart(INFINITE);
    if (rc == false)
        return false;

    return true;
}
