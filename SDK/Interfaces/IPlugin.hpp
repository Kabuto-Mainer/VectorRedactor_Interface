#ifndef I_PLUGIN_HPP
#define I_PLUGIN_HPP

#include "Interfaces/IUnknown.hpp"

// NOTE: Forward declaration
class IApplicationCore;

class IPlugin : public IUnknown {
public:
    virtual HRESULT init(IApplicationCore* core) = 0;
    virtual HRESULT shutdown() = 0;
};

#endif