/* opkg.c - the opkg  package management system

   Thomas Wood <thomas@openedhand.com>

   Copyright (C) 2008 OpenMoko Inc

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
 */

#include "config.h"

#include <stdio.h>
#include <unistd.h>
#include <fnmatch.h>

#include "opkg.h"
#include "opkg_conf.h"

#include "opkg_install.h"
#include "opkg_configure.h"
#include "opkg_download.h"
#include "opkg_remove.h"
#include "opkg_upgrade.h"

#include "sprintf_alloc.h"
#include "file_util.h"

#include <libbb/libbb.h>

#define opkg_assert(expr) if (!(expr)) { \
    printf ("opkg: file %s: line %d (%s): Assertation '%s' failed",\
            __FILE__, __LINE__, __PRETTY_FUNCTION__, # expr); abort (); }

#define progress(d, p) d.percentage = p; if (progress_callback) progress_callback (&d, user_data);

/** Private Functions ***/

static int
opkg_configure_packages(char *pkg_name)
{
	pkg_vec_t *all;
	int i;
	pkg_t *pkg;
	int r, err = 0;

	all = pkg_vec_alloc();
	pkg_hash_fetch_available(all);

	for (i = 0; i < all->len; i++) {
		pkg = all->pkgs[i];

		if (pkg_name && fnmatch(pkg_name, pkg->name, 0))
			continue;

		if (pkg->state_status == SS_UNPACKED) {
			r = opkg_configure(pkg);
			if (r == 0) {
				pkg->state_status = SS_INSTALLED;
				pkg->parent->state_status = SS_INSTALLED;
				pkg->state_flag &= ~SF_PREFER;
			} else {
				if (!err)
					err = r;
			}
		}
	}

	pkg_vec_free(all);
	return err;
}

struct _curl_cb_data {
	opkg_progress_callback_t cb;
	opkg_progress_data_t *progress_data;
	void *user_data;
	int start_range;
	int finish_range;
};

int
curl_progress_cb(struct _curl_cb_data *cb_data, double t,	/* dltotal */
		double d,	/* dlnow */
		double ultotal, double ulnow)
{
	int p = (t) ? d * 100 / t : 0;
	static int prev = -1;
	int progress = 0;

	/* prevent the same value being sent twice (can occur due to rounding) */
	if (p == prev)
		return 0;
	prev = p;

	if (t < 1)
		return 0;

	progress = cb_data->start_range +
	    (d / t * ((cb_data->finish_range - cb_data->start_range)));
	cb_data->progress_data->percentage = progress;

	(cb_data->cb) (cb_data->progress_data, cb_data->user_data);

	return 0;
}


static struct opkg_conf saved_conf;
/*** Public API ***/

int
opkg_new()
{
	saved_conf = *conf;

	if (opkg_conf_init())
		goto err0;

	if (opkg_conf_load())
		goto err0;

	if (pkg_hash_load_feeds())
		goto err1;

	if (pkg_hash_load_status_files())
		goto err1;

	return 0;

err1:
	pkg_hash_deinit();
err0:
	opkg_conf_deinit();
	return -1;
}

void
opkg_free(void)
{
#ifdef HAVE_CURL
	opkg_curl_cleanup();
#endif
	opkg_conf_deinit();
}

int
opkg_re_read_config_files(void)
{
	opkg_free();
	*conf = saved_conf;
	return opkg_new();
}

void
opkg_get_option(char *option, void **value)
{
	int i = 0;
	extern opkg_option_t options[];

	/* look up the option
	 * TODO: this would be much better as a hash table
	 */
	while (options[i].name) {
		if (strcmp(options[i].name, option) != 0) {
			i++;
			continue;
		}
	}

	/* get the option */
	switch (options[i].type) {
	case OPKG_OPT_TYPE_BOOL:
		*((int *) value) = *((int *) options[i].value);
		return;

	case OPKG_OPT_TYPE_INT:
		*((int *) value) = *((int *) options[i].value);
		return;

	case OPKG_OPT_TYPE_STRING:
		*((char **) value) = xstrdup(options[i].value);
		return;
	}

}

void
opkg_set_option(char *option, void *value)
{
	int i = 0, found = 0;
	extern opkg_option_t options[];

	opkg_assert(option != NULL);
	opkg_assert(value != NULL);

	/* look up the option
	 * TODO: this would be much better as a hash table
	 */
	while (options[i].name) {
		if (strcmp(options[i].name, option) == 0) {
			found = 1;
			break;
		}
		i++;
	}

	if (!found) {
		opkg_msg(ERROR, "Invalid option: %s\n", option);
		return;
	}

	/* set the option */
	switch (options[i].type) {
	case OPKG_OPT_TYPE_BOOL:
		if (*((int *) value) == 0)
			*((int *) options[i].value) = 0;
		else
			*((int *) options[i].value) = 1;
		return;

	case OPKG_OPT_TYPE_INT:
		*((int *) options[i].value) = *((int *) value);
		return;

	case OPKG_OPT_TYPE_STRING:
		*((char **) options[i].value) = xstrdup(value);
		return;
	}

}

/**
 * @brief libopkg API: Install package
 * @param package_name The name of package in which is going to install
 * @param progress_callback The callback function that report the status to caller.
 */
int
opkg_install_package(const char *package_name,
		opkg_progress_callback_t progress_callback,
		void *user_data)
{
	int err;
	char *stripped_filename;
	opkg_progress_data_t pdata;
	pkg_t *old, *new;
	pkg_vec_t *deps, *all;
	int i, ndepends;
	char **unresolved = NULL;

	opkg_assert(package_name != NULL);

	/* ... */
	pkg_info_preinstall_check();


	/* check to ensure package is not already installed */
	old = pkg_hash_fetch_installed_by_name(package_name);
	if (old) {
		opkg_msg(ERROR, "Package %s is already installed\n",
				package_name);
		return -1;
	}

	new = pkg_hash_fetch_best_installation_candidate_by_name(package_name);
	if (!new) {
		opkg_msg(ERROR, "Couldn't find package %s\n", package_name);
		return -1;
	}

	new->state_flag |= SF_USER;

	pdata.action = -1;
	pdata.pkg = new;

	progress(pdata, 0);

	/* find dependancies and download them */
	deps = pkg_vec_alloc();
	/* this function does not return the original package, so we insert it later */
	ndepends = pkg_hash_fetch_unsatisfied_dependencies(new, deps,
			&unresolved);
	if (unresolved) {
		char **tmp = unresolved;
		opkg_msg(ERROR, "Couldn't satisfy the following dependencies"
			       " for %s:\n", package_name);
		while (*tmp) {
			opkg_msg(ERROR, "\t%s", *tmp);
			free(*tmp);
			tmp++;
		}
		free(unresolved);
		pkg_vec_free(deps);
		opkg_message(ERROR, "\n");
		return -1;
	}

	/* insert the package we are installing so that we download it */
	pkg_vec_insert(deps, new);

	/* download package and dependencies */
	for (i = 0; i < deps->len; i++) {
		pkg_t *pkg;
		struct _curl_cb_data cb_data;
		char *url;

		pkg = deps->pkgs[i];
		if (pkg->local_filename)
			continue;

		pdata.pkg = pkg;
		pdata.action = OPKG_DOWNLOAD;

		if (pkg->src == NULL) {
			opkg_msg(ERROR, "Package %s not available from any "
					"configured src\n", package_name);
			return -1;
		}

		sprintf_alloc(&url, "%s/%s", pkg->src->value, pkg->filename);

		/* Get the filename part, without any directory */
		stripped_filename = strrchr(pkg->filename, '/');
		if (!stripped_filename)
			stripped_filename = pkg->filename;

		sprintf_alloc(&pkg->local_filename, "%s/%s", conf->tmp_dir,
			      stripped_filename);

		cb_data.cb = progress_callback;
		cb_data.progress_data = &pdata;
		cb_data.user_data = user_data;
		/* 75% of "install" progress is for downloading */
		cb_data.start_range = 75 * i / deps->len;
		cb_data.finish_range = 75 * (i + 1) / deps->len;

		err = opkg_download(url, pkg->local_filename,
				    (curl_progress_func) curl_progress_cb,
				    &cb_data, 0);
		free(url);

		if (err) {
			pkg_vec_free(deps);
			return -1;
		}

	}
	pkg_vec_free(deps);

	/* clear depenacy checked marks, left by pkg_hash_fetch_unsatisfied_dependencies */
	all = pkg_vec_alloc();
	pkg_hash_fetch_available(all);
	for (i = 0; i < all->len; i++) {
		all->pkgs[i]->parent->dependencies_checked = 0;
	}
	pkg_vec_free(all);


	/* 75% of "install" progress is for downloading */
	pdata.pkg = new;
	pdata.action = OPKG_INSTALL;
	progress(pdata, 75);

	/* unpack the package */
	err = opkg_install_pkg(new, 0);

	if (err) {
		return -1;
	}

	progress(pdata, 75);

	/* run configure scripts, etc. */
	err = opkg_configure_packages(NULL);
	if (err) {
		return -1;
	}

	/* write out status files and file lists */
	opkg_conf_write_status_files();
	pkg_write_changed_filelists();

	progress(pdata, 100);
	return 0;
}

int
opkg_remove_package(const char *package_name,
		opkg_progress_callback_t progress_callback, void *user_data)
{
	int err;
	pkg_t *pkg = NULL;
	pkg_t *pkg_to_remove;
	opkg_progress_data_t pdata;

	opkg_assert(package_name != NULL);

	pkg_info_preinstall_check();

	pkg = pkg_hash_fetch_installed_by_name(package_name);

	if (pkg == NULL || pkg->state_status == SS_NOT_INSTALLED) {
		opkg_msg(ERROR, "Package %s not installed\n", package_name);
		return -1;
	}

	pdata.action = OPKG_REMOVE;
	pdata.pkg = pkg;
	progress(pdata, 0);

	if (conf->restrict_to_default_dest) {
		pkg_to_remove = pkg_hash_fetch_installed_by_name_dest(pkg->name,
						      conf->default_dest);
	} else {
		pkg_to_remove = pkg_hash_fetch_installed_by_name(pkg->name);
	}


	progress(pdata, 75);

	err = opkg_remove_pkg(pkg_to_remove, 0);

	/* write out status files and file lists */
	opkg_conf_write_status_files();
	pkg_write_changed_filelists();


	progress(pdata, 100);
	return (err) ? -1 : 0;
}

int
opkg_upgrade_package(const char *package_name,
		opkg_progress_callback_t progress_callback,
		void *user_data)
{
	int err;
	pkg_t *pkg;
	opkg_progress_data_t pdata;

	opkg_assert(package_name != NULL);

	pkg_info_preinstall_check();

	if (conf->restrict_to_default_dest) {
		pkg = pkg_hash_fetch_installed_by_name_dest(package_name,
							    conf->default_dest);
	} else {
		pkg = pkg_hash_fetch_installed_by_name(package_name);
	}

	if (!pkg) {
		opkg_msg(ERROR, "Package %s not installed\n", package_name);
		return -1;
	}

	pdata.action = OPKG_INSTALL;
	pdata.pkg = pkg;
	progress(pdata, 0);

	err = opkg_upgrade_pkg(pkg);
	if (err) {
		return -1;
	}
	progress(pdata, 75);

	err = opkg_configure_packages(NULL);
	if (err) {
		return -1;
	}

	/* write out status files and file lists */
	opkg_conf_write_status_files();
	pkg_write_changed_filelists();

	progress(pdata, 100);
	return 0;
}

int
opkg_upgrade_all(opkg_progress_callback_t progress_callback, void *user_data)
{
	pkg_vec_t *installed;
	int err = 0;
	int i;
	pkg_t *pkg;
	opkg_progress_data_t pdata;

	pdata.action = OPKG_INSTALL;
	pdata.pkg = NULL;

	progress(pdata, 0);

	installed = pkg_vec_alloc();
	pkg_info_preinstall_check();

	pkg_hash_fetch_all_installed(installed);
	for (i = 0; i < installed->len; i++) {
		pkg = installed->pkgs[i];

		pdata.pkg = pkg;
		progress(pdata, 99 * i / installed->len);

		err += opkg_upgrade_pkg(pkg);
	}
	pkg_vec_free(installed);

	if (err)
		return 1;

	err = opkg_configure_packages(NULL);
	if (err)
		return 1;

	/* write out status files and file lists */
	opkg_conf_write_status_files();
	pkg_write_changed_filelists();

	pdata.pkg = NULL;
	progress(pdata, 100);
	return 0;
}

int
opkg_update_package_lists(opkg_progress_callback_t progress_callback,
			void *user_data)
{
	char *tmp;
	int err, result = 0;
	char *lists_dir;
	pkg_src_list_elt_t *iter;
	pkg_src_t *src;
	int sources_list_count, sources_done;
	opkg_progress_data_t pdata;

	pdata.action = OPKG_DOWNLOAD;
	pdata.pkg = NULL;
	progress(pdata, 0);

	sprintf_alloc(&lists_dir, "%s", (conf->restrict_to_default_dest)
		? conf->default_dest->lists_dir : conf->lists_dir);

	if (!file_is_dir(lists_dir)) {
		if (file_exists(lists_dir)) {
			opkg_msg(ERROR, "%s is not a directory\n", lists_dir);
			free(lists_dir);
			return 1;
		}

		err = file_mkdir_hier(lists_dir, 0755);
		if (err) {
			opkg_msg(ERROR, "Couldn't create lists_dir %s\n",
					lists_dir);
			free(lists_dir);
			return 1;
		}
	}

	sprintf_alloc(&tmp, "%s/update-XXXXXX", conf->tmp_dir);
	if (mkdtemp(tmp) == NULL) {
		opkg_perror(ERROR, "Coundn't create temporary directory %s",
				tmp);
		free(lists_dir);
		free(tmp);
		return 1;
	}

	/* count the number of sources so we can give some progress updates */
	sources_list_count = 0;
	sources_done = 0;
	list_for_each_entry(iter, &conf->pkg_src_list.head, node) {
		sources_list_count++;
	}

	list_for_each_entry(iter, &conf->pkg_src_list.head, node) {
		char *url, *list_file_name = NULL;

		src = (pkg_src_t *) iter->data;

		if (src->extra_data)	/* debian style? */
			sprintf_alloc(&url, "%s/%s/%s", src->value,
				      src->extra_data,
				      src->gzip ? "Packages.gz" : "Packages");
		else
			sprintf_alloc(&url, "%s/%s", src->value,
				      src->gzip ? "Packages.gz" : "Packages");

		sprintf_alloc(&list_file_name, "%s/%s", lists_dir, src->name);
		if (src->gzip) {
			FILE *in, *out;
			struct _curl_cb_data cb_data;
			char *tmp_file_name = NULL;

			sprintf_alloc(&tmp_file_name, "%s/%s.gz", tmp,
				      src->name);

			opkg_msg(INFO, "Downloading %s to %s...\n", url,
					tmp_file_name);

			cb_data.cb = progress_callback;
			cb_data.progress_data = &pdata;
			cb_data.user_data = user_data;
			cb_data.start_range =
			    100 * sources_done / sources_list_count;
			cb_data.finish_range =
			    100 * (sources_done + 1) / sources_list_count;

			err = opkg_download(url, tmp_file_name,
					  (curl_progress_func) curl_progress_cb,
					  &cb_data, 0);

			if (err == 0) {
				opkg_msg(INFO, "Inflating %s...\n",
						tmp_file_name);
				in = fopen(tmp_file_name, "r");
				out = fopen(list_file_name, "w");
				if (in && out)
					unzip(in, out);
				else
					err = 1;
				if (in)
					fclose(in);
				if (out)
					fclose(out);
				unlink(tmp_file_name);
			}
			free(tmp_file_name);
		} else
			err = opkg_download(url, list_file_name, NULL, NULL, 0);

		if (err) {
			opkg_msg(ERROR, "Couldn't retrieve %s\n", url);
			result = -1;
		}
		free(url);

#if defined(HAVE_GPGME) || defined(HAVE_OPENSSL)
		if (conf->check_signature) {
			char *sig_file_name;
			/* download detached signitures to verify the package lists */
			/* get the url for the sig file */
			if (src->extra_data)	/* debian style? */
				sprintf_alloc(&url, "%s/%s/%s", src->value,
					      src->extra_data, "Packages.sig");
			else
				sprintf_alloc(&url, "%s/%s", src->value,
					      "Packages.sig");

			/* create filename for signature */
			sprintf_alloc(&sig_file_name, "%s/%s.sig", lists_dir,
				      src->name);

			/* make sure there is no existing signature file */
			unlink(sig_file_name);

			err = opkg_download(url, sig_file_name, NULL, NULL, 0);
			if (err) {
				opkg_msg(ERROR, "Couldn't retrieve %s\n", url);
			} else {
				int err;
				err = opkg_verify_file(list_file_name,
						     sig_file_name);
				if (err == 0) {
					opkg_msg(INFO, "Signature check "
							"passed for %s",
							list_file_name);
				} else {
					opkg_msg(ERROR, "Signature check "
							"failed for %s",
							list_file_name);
				}
			}
			free(sig_file_name);
			free(url);
		}
#else
		opkg_msg(INFO, "Signature check skipped for %s as GPG support"
				" has not been enabled in this build\n",
				list_file_name);
#endif
		free(list_file_name);

		sources_done++;
		progress(pdata, 100 * sources_done / sources_list_count);
	}

	rmdir(tmp);
	free(tmp);
	free(lists_dir);

	/* Now re-read the package lists to update package hash tables. */
	opkg_re_read_config_files();

	return result;
}

static int
pkg_compare_names_and_version(const void *a0, const void *b0)
{
	const pkg_t *a = *(const pkg_t **)a0;
	const pkg_t *b = *(const pkg_t **)b0;
	int ret;

	ret = strcmp(a->name, b->name);

	if (ret == 0)
		ret = pkg_compare_versions(a, b);

	return ret;
}

int
opkg_list_packages(opkg_package_callback_t callback, void *user_data)
{
	pkg_vec_t *all;
	int i;

	opkg_assert(callback);

	all = pkg_vec_alloc();
	pkg_hash_fetch_available(all);

	pkg_vec_sort(all, pkg_compare_names_and_version);

	for (i = 0; i < all->len; i++) {
		pkg_t *pkg;

		pkg = all->pkgs[i];

		callback(pkg, user_data);
	}

	pkg_vec_free(all);

	return 0;
}

int
opkg_list_upgradable_packages(opkg_package_callback_t callback, void *user_data)
{
	struct active_list *head;
	struct active_list *node;
	pkg_t *old = NULL, *new = NULL;

	opkg_assert(callback);

	/* ensure all data is valid */
	pkg_info_preinstall_check();

	head = prepare_upgrade_list();
	for (node = active_list_next(head, head); node;
	     node = active_list_next(head, node)) {
		old = list_entry(node, pkg_t, list);
		new = pkg_hash_fetch_best_installation_candidate_by_name(old->name);
		if (new == NULL)
			continue;
		callback(new, user_data);
	}
	active_list_head_delete(head);
	return 0;
}

pkg_t *
opkg_find_package(const char *name, const char *ver, const char *arch,
		const char *repo)
{
	int pkg_found = 0;
	pkg_t *pkg = NULL;
	pkg_vec_t *all;
	int i;
#define sstrcmp(x,y) (x && y) ? strcmp (x, y) : 0

	all = pkg_vec_alloc();
	pkg_hash_fetch_available(all);
	for (i = 0; i < all->len; i++) {
		char *pkgv;

		pkg = all->pkgs[i];

		/* check name */
		if (sstrcmp(pkg->name, name))
			continue;

		/* check version */
		pkgv = pkg_version_str_alloc(pkg);
		if (sstrcmp(pkgv, ver)) {
			free(pkgv);
			continue;
		}
		free(pkgv);

		/* check architecture */
		if (arch) {
			if (sstrcmp(pkg->architecture, arch))
				continue;
		}

		/* check repository */
		if (repo) {
			if (sstrcmp(pkg->src->name, repo))
				continue;
		}

		/* match found */
		pkg_found = 1;
		break;
	}

	pkg_vec_free(all);

	return pkg_found ? pkg : NULL;
}

/**
 * @brief Check the accessibility of repositories.
 * @return return how many repositories cannot access. 0 means all okay.
 */
int
opkg_repository_accessibility_check(void)
{
	pkg_src_list_elt_t *iter;
	str_list_elt_t *iter1;
	str_list_t *src;
	int repositories = 0;
	int ret = 0;
	char *repo_ptr;
	char *stmp;
	char *host, *end;

	src = str_list_alloc();

	list_for_each_entry(iter, &conf->pkg_src_list.head, node) {
		host = strstr(((pkg_src_t *)iter->data)->value, "://") + 3;
		end = index(host, '/');
		if (strstr(((pkg_src_t *) iter->data)->value, "://") && end)
			stmp = xstrndup(((pkg_src_t *) iter->data)->value,
				     end - ((pkg_src_t *) iter->data)->value);
		else
			stmp = xstrdup(((pkg_src_t *) iter->data)->value);

		for (iter1 = str_list_first(src); iter1;
		     iter1 = str_list_next(src, iter1)) {
			if (strstr(iter1->data, stmp))
				break;
		}
		if (iter1)
			continue;

		sprintf_alloc(&repo_ptr, "%s/index.html", stmp);
		free(stmp);

		str_list_append(src, repo_ptr);
		free(repo_ptr);
		repositories++;
	}

	while (repositories > 0) {
		iter1 = str_list_pop(src);
		repositories--;

		if (opkg_download(iter1->data, "/dev/null", NULL, NULL, 0))
			ret++;
		str_list_elt_deinit(iter1);
	}

	free(src);

	return ret;
}
