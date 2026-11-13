#pragma once

#include "Interfaces/IID.hpp"
#include "Utils/TObject.hpp"

#include <memory>

constexpr static const IID IID_DESKTOP = 4;

class IDesktop
{
public:
    IDesktop()          = default;
    virtual ~IDesktop() = default;

    IDesktop(const IDesktop&)            = delete;
    IDesktop(IDesktop&&)                 = delete;
    IDesktop& operator=(const IDesktop&) = delete;
    IDesktop& operator=(IDesktop&&)      = delete;

    enum class Error;
    
    virtual Error add(std::unique_ptr<TObject> object) = 0;
    virtual Error remove(TObject::ID object_id)        = 0;
};

enum class IDesktop::Error
{
    SUCCESS,
    INVALID_ARGS,
    OBJECT_NOT_FOUND,
};