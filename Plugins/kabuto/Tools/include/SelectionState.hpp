#pragma once

#include "Utils/BoxF.hpp"
#include "Utils/TObject.hpp"

#include <algorithm>
#include <optional>
#include <vector>

namespace Kabuto
{

namespace Tools
{

/**
 * Shared selection model.
 *
 * The tool mutates it, while SelectionOverlay only reads it. std::shared_ptr
 * keeps both sides alive independently of their destruction order.
 */
class SelectionState
{
public:
    SelectionState()  = default;
    ~SelectionState() = default;

    SelectionState(const SelectionState&)            = delete;
    SelectionState(SelectionState&&)                 = delete;
    SelectionState& operator=(const SelectionState&) = delete;
    SelectionState& operator=(SelectionState&&)      = delete;

    void selectOnly(TObject::ID shape_id);
    void add(TObject::ID shape_id);
    void remove(TObject::ID shape_id);
    void toggle(TObject::ID shape_id);
    void clear();

    bool                            isSelected(TObject::ID shape_id) const;
    bool                            empty() const;
    const std::vector<TObject::ID>& selected() const;

    void                       setMarquee(std::optional<BoxF> marquee);
    const std::optional<BoxF>& marquee() const;

private:
    std::vector<TObject::ID> selected_;
    std::optional<BoxF>      marquee_;
}; // class SelectionState

/* ====================== Implementation ====================== */

inline void SelectionState::selectOnly(TObject::ID shape_id)
{
    selected_.clear();
    selected_.push_back(shape_id);
}

inline void SelectionState::add(TObject::ID shape_id)
{
    if (!isSelected(shape_id))
    {
        selected_.push_back(shape_id);
    }
}

inline void SelectionState::remove(TObject::ID shape_id)
{
    selected_.erase(std::remove(selected_.begin(), selected_.end(), shape_id), selected_.end());
}

inline void SelectionState::toggle(TObject::ID shape_id)
{
    if (isSelected(shape_id))
    {
        remove(shape_id);
    }
    else
    {
        add(shape_id);
    }
}

inline void SelectionState::clear()
{
    selected_.clear();
}

inline bool SelectionState::isSelected(TObject::ID shape_id) const
{
    return std::find(selected_.begin(), selected_.end(), shape_id) != selected_.end();
}

inline bool SelectionState::empty() const
{
    return selected_.empty();
}

inline const std::vector<TObject::ID>& SelectionState::selected() const
{
    return selected_;
}

inline void SelectionState::setMarquee(std::optional<BoxF> marquee)
{
    marquee_ = marquee;
}

inline const std::optional<BoxF>& SelectionState::marquee() const
{
    return marquee_;
}

}; // namespace Tools

}; // namespace Kabuto
