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

Parse_Result Fix_Parser::parse(std::string& data, int& curr_loc, int  &traversed, size_t length)
{
    // Start from the curr_loc of the data like data.data() index
    // start with |8...|10=| 
    // just after |8 we have |9=body_length so validate the body_length
    // |10=check_sum we need to check the checksum from |8 to the ..)|10=
    // check for the duplicates while adding the values 
    // need to separately handle the Fix use cases
    // case 1 (normal): 8=FIX.4.2|9=112|35=A|34=1|49=CLIENT1|52=20260513-13:45:00.000|56=SERVER1|98=0|108=30|10=185|
    // case 2 (multiple messages in the same data): 8=FIX.4.2|9=112|35=A|34=1|49=CLIENT1|52=20260513-13:45:00.000|56=SERVER1|98=0|108=30|10=185|8=FIX.4.2|9=112|35=A|34=1|49=CLIENT1|52=20260513-13:45:00.000|56=SERVER1|98=0|108=30|10=185|
    // case 3 (half messages in the middle of the data 1st incomplete and 2nd complete): 8=FIX.4.2|9=112|35=A|34=1|49=CLIENT1|52=20260513-13:45:00.000|56=SERVER1|98=0|108=30|8=FIX.4.2|9=112|35=A|34=1|49=CLIENT1|52=20260513-13:45:00.000|56=SERVER1|98=0|108=30|10=185|
    // case 4 :(half message in the end)-> we need to wait for the next data to come and then we can parse the complete message
    // case 5 duplicate tags in the message -> we can't parse the message and we can discard the message as it is not valid

    // we first decide which is incomplete and which is complete message
    // if |8= represent the start of new message if it present twice before the next |10= then can conclude 
    // the first message is incomplete and the second message is complete and we can parse the second 
    //message and keep the first message in the buffer for the next data to come and then we can parse the 
    // first message
    // if |8= is present only once before the next |10= then we can conclude that the message is complete
    // if |8= is present only once and there is no |10= then we can conclude that the message is incomplete 
    // and we can keep the message in the buffer for the next data to come and then we can parse the message
    // Let's return the status of the message is half message in the end waiting for the next data to come 
    std::string_view str(data.data() + curr_loc, length - curr_loc);
    size_t start = str.find("8=FIX.");
    if (start == std::string_view::npos)
    {   
        traversed = str.size(); // move the curr_loc to the end of the data as 
        return Parse_Result{false, false, std::nullopt};
    }
    size_t pos = str.find("|10=", start);
    if (pos == std::string_view::npos)
    {
        traversed = str.size();
        return Parse_Result{false, true, std::nullopt};
    }
    size_t end = str.find('|', pos + 1);
    if (end == std::string_view::npos)
    {
        traversed = str.size();
        return Parse_Result{false, true, std::nullopt};
    }
    end ++;
    size_t tmp = start+6; // move past "8=FIX."
    while (tmp < end)
    {
        tmp = str.find("8=FIX.", tmp);
        if (tmp == std::string_view::npos || tmp >= end)
            break;
        start = tmp; // move the start to the new |8= position
        tmp += 6; // move past "8=FIX."
    }
    traversed = end;
    std::string msg_string(str.substr(start, end-start));
    std::cout << "Extracted message string: " << msg_string << "\n";
    if (!validate_fix_message(msg_string) && !validateFixBodyLength(msg_string))
    {
        return Parse_Result{false, false, std::nullopt};
    }
    Fix_Message fmsg;
    fmsg.fix_str = msg_string;
    int i = 0;
    while (i < msg_string.length())
    {
        if (msg_string[i] < '0' || msg_string[i] > '9')
        {
            i++;
            continue;
        }
        auto nxt = find_next(msg_string, i);
        fmsg.seq.push_back(nxt.first);
        if (fmsg.add_field(nxt.first, nxt.second))
        {
            // duplicate tag found, invalid message
            return Parse_Result{false, false, std::nullopt};
        }
    }
    return Parse_Result{true, false, fmsg};
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

bool Fix_Message::add_field(int tag, std::string val)
{
    if (msg.find(tag)==msg.end())
    {
        msg[tag] = val;
        return false;
    }
    else
    {
        return true;
    }
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
