#ifndef JSON_PARSER_HPP
#define JSON_PARSER_HPP

#include <cstddef>
#include <string>
#include <unordered_map>

namespace Json
{

using Fields = std::unordered_map<std::string, std::string>;

namespace detail
{

void skipSpaces(const std::string& text, std::size_t& pos);

bool parseString(const std::string& text, std::size_t& pos, std::string& out);

bool parseObject(const std::string& text, std::size_t& pos, const std::string& prefix, Fields& fields);

} // namespace detail

bool parse(const std::string& text, Fields& fields);

} // namespace Json

#endif
