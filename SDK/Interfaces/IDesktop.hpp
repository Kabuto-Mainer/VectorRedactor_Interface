#ifndef I_DESKTOP_HPP
#define I_DESKTOP_HPP

#include "Interfaces/IID.hpp"
#include "Utils/TObject.hpp"

#include <memory>

constexpr static const IID IID_DESKTOP = 0x07;

class IDesktop
{
public:
    virtual void addObject(std::unique_ptr<TObject> object) = 0;
    virtual void deleteObject(TObject::ID obj_id)           = 0;

}; // class IDesktop


#endif // I_DESKTOP_HPP
