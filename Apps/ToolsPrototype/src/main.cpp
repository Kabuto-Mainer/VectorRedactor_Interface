#include "Core/ApplicationCore.hpp"
#include "IUIFramework.hpp"

#include <cstdlib>
#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

namespace
{

constexpr const char* PLUGINS_LIST_FILENAME = "plugins_list.txt";

std::filesystem::path executableDirectory(const char* argv0)
{
    std::error_code             error;
    const std::filesystem::path self = std::filesystem::read_symlink("/proc/self/exe", error);
    if (!error && !self.empty())
    {
        return self.parent_path();
    }

    return std::filesystem::absolute(argv0, error).parent_path();
}

} // namespace

int main(int argc, char** argv)
{
    const std::filesystem::path app_dir      = executableDirectory(argc > 0 ? argv[0] : "app");
    const std::filesystem::path plugins_file = app_dir / PLUGINS_LIST_FILENAME;

    std::ifstream list(plugins_file, std::ios::in);
    if (!list.is_open())
    {
        ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Required '%s' is not found",
                             plugins_file.string().c_str());
        return EXIT_FAILURE;
    }

    ApplicationCore::log(ApplicationCore::LogLevel::OK, "Loading plugins from '%s'", plugins_file.string().c_str());

    std::string line;
    while (std::getline(list, line))
    {
        // обрезаем пробельные символы (включая '\r' от Windows-файлов)
        const std::size_t first = line.find_first_not_of(" \t\r");
        if (first == std::string::npos)
        {
            continue;
        }

        const std::size_t last        = line.find_last_not_of(" \t\r");
        const std::string plugin_file = line.substr(first, last - first + 1);

        // пропускаем строки-комментарии
        if (plugin_file.front() == '#' || plugin_file.starts_with("//"))
        {
            continue;
        }

        const std::filesystem::path plugin_path = app_dir / plugin_file;

        const ApplicationCore::Error error = ApplicationCore::instance().loadPackage(plugin_path.string());
        if (error != ApplicationCore::Error::SUCCESS)
        {
            ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "%s", dlerror());
            ApplicationCore::log(ApplicationCore::LogLevel::CRITICAL, "Error while '%s' plugin loading: %s",
                                 plugin_path.string().c_str(), ApplicationCore::strError(error));
            return EXIT_FAILURE;
        }

        ApplicationCore::log(ApplicationCore::LogLevel::OK, "'%s' loaded", plugin_path.string().c_str());
    }

    const ApplicationCore::Error error = ApplicationCore::instance().run();
    if (error != ApplicationCore::Error::SUCCESS)
    {
        ApplicationCore::log(ApplicationCore::LogLevel::ERROR, "Application exited with error: %s",
                             ApplicationCore::strError(error));
        return EXIT_FAILURE;
    }

    ApplicationCore::log(ApplicationCore::LogLevel::OK, "Application successfully exited");
    return EXIT_SUCCESS;
}
