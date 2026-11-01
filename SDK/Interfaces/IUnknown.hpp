#ifndef I_UNKNOWN_HPP
#define I_UNKNOWN_HPP

#include <cstdint>

using HRESULT = int32_t;
using ULONG   = uint32_t;

inline constexpr HRESULT S_OK          = 0;
inline constexpr HRESULT S_FALSE       = 1;
inline constexpr HRESULT E_NOTIMPL     = static_cast<HRESULT>(-1);
inline constexpr HRESULT E_NOINTERFACE = static_cast<HRESULT>(-2);
inline constexpr HRESULT E_POINTER     = static_cast<HRESULT>(-3);
inline constexpr HRESULT E_FAIL        = static_cast<HRESULT>(-4);
inline constexpr HRESULT E_UNEXPECTED  = static_cast<HRESULT>(-5);
inline constexpr HRESULT E_OUTOFMEMORY = static_cast<HRESULT>(-6);
inline constexpr HRESULT E_INVALIDARG  = static_cast<HRESULT>(-7);

constexpr bool SUCCEEDED(HRESULT hr) { return hr >= 0; }
constexpr bool FAILED(HRESULT hr)    { return hr < 0; }

using IID = uint64_t; 

class IUnknown {
public:
    virtual ~IUnknown() = default;
    
    virtual HRESULT queryInterface(const IID& iid, void** ppv) = 0;
    virtual ULONG   addRef() = 0;
    virtual ULONG   release() = 0;
};

#endif