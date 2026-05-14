#pragma once

#include <iostream>
#include "fix_parser.h"

bool Fix_Parser::validateFixBodyLength(const std::string &fixMessage, char delimiter)
{
    // 1. Find the end of Tag 8 (BeginString)
    std::string tag8Header = "8=";
    size_t tag8Start = fixMessage.find(tag8Header);
    if (tag8Start == std::string::npos)
        return false;

    size_t tag8End = fixMessage.find(delimiter, tag8Start);
    if (tag8End == std::string::npos)
        return false;

    // 2. Find and parse Tag 9 (BodyLength)
    std::string tag9Header = "9=";
    size_t tag9Start = fixMessage.find(tag9Header, tag8End + 1);
    // Ensure Tag 9 immediately follows Tag 8
    if (tag9Start != tag8End + 1)
        return false;

    size_t tag9End = fixMessage.find(delimiter, tag9Start);
    if (tag9End == std::string::npos)
        return false;

    // Extract the expected length value declared in the message
    std::string lengthStr = fixMessage.substr(tag9Start + tag9Header.length(), tag9End - (tag9Start + tag9Header.length()));
    int expectedLength = 0;
    try
    {
        expectedLength = std::stoi(lengthStr);
    }
    catch (...)
    {
        return false; // Invalid integer format in Tag 9
    }

    // 3. Find the start of Tag 10 (Checksum) to define the boundary
    std::string tag10Header = "10=";
    size_t tag10Start = fixMessage.find(tag10Header, tag9End + 1);
    if (tag10Start == std::string::npos)
        return false;

    // 4. Calculate actual body length
    // Body starts right after Tag 9's delimiter and ends right before Tag 10's '1'
    size_t bodyStart = tag9End + 1;
    size_t actualLength = tag10Start - bodyStart;
    
    return (static_cast<int>(actualLength) == expectedLength);
}

std::optional<Fix_Message> Fix_Parser::parse(std::string str)
{
    int i = 0;
    if (!validateFixBodyLength(str))
    {
        return std::nullopt;
    }
    Fix_Message fmsg;
    while (i < str.length())
    {
        if (str[i] < '0' || str[i] > '9')
        {
            i++;
            continue;
        }
        auto nxt = find_next(str, i);
        fmsg.seq.push_back(nxt.first);
        fmsg.add_field(nxt.first, nxt.second);
    }
    return fmsg;
}

int Fix_Parser::calculate_checksum(const std::string &fix_msg)
{
    int sum = 0;

    for (unsigned char ch : fix_msg)
    {
        sum += ch;
    }

    return sum % 256;
}

std::string Fix_Parser::parse_to_string(Fix_Message &msg)
{
    std::string str;
    for (auto x : msg.seq)
    {

        str.append(std::to_string(x));
        str.push_back('=');
        if (x == 10)
        {
            str.append(std::to_string(calculate_checksum(str)));
        }
        else
            str.append(msg.msg[x]);
        str.push_back('|');
    }
    return str;
}
// 8=FIX.4.2|9=112|35=A|34=1|49=CLIENT1|52=20260513-13:45:00.000|56=SERVER1|98=0|108=30|10=185|
std::pair<int, std::string> Fix_Parser::find_next(std::string &s, int &i)
{
    int tag = 0;
    while (s[i] >= '0' && s[i] <= '9')
    {
        tag *= 10;
        tag += s[i] - '0';
        i++;
    }
    if (s[i] == '=')
        i++;
    std::string val;
    while (s[i] != '|')
    {
        val.push_back(s[i]);
        i++;
    }
    if (s[i] == '|')
        i++;
    // std::cout << tag << " " << val << "\n";
    return std::make_pair(tag, val);
}

void Fix_Message::add_field(int tag, std::string val)
{
    msg[tag] = val;
}

bool Fix_Message::has_field(int tag)
{
    return msg.find(tag) != msg.end();
}

std::string Fix_Message::get_field(int tag)
{
    auto it = msg.find(tag);
    if (it != msg.end())
    {
        return it->second;
    }
    else
    {
        throw std::runtime_error("this tag is not present");
    }
}
int Fix_Message::get_int(int tag)
{
    auto it = msg.find(tag);
    if (it != msg.end())
    {
        return std::stoi(it->second);
    }
    else
    {
        throw std::runtime_error("this tag is invalid");
    }
}
