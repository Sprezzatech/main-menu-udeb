/*
 * packages.h
 *
 * Copyright (C) 2003 Bastian Blank <waldi@debian.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id: packages.h,v 1.3 2003/09/26 00:18:09 waldi Exp $
 */

#ifndef DEBIAN_INSTALLER__SYSTEM__PACKAGES_H
#define DEBIAN_INSTALLER__SYSTEM__PACKAGES_H

#include <debian-installer/package.h>
#include <debian-installer/packages.h>

typedef struct di_system_package di_system_package;

/**
 * @defgroup di_system_packages Packages file - system
 * @{
 */

/**
 * @brief Package
 */
struct di_system_package
{
  di_package p;
  int installer_menu_item;
};

di_packages *di_system_packages_alloc (void);
di_packages_allocator *di_system_packages_allocator_alloc (void);

extern const di_parser_fieldinfo *di_system_package_parser_fieldinfo[];

di_parser_info *di_system_package_parser_info (void);
di_parser_info *di_system_packages_parser_info (void);
di_parser_info *di_system_packages_status_parser_info (void);

/**
 * Read a standard package control file
 *
 * @param file file to read
 */
static inline di_package *di_system_package_read_file (const char *file, di_packages_allocator *allocator)
{
  return di_package_special_read_file (file, allocator, di_system_package_parser_info);
}

/**
 * Read a standard Packages file
 *
 * @param file file to read
 */
static inline di_packages *di_system_packages_read_file (const char *file, di_packages_allocator *allocator)
{
  return di_packages_special_read_file (file, allocator, di_system_packages_parser_info);
}

/**
 * Read a standard status file
 *
 * @param file file to read
 */
static inline di_packages *di_system_packages_status_read_file (const char *file, di_packages_allocator *allocator)
{
  return di_packages_special_read_file (file, allocator, di_system_packages_status_parser_info);
}

/**
 * Write a standard Packages file
 *
 * @param file file to read
 */
static inline int di_system_packages_write_file (di_packages *packages, const char *file)
{
  return di_packages_special_write_file (packages, file, di_system_packages_parser_info);
}

/**
 * Write a standard status file
 *
 * @param file file to read
 */
static inline int di_system_packages_status_write_file (di_packages *packages, const char *file)
{
  return di_packages_special_write_file (packages, file, di_system_packages_status_parser_info);
}

/** @} */
#endif
