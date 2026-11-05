#include "Core/ApplicationCore.hpp"

#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/Package.hpp"
#include "Utils/ApplicationRoot.hpp"

#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <dlfcn.h>
#include <string>

const char* ApplicationCore::strError(Error const error)
{
    using enum Error;

    const char* str{"Unknown error"};

    switch (error)
    {
        case SUCCESS:
            str = "Success";
            break;
        case INVALID_ARGS:
            str = "Invalid arguments";
            break;
        case INTERFACE_NOT_FOUND:
            str = "Interface is not found";
            break;
        case APPLICATION_ALREADY_RUNNING:
            str = "Application is already running";
            break;
        case NOT_LOADED_RENDERER:
            str = "Renderer is not loaded";
            break;
        case NOT_LOADED_INPUT_PUMP:
            str = "Input pump is not loaded";
            break;
        case INTERFACE_ALREADY_EXISTS:
            str = "Attempt to add existing interface";
            break;
        case PACKAGE_NOT_FOUND:
            str = "Loading package is not found";
            break;
        case PACKAGE_INVALID_BINARY:
            str = "Package does not have init function";
            break;
        case PACKAGE_INITIALIZATION_FAILED:
            str = "Package initialization failed";
            break;

        default:
            break;
    }

    return str;
}

void ApplicationCore::log(LogLevel const level, const char fmt[], ...)
{
    const char* log_level_str{""};

    switch (level)
    {
        case LogLevel::OK:
            log_level_str = "[ ok ]";
            break;
        case LogLevel::WARNING:
            log_level_str = "[ warning ]";
            break;
        case LogLevel::ERROR:
            log_level_str = "[ ERROR ]";
            break;
        case LogLevel::CRITICAL:
            log_level_str = "[ CRITICAL ]";
            break;

        default:
            break;
    }

    std::fprintf(stderr, "%s ", log_level_str);

    std::va_list args{};
    va_start(args, fmt);
    std::vfprintf(stderr, fmt, args);
    va_end(args);

    /* dubiously, but sometimes helpful */
    std::fputc('\n', stderr);
}

ApplicationCore& ApplicationCore::instance()
{
    static ApplicationCore instance{};
    return instance;
}

ApplicationCore::ApplicationCore() : root_{std::make_unique<ApplicationRoot>()}
{
}

ApplicationCore::~ApplicationCore()
{
    /* XXX: never change or replace this */
    root_.reset();
    /* --------------------------------- */

    auto end_iterator{package_handles_.rend()};
    for (auto handle{package_handles_.rbegin()}; handle != end_iterator; ++handle)
    {
        dlclose(*handle);
    }
}

ApplicationCore::ApplicationCore(ApplicationCore&&) = default;

ApplicationCore::Error ApplicationCore::registerInterface(IID const iid, void* const interface)
{
    if (interface == nullptr)
    {
        return Error::INVALID_ARGS;
    }

    if (registry_.contains(iid))
    {
        return Error::INTERFACE_ALREADY_EXISTS;
    }

    registry_[iid] = interface;
    return Error::SUCCESS;
}

ApplicationCore::Error ApplicationCore::loadPackage(const std::string& path)
{
    void* const handle{dlopen(path.c_str(), RTLD_NOW)};
    if (handle == nullptr)
    {
        return Error::PACKAGE_NOT_FOUND;
    }

    InitPackageFunc* const init_package = reinterpret_cast<InitPackageFunc*>(dlsym(handle, INIT_PACKAGE_FUNC_NAME_STR));
    if (init_package == nullptr)
    {
        dlclose(handle);
        return Error::PACKAGE_INVALID_BINARY;
    }

    if (!init_package())
    {
        return Error::PACKAGE_INITIALIZATION_FAILED;
    }

    package_handles_.push_back(handle);

    return Error::SUCCESS;
}

ApplicationRoot* ApplicationCore::root()
{
    return root_.get();
}

ApplicationRoot const* ApplicationCore::root() const
{
    return root_.get();
}

ApplicationCore::Error ApplicationCore::queryInterface(IID const iid, void** const interface)
{
    if (interface == nullptr)
    {
        return Error::INVALID_ARGS;
    }

    auto const found{registry_.find(iid)};
    if (found != registry_.end())
    {
        *interface = found->second;
        assert(*interface);

        return Error::SUCCESS;
    }

    return Error::INTERFACE_NOT_FOUND;
}

ApplicationCore::Error ApplicationCore::queryInterface(IID const iid, void const** const interface) const
{
    if (interface == nullptr)
    {
        return Error::INVALID_ARGS;
    }

    auto const found{registry_.find(iid)};
    if (found != registry_.end())
    {
        *interface = found->second;
        assert(*interface);

        return Error::SUCCESS;
    }

    return Error::INTERFACE_NOT_FOUND;
}

ApplicationCore::Error ApplicationCore::run()
{
    /* already running => return */
    if (is_running_)
    {
        return Error::APPLICATION_ALREADY_RUNNING;
    }

    IRenderer* renderer{nullptr};
    if (queryInterface(IID_RENDERER, reinterpret_cast<void**>(&renderer)) != Error::SUCCESS)
    {
        return Error::NOT_LOADED_RENDERER;
    }

    IInputPump* input_pump{nullptr};
    if (queryInterface(IID_INPUT_PUMP, reinterpret_cast<void**>(&input_pump)) != Error::SUCCESS)
    {
        return Error::NOT_LOADED_INPUT_PUMP;
    }

    is_running_ = true;
    while (is_running_)
    {
        auto const polled{input_pump->pollEvent()};
        if (polled.has_value())
        {
            root_->processEvent(polled.value());
        }

        root_->render();
        renderer->update();
    }

    return Error::SUCCESS;
}

void ApplicationCore::stop()
{
    is_running_ = false;
}
