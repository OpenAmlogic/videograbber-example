#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT([videograbber],[0.1],[reichi@opendreambox.org])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([main.c])
AC_CONFIG_HEADERS([videograbber_config.h])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_USE_SYSTEM_EXTENSIONS
AC_SYS_LARGEFILE

# Checks for programs.
AC_PROG_CC

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdio.h stdlib.h unistd.h sys/ioctl.h sys/mman.h sys/stat.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T

# Checks for library functions.
AC_FUNC_MMAP
AC_CHECK_FUNCS([memmove memset munmap])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
