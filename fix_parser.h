#pragma once

#include <map>
#include <string>
#include <unordered_map>
#include <optional>
#include <vector>
#include <string_view>
#include <cctype>

struct Fix_Message
{
    bool add_field(int tag, std::string val);
    bool has_field(int tag);
    std::string get_field(int tag);
    int get_int(int tag);
    std::vector<int> seq;
    std::string fix_str;
    std::unordered_map<int, std::string> msg;
};

struct Parse_Result
{
    bool is_valid;
    bool is_half_message;
    std::optional<Fix_Message> msg;
};
struct Fix_Parser
{
    bool validateFixBodyLength(const std::string &fixMessage, char delimiter = '|');
    size_t calculateBodyLength(const std::string &fixMessage,
                               char delimiter = '|');
    int calculate_checksum(const std::string &fix_msg);
    Parse_Result parse(std::string& str, int &curr_loc, int  &traversed, size_t length = 10240);
    std::string parse_to_string(Fix_Message &msg);
    bool validate_fix_message(std::string &msg);
    std::pair<int, std::string> find_next(std::string &s, int &i);
};