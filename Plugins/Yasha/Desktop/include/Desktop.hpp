#pragma once

#include "Core/ApplicationCore.hpp"
#include "Interfaces/IDesktop.hpp"
#include "Utils/TWidget.hpp"

#include <memory>

class Desktop final : public TWidget, public IDesktop
{
public:
    Error add(std::unique_ptr<TObject> object) override;
    Error remove(TObject::ID object_id) override;

    bool processEvent(const IInputPump::Event& event) override;
    void render() override;
};

ApplicationCore::Error initializeDesktop();
