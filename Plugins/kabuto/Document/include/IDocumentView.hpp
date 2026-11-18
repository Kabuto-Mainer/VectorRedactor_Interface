#pragma once

#include "Interfaces/IID.hpp"
#include "Utils/TObject.hpp"

constexpr static const IID IID_DOCUMENT_VIEW = 0x0'01'21;

/**
 * Access to the scene node that displays an IDocument.
 *
 * The document service itself is not a TObject. Plugins that need document
 * coordinates or document-layer placement query this interface and use the
 * returned node. The node is owned by the application scene tree.
 */
class IDocumentView
{
public:
    IDocumentView()          = default;
    virtual ~IDocumentView() = default;

    IDocumentView(const IDocumentView&)            = delete;
    IDocumentView(IDocumentView&&)                 = delete;
    IDocumentView& operator=(const IDocumentView&) = delete;
    IDocumentView& operator=(IDocumentView&&)      = delete;

    virtual TObject* view() const = 0;
}; // class IDocumentView
