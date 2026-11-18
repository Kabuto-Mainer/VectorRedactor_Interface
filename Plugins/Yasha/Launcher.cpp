#include "Desktop.hpp"
#include "Document.hpp"
#include "Core/ApplicationCore.hpp"

#include <fstream> 

static constexpr const char* PLUGINS_LIST_FILENAME = "plugins_list.txt";

int main() {
    std::ifstream plugins_list_file(PLUGINS_LIST_FILENAME, std::ios::in);
    if (!plugins_list_file.is_open())
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Required '%s' is not found", PLUGINS_LIST_FILENAME);
        return EXIT_FAILURE;
    }

    ApplicationCore::Error error = initializeDesktop();
    if (error != ApplicationCore::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Desktop initialization failed: %s",
                             ApplicationCore::strError(error));
        return EXIT_FAILURE;
    }
    error = initializeDocument();
    if (error != ApplicationCore::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Document initialization failed: %s",
                             ApplicationCore::strError(error));
        return EXIT_FAILURE;
    }
    std::string            plugin_file;

    // TODO: подумать можно ли вынести в отдельный метод

    while (plugins_list_file >> plugin_file)
    {
        error = ApplicationCore::instance().loadPackage(plugin_file);
        if (error != ApplicationCore::Error::SUCCESS)
        {
            ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Error while '%s' plugin loading: %s",
                                 plugin_file.c_str(), ApplicationCore::strError(error));
            return EXIT_FAILURE;
        }

        ApplicationCore::log(ApplicationCore::LogLevel::OK, "'%s' loaded", plugin_file.c_str());
    }

    error = ApplicationCore::instance().run();
    if (error != ApplicationCore::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Application exited with error: %s",
                             ApplicationCore::strError(error));
        return EXIT_FAILURE;
    }

    ApplicationCore::log(ApplicationCore::LogLevel::OK, "application successfully exited");
    return EXIT_SUCCESS;
    
}
