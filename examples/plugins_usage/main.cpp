#include "plugin.hpp"

#include <cassert>
#include <dlfcn.h>
#include <err.h>

static const char* filename = "./plugin.so";

int main()
{
    void* logger_handle = dlopen(filename, RTLD_NOW);
    if (logger_handle == nullptr) errx(1, "dlopen: %s", dlerror());

    PluginInterfaceGetter* getInterface = (PluginInterfaceGetter*)dlsym(logger_handle, "getPluginInterface");
    if (getInterface == nullptr) err(1, "dlsym: %s", dlerror());

    ILogger* logger = dynamic_cast<ILogger*>(getInterface());
    assert(logger);

    logger->init(nullptr);
    logger->log("Hello, world, youpta\n");
    logger->shutDown();

    dlclose(logger_handle);
    return 0;
}
