#ifndef A_TEXT_HPP
#define A_TEXT_HPP

#include "Interfaces/IRenderer.hpp"
#include "Utils/AShape.hpp"
#include "Utils/BoxF.hpp"
#include "Utils/Event.hpp"
#include "Utils/Vec2F.hpp"

class AText final : public AShape
{
public:
    AText(std::string text = {}, Color color = Color::BLACK);
    ~AText();

    bool processEvent(Event const& event) override;
    virtual void render() const;

    bool hitTest(Vec2F const& pos) const override;
    virtual BoxF axesAlignedBoundingBox() const override;

private:
    std::string text_;
    size_t index_;

}; // class AText


#endif // A_TEXT_HPP
