#pragma once

#include "Interfaces/IDocument.hpp"
#include "Utils/TObject.hpp"
#include "Utils/Transformable.hpp"
#include "Utils/Vec2F.hpp"
#include "export/ICamera.hpp"

#include <memory>
#include <optional>

namespace Kabuto
{

namespace Document
{

class TDocument final : public TObject, public Transformable
{
public:
    TDocument()           = default;
    ~TDocument() override = default;

    TDocument(const TDocument&)            = delete;
    TDocument(TDocument&&)                 = delete;
    TDocument& operator=(const TDocument&) = delete;
    TDocument& operator=(TDocument&&)      = delete;

    void addShape(std::unique_ptr<IDocument::TShape> shape);
    void removeShape(TObject::ID shape_id);

    std::optional<TObject::ID> hitTest(const Vec2F& pos);

    void accept(IDocument::Visitor& visitor) const;

    bool  processEvent(const IInputPump::Event& event) override;
    void  render() override;
    Vec2F mapLocalToGlobal(const Vec2F& pos) const override;
    Vec2F mapGlobalToLocal(const Vec2F& pos) const override;
}; // class TDocument

class CDocument final : public IDocument, public ICamera
{
public:
    CDocument(const CDocument&)            = delete;
    CDocument(CDocument&&)                 = delete;
    CDocument& operator=(const CDocument&) = delete;
    CDocument& operator=(CDocument&&)      = delete;

    static CDocument& instance();

    void bindNode(TDocument& document);
    void unbindNode();

    void addShape(std::unique_ptr<TShape> shape) override;
    void removeShape(TObject::ID shape_id) override;

    std::optional<TObject::ID> hitTest(const Vec2F& pos) override;
    void                       accept(Visitor& visitor) const override;

    void addScale(float scale) override;
    void addScaleAt(float scale, const Vec2F& anchor) override;
    void addShift(const Vec2F& shift) override;
    void reset() override;

    Vec2F mapGlobalToLocal(const Vec2F& pos) const override;

private:
    CDocument();
    ~CDocument() override = default;

    TDocument* node() const;

    TDocument*  document_node_ = nullptr;
    TObject::ID document_id_;
    float       min_scale_ = ICamera::MIN_SCALE;
    float       max_scale_ = ICamera::MAX_SCALE;
}; // class CDocument

}; // namespace Document

} // namespace Kabuto
