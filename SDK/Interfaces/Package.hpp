#pragma once

/* Returns:
 * true		package initialized successfully
 * false	errors occured
 */
using InitPackageFunc = bool();

#define INIT_PACKAGE_FUNC_NAME     initPackage
#define EXPAND_STRING(string)      #string
#define INIT_PACKAGE_FUNC_NAME_STR (EXPAND_STRING(INIT_PACKAGE_FUNC_NAME))
