#ifndef APPLICATION_CORE_HPP
#define APPLICATION_CORE_HPP

#include "Interfaces/IID.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

class ApplicationRoot;

class ApplicationCore
{
public:
    enum class Error;
    enum class LogLevel;

public:
    static const char* strError(Error error);
    static void        log(LogLevel level, const char* fmt, ...);

    static ApplicationCore& instance();

public:
    Error registerInterface(IID iid, void* interface);
    Error loadPackage(const std::string& path);

    ApplicationRoot*       root();
    ApplicationRoot const* root() const;
    Error                  queryInterface(IID iid, void** interface);
    Error                  queryInterface(IID iid, void const** interface) const;

    Error run();
    void  stop();

private:
    ApplicationCore();
    ~ApplicationCore();

    ApplicationCore(const ApplicationCore&) = delete;
    ApplicationCore(ApplicationCore&&);

    ApplicationCore& operator=(const ApplicationCore&) = delete;
    ApplicationCore& operator=(ApplicationCore&&)      = delete;

private:
    std::unique_ptr<ApplicationRoot> root_{};
    std::unordered_map<IID, void*>   registry_{};
    std::vector<void*>               package_handles_{};
    bool                             is_running_{false};
};

enum class ApplicationCore::Error
{
    SUCCESS,
    INVALID_ARGS,
    APPLICATION_ALREADY_RUNNING,
    INTERFACE_NOT_FOUND,
    INTERFACE_ALREADY_EXISTS,
    NOT_LOADED_RENDERER,
    NOT_LOADED_INPUT_PUMP,
    PACKAGE_NOT_FOUND,
    PACKAGE_INVALID_BINARY,
    PACKAGE_INITIALIZATION_FAILED,
};

enum class ApplicationCore::LogLevel
{
    OK,
    WARNING,
    ERROR,
    CRITICAL,
};

#endif
