#if 0
#include "SDK/Core/ApplicationCore.hpp"
#include "Interfaces/IDocument.hpp"
#include "Interfaces/Package.hpp"
#include "Interfaces/IID.hpp"
#include "Document.hpp"

template <typename T> T *quaryInterface(IID iid) {
    void* raw = nullptr;
  if (ApplicationCore::instance().queryInterface(iid, &raw) !=
      ApplicationCore::Error::SUCCESS) {
    return nullptr;
  }

  return static_cast<T*>(raw);
}

extern "C" bool INIT_PACKAGE_FUNC_NAME() {
  // auto document = quaryInterface<IDocument>(IID_IDOCUMENT);
    auto document = std::make_unique<Yasha::Document>;
    ApplicationCore::instance().registerInterface(IID_IDOCUMENT, document);
}    
#endif
