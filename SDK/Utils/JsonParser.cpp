#include "SDK/Utils/JsonParser.hpp"

#include <cctype>

namespace Json
{

namespace detail
{

void skipSpaces(const std::string& text, std::size_t& pos)
{
    while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos])))
    {
        ++pos;
    }
}

bool parseString(const std::string& text, std::size_t& pos, std::string& out)
{
    if (pos >= text.size() || text[pos] != '"')
    {
        return false;
    }
    ++pos;
    out.clear();
    while (pos < text.size() && text[pos] != '"')
    {
        if (text[pos] == '\\' && pos + 1 < text.size())
        {
            ++pos;
        }
        out.push_back(text[pos++]);
    }
    if (pos >= text.size())
    {
        return false;
    }
    ++pos;
    return true;
}

bool parseObject(const std::string& text, std::size_t& pos, const std::string& prefix, Fields& fields)
{
    if (pos >= text.size() || text[pos] != '{')
    {
        return false;
    }
    ++pos;
    skipSpaces(text, pos);
    if (pos < text.size() && text[pos] == '}')
    {
        ++pos;
        return true;
    }

    while (true)
    {
        std::string key;
        skipSpaces(text, pos);
        if (!parseString(text, pos, key))
        {
            return false;
        }
        skipSpaces(text, pos);
        if (pos >= text.size() || text[pos] != ':')
        {
            return false;
        }
        ++pos;
        skipSpaces(text, pos);

        if (pos < text.size() && text[pos] == '{')
        {
            if (!parseObject(text, pos, prefix + key + ".", fields))
            {
                return false;
            }
        }
        else
        {
            std::string value;
            if (!parseString(text, pos, value))
            {
                return false;
            }
            fields[prefix + key] = value;
        }

        skipSpaces(text, pos);
        if (pos >= text.size())
        {
            return false;
        }
        if (text[pos] == ',')
        {
            ++pos;
            continue;
        }
        if (text[pos] == '}')
        {
            ++pos;
            return true;
        }
        return false;
    }
}

} // namespace detail

bool parse(const std::string& text, Fields& fields)
{
    std::size_t pos = 0;
    return detail::parseObject(text, pos, "", fields);
}

} // namespace Json
