#include "Core/ApplicationCore.hpp"

#include "Interfaces/IID.hpp"
#include "Interfaces/IInputPump.hpp"
#include "Interfaces/IRenderer.hpp"
#include "Interfaces/Package.hpp"
#include "Utils/TObject.hpp"
#include "Utils/Vec2F.hpp"

#include <cassert>
#include <chrono>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <dlfcn.h>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <chrono>

/* Root */

ApplicationCore::Root::Root() = default;

void ApplicationCore::Root::render()
{
    renderChildren();
}

bool ApplicationCore::Root::processEvent(const IInputPump::Event& event)
{
    if (children().empty())
    {
        return false;
    }

    return children().back()->processEvent(event);
}

Vec2F ApplicationCore::Root::mapGlobalToLocal(const Vec2F& pos) const
{
    return pos;
}

Vec2F ApplicationCore::Root::mapLocalToGlobal(const Vec2F& pos) const
{
    return pos;
}

void ApplicationCore::Root::pushModal(std::unique_ptr<TObject> object)
{
    children().push_back(std::move(object));
}

void ApplicationCore::Root::popModal()
{
    children().pop_back();
}

/* ApplicationCore */

ApplicationCore::ApplicationCore() : root_(std::make_unique<Root>())
{
}

ApplicationCore::~ApplicationCore()
{
    /* XXX: never change or replace this */
    root_.reset();
    /* --------------------------------- */

    for (auto handle = package_handles_.rbegin(); handle != package_handles_.rend(); ++handle)
    {
        dlclose(*handle);
    }
}

const char* ApplicationCore::strError(Error error)
{
    using enum Error;

    const char* str = "Unknow error";

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
            str = dlerror();
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

ApplicationCore& ApplicationCore::instance()
{
    static ApplicationCore instance;
    return instance;
}

ApplicationCore::Error ApplicationCore::queryInterface(IID iid, void** interface) const
{
    if (interface == nullptr)
    {
        return Error::INVALID_ARGS;
    }

    auto found = registry_.find(iid);
    if (found != registry_.end())
    {
        *interface = found->second;
        assert(*interface);

        return Error::SUCCESS;
    }

    return Error::INTERFACE_NOT_FOUND;
}

void ApplicationCore::pushCommand(std::unique_ptr<ACommand> command)
{
    commands_.push(std::move(command));
}

TObject* ApplicationCore::findObject(TObject::ID object_id) const
{
    return root_->findChild(object_id);
}

void ApplicationCore::removeObject(TObject::ID object_id)
{
    root_->removeChild(object_id);
}

void ApplicationCore::pushModal(std::unique_ptr<TObject> object)
{
    root_->pushModal(std::move(object));
}

void ApplicationCore::popModal()
{
    root_->popModal();
}

void ApplicationCore::setFPS(FPS fps)
{
    microseconds_per_frame_ = std::chrono::microseconds(static_cast<size_t>(1e6 / fps));
}

ApplicationCore::Error ApplicationCore::run()
{
    if (is_running_)
    {
        return Error::APPLICATION_ALREADY_RUNNING;
    }

    IRenderer* renderer = nullptr;
    if (queryInterface(IID_RENDERER, reinterpret_cast<void**>(&renderer)) != Error::SUCCESS)
    {
        return Error::NOT_LOADED_RENDERER;
    }

    IInputPump* input_pump = nullptr;
    if (queryInterface(IID_INPUT_PUMP, reinterpret_cast<void**>(&input_pump)) != Error::SUCCESS)
    {
        return Error::NOT_LOADED_INPUT_PUMP;
    }


    auto next_time = std::chrono::steady_clock::now();

    is_running_ = true;
    while (is_running_)
    {
        while (auto polled = input_pump->pollEvent())
        {
            root_->processEvent(*polled);

            while (!commands_.empty())
            {
                commands_.front()->execute();
                commands_.pop();
            }
        }

        root_->render();
        renderer->update();

        next_time += microseconds_per_frame_;
        auto now = std::chrono::steady_clock::now();
        if (next_time < now)
        {
            next_time = now;
        }
        std::this_thread::sleep_until(next_time);
    }

    is_running_ = false;
    return Error::SUCCESS;
}
void ApplicationCore::stop()
{
    is_running_ = false;
}

void ApplicationCore::log(LogLevel level, const char* fmt, ...)
{
    const char* log_level_str = "";

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

    std::va_list args;
    va_start(args, fmt);
    std::vfprintf(stderr, fmt, args);
    va_end(args);

    /* dubiously, but sometimes helpful */
    std::fputc('\n', stderr);
}

ApplicationCore::Error ApplicationCore::registerInterface(IID iid, void* interface)
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
    void* handle = dlopen(path.c_str(), RTLD_NOW);
    if (handle == nullptr)
    {
        return Error::PACKAGE_NOT_FOUND;
    }

    InitPackageFunc* init_package = reinterpret_cast<InitPackageFunc*>(dlsym(handle, INIT_PACKAGE_FUNC_NAME_STR));
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
