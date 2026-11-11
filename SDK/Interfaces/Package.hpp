#ifndef PACKAGE_HPP
#define PACKAGE_HPP

class ApplicationCore;

/* Returns:
 * true		package initialized successfully
 * false	errors occured
 */
using InitPackageFunc = bool();

#define INIT_PACKAGE_FUNC_NAME     initPackage
#define INIT_PACKAGE_FUNC_NAME_STR ("initPackage")

#endif
