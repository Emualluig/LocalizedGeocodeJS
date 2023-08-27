#ifndef __HEADER_STRINGSPLIT_HPP_CPP_
#define __HEADER_STRINGSPLIT_HPP_CPP_

#include <string>
#include <string_view>
#include <vector>
#include <array>

// This header requires C++17 or later
// These functions split a str into string_view based on the same rules as js string.split()
// ie: "abaabaaabbabbbabb", 'b' => "a", "aa", "aaa", "", "a", "", "", "a", "", ""

// Splits a string on the delimiter, similar to string.split in JavaScript
// Returns: A vector of string views
constexpr std::vector<std::string_view> split(const std::string& str, const char delim) noexcept
{
    std::vector<std::string_view> retval = {};
    std::pair<std::size_t, std::size_t> bounds = { 0, 0 };
    std::size_t i = 0;
    for (; i < str.size(); i++)
    {
        if (str[i] == delim)
        {
            retval.push_back(std::string_view(str.begin() + bounds.first, str.begin() + bounds.second));
            bounds = { i + 1, i + 1 };
        }
        else
        {
            bounds.second += 1;
        }
    }
    retval.push_back(std::string_view(str.begin() + bounds.first, str.begin() + bounds.second));

    return retval;
}

// Splits a string on the delimiter, similar to string.split in JavaScript
// Template parameters:
//      std::size_t maximum: The maximum number of splits to get.
// Returns: An array of splits, if not enough delimiters the elements at the end will be invalid
template <std::size_t maximum>
constexpr std::array<std::string_view, maximum> split(const std::string& str, const char delim) noexcept
{
    std::size_t index = 0;
    std::array<std::string_view, maximum> retval = {};
    std::pair<std::size_t, std::size_t> bounds = { 0, 0 };
    std::size_t i = 0;
    for (; i < str.size(); i++)
    {
        if (index == maximum)
        {
            return retval;
        }
        if (str[i] == delim)
        {
            if (index < maximum)
            {
                retval[index] = std::string_view(str.begin() + bounds.first, str.begin() + bounds.second);
                index++;
                bounds = { i + 1, i + 1 };
            }
            else
            {
                return retval;
            }
        }
        else
        {
            bounds.second += 1;
        }
    }
    if (index < maximum)
    {
        retval[index] = std::string_view(str.begin() + bounds.first, str.begin() + bounds.second);
    }

    return retval;
}

// Splits a string on the delimiter, similar to string.split in JavaScript
// Template parameters:
//      std::size_t maximum: The maximum number of splits to get.
// Effects: modifies count to be the number of segments in the split
// Returns: An array of splits, if not enough delimiters the elements at the end will be invalid
template <std::size_t maximum>
constexpr std::array<std::string_view, maximum> split(const std::string& str, const char delim, std::size_t& count) noexcept
{
    count = 0;
    std::array<std::string_view, maximum> retval = {};
    std::pair<std::size_t, std::size_t> bounds = { 0, 0 };
    std::size_t i = 0;
    for (; i < str.size(); i++)
    {
        if (count == maximum)
        {
            return retval;
        }
        if (str[i] == delim)
        {
            if (count < maximum)
            {
                retval[count] = std::string_view(str.begin() + bounds.first, str.begin() + bounds.second);
                count++;
                bounds = { i + 1, i + 1 };
            }
            else
            {
                return retval;
            }
        }
        else
        {
            bounds.second += 1;
        }
    }
    if (count < maximum)
    {
        retval[count] = std::string_view(str.begin() + bounds.first, str.begin() + bounds.second);
    }

    return retval;
}

#endif // !__HEADER_STRINGSPLIT_HPP_CPP_
