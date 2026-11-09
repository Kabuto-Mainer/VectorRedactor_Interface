#ifndef APPLICATION_CORE_HPP
#define APPLICATION_CORE_HPP

#include "Interfaces/IID.hpp"
#include "Utils/Base/AObject.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

class ApplicationCore
{
public:
    enum class Error;
    enum class LogLevel;
    class Root;

private:
    ApplicationCore();
    ~ApplicationCore();

    std::unordered_map<IID, void*> registry_;
    std::unique_ptr<Root>          root_;
    std::vector<void*>             package_handles_;
    bool                           is_running_ = false;

public:
    ApplicationCore(const ApplicationCore&)            = delete;
    ApplicationCore(ApplicationCore&&)                 = delete;
    ApplicationCore& operator=(const ApplicationCore&) = delete;
    ApplicationCore& operator=(ApplicationCore&&)      = delete;

    static const char*      strError(Error error);
    static ApplicationCore& instance();
    AObject*                root() const;
    Error                   queryInterface(IID iid, void** interface) const;

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

class ApplicationCore::Root final : public AObject
{
public:
    Root()  = default;
    ~Root() = default;

    bool  processEvent(const Event& event) override;
    void  render() override;
    Vec2F mapLocalToGlobal(const Vec2F& pos) const override;
    Vec2F mapGlobalToLocal(const Vec2F& pos) const override;

    bool pushModal(std::unique_ptr<AObject> window);
    bool popModal();
};

#endif
