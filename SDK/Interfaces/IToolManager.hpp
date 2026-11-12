#ifndef I_TOOL_MANAGER_HPP
#define I_TOOL_MANAGER_HPP

#include "Interfaces/IID.hpp"
#include "Interfaces/IInputPump.hpp"

constexpr static const IID IID_TOOL_MANAGER = 3;


class IToolManager
{
public:
    class ATool;

    IToolManager()          = default;
    virtual ~IToolManager() = default;

    IToolManager(const IToolManager&)            = delete;
    IToolManager(IToolManager&&)                 = delete;
    IToolManager& operator=(const IToolManager&) = delete;
    IToolManager& operator=(IToolManager&&)      = delete;

    enum class Error;

    class ATool
    {
    public:
        using ID = uint64_t;
        ATool()          = default;
        virtual ~ATool() = default;

        ATool(const ATool&)            = delete;
        ATool(ATool&&)                 = delete;
        ATool& operator=(const ATool&) = delete;
        ATool& operator=(ATool&&)      = delete;

        virtual ID id() const                       = 0;
        virtual bool   processEvent(const IInputPump::Event& event) = 0;
        /**
         * Если инструменту понадобится отрисовыватся - он
         * должен будет добавляться в дерево
         */
    };

    virtual Error addTool(ATool* tool)          = 0;
    virtual Error removeTool(ATool::ID* tool_id)   = 0;
    virtual Error setActiveTool(ATool::ID tool_id) = 0;

    virtual bool processEvent(const IInputPump::Event& event) = 0;

};

enum class IToolManager::Error
{
    SUCCESS,
    INVALID_ARGS,

    TOOL_NOT_FOUND,
};

#endif // I_TOOL_MANAGER_HPP
