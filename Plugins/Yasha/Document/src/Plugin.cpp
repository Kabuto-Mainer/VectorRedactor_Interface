#if 0
#include "SDK/Core/ApplicationCore.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/Package.hpp"
#include "Interfaces/IID.hpp"
#include "Document.hpp"
#include "QueryInterface.hpp"

extern "C" bool INIT_PACKAGE_FUNC_NAME() {
  // auto document = queryInterface<IDocument>(IID_IDOCUMENT);
    auto document = std::make_unique<Yasha::Document>;
    ApplicationCore::instance().registerInterface(IID_IDOCUMENT, document);
}    
#endif
