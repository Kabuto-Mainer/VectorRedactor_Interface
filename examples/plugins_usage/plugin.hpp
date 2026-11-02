#pragma once

/* ------------------------- IUnknown ------------------------- */

class Core;

/*
 * IPlugin may be inherited from IUnknown,
 * but at the same time it isn't needed to
 * query interface from an exisiting IPlugin*
 */
class IPlugin
{
public:
    IPlugin()          = default;
    virtual ~IPlugin() = default;

    virtual void init(Core* core) = 0;
    virtual void shutDown()       = 0;
};

extern "C" IPlugin* getPluginInterface();
using PluginInterfaceGetter = decltype(getPluginInterface);

/* ------------------------------------------------------------ */

class ILogger : public IPlugin
{
public:
    ILogger()          = default;
    virtual ~ILogger() = default;

    virtual void log(const char* fmt, ...) = 0;
};

/* Logger - final class, singleton */

class Logger final : public ILogger
{
public:
    Logger(const Logger&)            = delete;
    Logger(Logger&&)                 = delete;
    Logger& operator=(const Logger&) = delete;
    Logger& operator=(Logger&&)      = delete;

    void init(Core* core) override;
    void shutDown() override;

    void log(const char* fmt, ...) override;

private:
    Logger()  = default;
    ~Logger() = default;

    /* actual interface getter */
    static Logger* _interface();
    /* C-linked interface getter */
    friend IPlugin* ::getPluginInterface();
};
