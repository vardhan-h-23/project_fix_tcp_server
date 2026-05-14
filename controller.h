#pragma once

#include "fix_parser.h"
#include "tcp_server.h"
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operator
// 8=FIX.4.2|9=112|35=A|34=1|49=CLIENT1|52=20260513-13:45:00.000|56=SERVER1|98=0|108=30|10=185|
struct controller
{
    void OnMessageReceive(Fix_Message &msg, tcp_connection::pointer conn);
    bool validate_logon(Fix_Message &msg);
    bool validate_nos(Fix_Message &msg);
    std::unordered_map<std::string, int> active_connections;

    std::string getFormattedTime()
    {
        // Get current time with high precision
        auto now = std::chrono::system_clock::now();

        // Extract milliseconds
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      now.time_since_epoch()) %
                  1000;

        // Convert to time_t for localtime
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        struct tm *parts = std::localtime(&now_c);

        // Format the date and time: YYYYMMDD-HH:MM:SS
        std::ostringstream oss;
        oss << std::put_time(parts, "%Y%m%d-%H:%M:%S");

        // Append milliseconds
        oss << '.' << std::setfill('0') << std::setw(3) << ms.count();

        return oss.str();
    }
};