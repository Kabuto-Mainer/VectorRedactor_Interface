#ifndef A_DOCUMENT_OBJECT_COMPONENT_HPP
#define A_DOCUMENT_OBJECT_COMPONENT_HPP

#include <cstdint>

// Component identifier. Unlike IID, CoID identifies a document object
// component rather than a service interface.
using CoID = uint64_t;

class ADocumentObjectComponent
{
public:
    ADocumentObjectComponent()          = default;
    virtual ~ADocumentObjectComponent() = default;

    ADocumentObjectComponent(const ADocumentObjectComponent&)            = delete;
    ADocumentObjectComponent(ADocumentObjectComponent&&)                 = delete;
    ADocumentObjectComponent& operator=(const ADocumentObjectComponent&) = delete;
    ADocumentObjectComponent& operator=(ADocumentObjectComponent&&)      = delete;

    // Every component reports the CoID it is registered under.
    virtual CoID coid() const = 0;
}; // class ADocumentObjectComponent

#endif // A_DOCUMENT_OBJECT_COMPONENT_HPP
