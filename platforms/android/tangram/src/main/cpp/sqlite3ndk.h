/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Krystian Bigaj code.
 *
 * The Initial Developer of the Original Code is
 * Krystian Bigaj (krystian.bigaj@gmail.com).
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef _SQLITE3_NDK_H_
#define _SQLITE3_NDK_H_

#include <sys/types.h>
#include <android/asset_manager.h>

#ifndef SQLITE_NDK_VFS_NAME
// Default name for VFS
#define SQLITE_NDK_VFS_NAME "ndk-asset"
#endif

#ifndef SQLITE_NDK_VFS_MAKE_DEFAULT
// Default sqlite3_ndk_init parameter
#define SQLITE_NDK_VFS_MAKE_DEFAULT 0
#endif

// Default sqlite3_ndk_init parameter
#define SQLITE_NDK_VFS_PARENT_VFS NULL

#ifndef SQLITE_NDK_VFS_MAX_PATH
// Maximum path name for database files
#define SQLITE_NDK_VFS_MAX_PATH 512
#endif

/*
 * This function registers VFS into SQLite.
 * It should be called only once (before SQLite-NDK usage).
 *
 * Params:
 * - assetMgr - pointer to AAssetManager. In most cases it will be:
 *   app->activity->assetManager (see example below).
 *   This parameter is required
 * - vfsName - name of VFS that can be used in sqlite3_open_v2
 *   as 4th parameter (http://www.sqlite.org/c3ref/open.html)
 *   or in URI filename (http://www.sqlite.org/uri.html).
 *   If NULL then default name "ndk-asset" is used (SQLITE_NDK_VFS_NAME)
 * - makeDflt - flag used to register SQLite-NDK as a default VFS.
 *   See: sqlite3_vfs_register at http://www.sqlite.org/c3ref/vfs_find.html
 *   Disabled by default (SQLITE_NDK_VFS_MAKE_DEFAULT)
 * - osVfs - name of VFS that will used only to redirect few sqlite calls.
 *   If NULL passed, then default VFS will be used (SQLITE_NDK_VFS_PARENT_VFS)
 *
 * Example:
 * void android_main(struct android_app* app)
 * {
 *   sqlite3_ndk_init(app->activity->assetManager);
 *   ...
 *   if (sqlite3_open_v2("data.sqlite3", &db, SQLITE_OPEN_READONLY,
 *     SQLITE_NDK_VFS_NAME) == SQLITE_OK)
 *   {
 *     ...
 */
int sqlite3_ndk_init(AAssetManager* assetMgr,
		const char* vfsName = SQLITE_NDK_VFS_NAME,
		int makeDflt = SQLITE_NDK_VFS_MAKE_DEFAULT,
		const char *osVfs = SQLITE_NDK_VFS_PARENT_VFS);

#endif
