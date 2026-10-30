#ifndef I_TOOL_HPP
#define I_TOOL_HPP

#include "Interfaces/IUnknown.hpp"
#include <memory>
// forward declaration
class Event;
class AObject;

class ITool : public IUnknown {
public: 
    virtual ~ITool() = default;
    
    // NECESSARY METHODS (USER NEEDS TO DEFINE THEM ALWAYS)
    virtual const char* getName() = 0;
    virtual std::unique_ptr<AObject> getSettingPanel() = 0;

    // HELPFUL METHODS THAT ARE NOT NECESSARY TO OVERRIDE
    // LEAVE THEM EMPTY BY DEFAULT, SO USER CAN OVERRIDE THEM
    // IF HE WANTS TO
    virtual void activate() {}
    virtual void deactivate() {}
public:
};

#endif
