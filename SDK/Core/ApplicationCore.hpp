#pragma once

#include "Interfaces/IID.hpp"
#include "Interfaces/IInputPump.hpp"
#include "Utils/ACommand.hpp"
#include "Utils/TObject.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <queue>
#include <unordered_map>
#include <vector>

class ApplicationCore
{
public:
    enum class Error;
    enum class LogLevel;
    class Root;
    using FPS = uint32_t;

private:
    ApplicationCore();
    ~ApplicationCore();

    std::unordered_map<IID, void*>        registry_;
    std::unique_ptr<Root>                 root_;
    std::vector<void*>                    package_handles_;
    std::queue<std::unique_ptr<ACommand>> commands_;
    bool                                  is_running_ = false;

    /* default fps ~= 60 */
    static constexpr std::chrono::microseconds MICROSECONDS_PER_FRAME_DEFAULT = std::chrono::microseconds(16'667);
    std::chrono::microseconds                  microseconds_per_frame_        = MICROSECONDS_PER_FRAME_DEFAULT;

    static TObject* findObject(TObject* object, TObject::ID object_id);

public:
    ApplicationCore(const ApplicationCore&)            = delete;
    ApplicationCore(ApplicationCore&&)                 = delete;
    ApplicationCore& operator=(const ApplicationCore&) = delete;
    ApplicationCore& operator=(ApplicationCore&&)      = delete;

    static const char*      strError(Error error);
    static ApplicationCore& instance();
    Error                   queryInterface(IID iid, void** interface) const;
    void                    pushCommand(std::unique_ptr<ACommand> command);

    TObject* findObject(TObject::ID object_id) const;
    void     pushModal(std::unique_ptr<TObject> object);
    void     popModal();

    void  setFPS(FPS fps);
    Error run();
    void  stop();

    static void log(LogLevel level, const char* fmt, ...);
    Error       registerInterface(IID iid, void* interface);
    Error       loadPackage(const std::string& path);
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

class ApplicationCore::Root final : public TObject
{
public:
    Root();
    ~Root() = default;

    bool  processEvent(const IInputPump::Event& event) override;
    void  render() override;
    Vec2F mapLocalToGlobal(const Vec2F& pos) const override;
    Vec2F mapGlobalToLocal(const Vec2F& pos) const override;
}; // class Root
