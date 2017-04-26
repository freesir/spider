#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(spider, 0.0.1, 411894032@qq.com)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([include/def.h])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
#AM_PROG_CC_C_O

# Checks for libraries.
AC_CHECK_LIB([pthread], [main])
# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC

AC_CHECK_FUNCS([dup2])
AC_CHECK_FUNCS([ftruncate])
AC_CHECK_FUNCS([bzero])
AC_CHECK_FUNCS([socket])


#AC_CHECK_HEADER_STDBOOL

AC_CHECK_HEADERS([sys/socket.h])
AC_CHECK_HEADERS([fcntl.h])
AC_CHECK_HEADERS([pthread.h])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
