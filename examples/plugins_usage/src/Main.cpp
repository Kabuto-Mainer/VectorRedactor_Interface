#include "Core/ApplicationCore.hpp"
#include <dlfcn.h>
#include <string>
#include <vector>

int main()
{
    const std::vector<std::string> packages_to_load = { "./MainPlugin.so", "./GUIEngine.so" };

    ApplicationCore& app = ApplicationCore::instance();
    ApplicationCore::Error error = ApplicationCore::Error::SUCCESS;

    for(const auto& package : packages_to_load)
    {
	    if ((error = app.loadPackage(package)) != ApplicationCore::Error::SUCCESS)
	    {
		    ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "%s", ApplicationCore::strError(error));
		    if(error == ApplicationCore::Error::PACKAGE_NOT_FOUND)
		    {
			    ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "dlerror = %s", dlerror());
		    }
		    return 1;
	    }
    }

    if((error = app.run()) != ApplicationCore::Error::SUCCESS)
    {
	    ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Application exited with error %s", ApplicationCore::strError(error));
	    return 1;
    }

    ApplicationCore::log(ApplicationCore::LogLevel::OK, "Application successfully stopped");
    return 0;
}
