#include <iostream>
#include "fix_parser.h"

size_t Fix_Parser::calculateBodyLength(const std::string &fixMessage,
                                       char delimiter)
{
    // Find tag 9
    const std::string tag9Header = "|9=";

    size_t tag9Start = fixMessage.find(tag9Header);
    if (tag9Start == std::string::npos)
        return 0;

    // Find end of tag 9 field
    size_t tag9End = fixMessage.find(delimiter, tag9Start + 1);
    if (tag9End == std::string::npos)
        return 0;

    // Find start of tag 10
    const std::string tag10Header = "|10=";

    size_t tag10Start = fixMessage.find(tag10Header, tag9End + 1);
    if (tag10Start == std::string::npos)
        tag10Start = fixMessage.size() - 1; // If tag 10 is not found, consider body until the end of the message

    // Body starts after delimiter of tag 9
    size_t bodyStart = tag9End + 1;

    // Body ends right before tag 10
    size_t bodyEnd = tag10Start + 1;

    return (bodyEnd - bodyStart);
}

bool Fix_Parser::validateFixBodyLength(const std::string &fixMessage, char delimiter)
{
    const std::string tag9Header = "|9=";

    size_t tag9Start = fixMessage.find(tag9Header);
    if (tag9Start == std::string::npos)
        return false;

    size_t tag9End = fixMessage.find(delimiter, tag9Start + 1);
    if (tag9End == std::string::npos)
        return false;

    std::string lengthStr =
        fixMessage.substr(tag9Start + tag9Header.length(),
                          tag9End - (tag9Start + tag9Header.length()));

    int expectedLength = 0;

    try
    {
        expectedLength = std::stoi(lengthStr);
    }
    catch (const std::exception &e)
    {
        return false;
    }
    size_t actualLength =
        calculateBodyLength(fixMessage, delimiter);
    return (static_cast<int>(actualLength) == expectedLength);
}

bool Fix_Parser::validate_fix_message(std::string &msg)
{
    if (msg.size() < 10 ||
        msg.back() != '|' ||
        msg.substr(0, 6) != "8=FIX.")
    {
        return false;
    }

    bool has8 = false;
    bool has9 = false;
    bool has35 = false;

    size_t start = 0;

    while (start < msg.size())
    {
        const size_t end = msg.find('|', start);

        if (end == std::string_view::npos || end == start)
            return false;

        const size_t eq = msg.find('=', start);

        if (eq == std::string_view::npos || eq >= end)
            return false;

        // tag validation
        for (size_t i = start; i < eq; ++i)
        {
            if (!std::isdigit(static_cast<unsigned char>(msg[i])))
                return false;
        }

        // empty value
        if (eq + 1 == end)
            return false;

        // value validation
        for (size_t i = eq + 1; i < end; ++i)
        {
            const unsigned char c = msg[i];

            if (c < 32 || c > 126)
                return false;
        }

        // required tags
        const auto tag_len = eq - start;

        if (tag_len == 1 && msg[start] == '8')
            has8 = true;
        else if (tag_len == 1 && msg[start] == '9')
            has9 = true;
        else if (tag_len == 2 && msg[start] == '3' && msg[start + 1] == '5')
            has35 = true;

        start = end + 1;
    }

    return has8 && has9 && has35;
}

std::optional<Fix_Message> Fix_Parser::parse(char *data, size_t length)
{
    std::string_view msg(data, length);
    size_t pos = msg.find("|10=");
    if (pos == std::string_view::npos)
    {
        return std::nullopt;
    }
    size_t end = msg.find("|", pos + 4);
    if (end == std::string_view::npos)
    {
        return std::nullopt;
    }
    std::string str(msg.substr(0, end + 1));

    if (!validate_fix_message(str) && !validateFixBodyLength(str))
    {
        return std::nullopt;
    }
    Fix_Message fmsg;
    fmsg.fix_str = str;
    int i = 0;
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
        if (10 == x)
            continue;
        str.append(std::to_string(x));
        str.push_back('=');
        str.append(msg.msg[x]);
        str.push_back('|');
    }
    // let's fix tag 9
    int actual_length = calculateBodyLength(str);
    const std::string tag9Header = "|9=";
    size_t tag9Start = str.find(tag9Header);
    if (tag9Start != std::string::npos)
    {
        size_t tag9End = str.find('|', tag9Start + 1);
        if (tag9End != std::string::npos)
        {
            str.replace(tag9Start + tag9Header.length(),
                        tag9End - (tag9Start + tag9Header.length()),
                        std::to_string(actual_length));
        }
    }
    str.append("10=");
    str.append(std::to_string(calculate_checksum(str)));
    str.push_back('|');
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
        int num = 0;
        try
        {
            num = std::stoi(it->second);
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error("this tag is not an integer");
        }
        return num;
    }
    else
    {
        throw std::runtime_error("this tag is invalid");
    }
}
