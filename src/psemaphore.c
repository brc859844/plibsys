/* 
 * 22.08.2010
 * Copyright (C) 2010 Alexander Saprykin <xelfium@gmail.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
 */

/* TODO: Error report system */

#include "pmem.h"
#include "psemaphore.h"
#include "pcryptohash.h"

#include <stdlib.h>
#include <string.h>

#ifndef P_OS_WIN
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#else
#include <winsock2.h>
#include <windows.h>
#endif

#ifndef P_OS_WIN
pchar *
p_ipc_unix_get_temp_dir (void)
{
	pchar	*str, *ret;
	pint	len;

#ifdef P_tmpdir
	if (strlen (P_tmpdir) > 0)
		str = strdup (P_tmpdir);
	else
		return strdup ("/tmp/");
#else
	const pchar *tmp_env;

	tmp_env = getenv ("TMPDIR");

	if (tmp_env != NULL)
		str = strdup (tmp_env);
	else
		return strdup ("/tmp/");
#endif /* P_tmpdir */

	/* Now we need to ensure that we have only one trailing slash */
	len = strlen (str);
	while (*(str + --len) == '/')
		;
	*(str + ++len) = '\0';

	/* len + / + zero symbol */
	if ((ret = p_malloc (len + 2)) == NULL) {
		p_free (str);
		return NULL;
	}

	strcpy (ret, str);
	strcat (ret, "/");

	return ret;
}

/* Create file for System V IPC, if needed
 * Returns: -1 = error, 0 = file successfully created, 1 = file already exists */
pint
p_ipc_unix_create_key_file (const pchar *file_name)
{
	pint fd;

	if (file_name == NULL)
		return -1;

	if ((fd = open (file_name, O_CREAT | O_EXCL | O_RDONLY, 0640)) == -1)
		/* file already exists */
		return (errno == EEXIST) ? 1 : -1;
	else
		return close (fd);
}

pint
p_ipc_unix_get_ftok_key (const pchar *file_name)
{
	struct stat st_info;

	if (file_name == NULL)
		return -1;

	if (stat (file_name, &st_info) == -1)
		return -1;

	return ftok (file_name, 'P');
}
#endif /* !P_OS_WIN */

/* Returns platform-independent key for IPC usage, object name for Windows and
 * file name to use with ftok () for UNIX-like systems */
pchar *
p_ipc_get_platform_key (const pchar *name, pboolean posix)
{
	PCryptoHash	*sha1;
	pchar		*hash_str;

#ifndef P_OS_WIN
	pchar		*path_name, *tmp_path;
#endif

	if (name == NULL)
		return NULL;

	if ((sha1 = p_crypto_hash_new (P_CRYPTO_HASH_TYPE_SHA1)) == NULL)
		return NULL;

	p_crypto_hash_update (sha1, (const puchar *) name, strlen (name));

	hash_str = p_crypto_hash_get_string (sha1);
	p_crypto_hash_free (sha1);

	if (hash_str == NULL)
		return NULL;

#ifdef P_OS_WIN
	return hash_str;
#else
	if (posix) {
		/* POSIX semaphores which are named kinda like '/semname' */
		if ((path_name = p_malloc0 (sizeof (hash_str) + 2)) == NULL) {
			p_free (hash_str);
			return NULL;
		}

		strcpy (path_name, "/");
		strcat (path_name, hash_str);
	} else {
		tmp_path = p_ipc_unix_get_temp_dir ();

		/* tmp dir + filename + zero symbol */
		path_name = p_malloc0 (strlen (tmp_path) + 40 + 1);

		if ((path_name) == NULL) {
			p_free (tmp_path);
			p_free (hash_str);
			return NULL;
		}

		strcpy (path_name, tmp_path);
		strcat (path_name, hash_str);

		p_free (tmp_path);
		p_free (hash_str);
	}

		return path_name;
#endif
}
