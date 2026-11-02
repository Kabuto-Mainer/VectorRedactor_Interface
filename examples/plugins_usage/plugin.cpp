#include "plugin.hpp"

#include <cstdarg>
#include <cstdio>

Logger* Logger::_interface(void)
{
    static Logger interface;
    return &interface;
}

void Logger::init(Core*)
{
    return;
}

void Logger::shutDown(void)
{
    return;
}

extern "C" IPlugin* getPluginInterface(void)
{
    return Logger::_interface();
}

void Logger::log(const char* fmt, ...)
{
    std::va_list args;
    vfprintf(stderr, fmt, args);
}
