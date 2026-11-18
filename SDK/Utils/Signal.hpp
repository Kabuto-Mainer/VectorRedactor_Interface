#pragma once

#include <functional>

/* NOTE:
 * Callback template class, uses for implementing callbacks in TObjects.
 *
 * Usage:
 * Signal< callback_args > signal;   <-- decl
 * signal.subscribe( new_callback ); <-- subscribing
 * signal( args );                   <-- signal sending
 */

template<typename... CBArgs>
class Signal
{
public:
    void subscribe(std::function<void(CBArgs...)> callback);
    void operator()(CBArgs... args) const;

private:
    std::vector<std::function<void(CBArgs...)>> callbacks_;
};

/* ======================= Implementation ======================= */

template<typename... CBArgs>
void Signal<CBArgs...>::subscribe(std::function<void(CBArgs...)> callback)
{
    callbacks_.push_back(callback);
}

template<typename... CBArgs>
void Signal<CBArgs...>::operator()(CBArgs... args) const
{
    for (auto callback : callbacks_)
    {
        callback(args...);
    }
}
