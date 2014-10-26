#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#include <opkg.h>

int opkg_state_changed;
pkg_t *find_pkg = NULL;


#define TEST_PACKAGE "aspell"

void
progress_callback (const opkg_progress_data_t *progress, void *data)
{
  printf ("\r%s %3d%%\n", (char*) data, progress->percentage);
  fflush (stdout);
}

static void list_pkg(pkg_t *pkg)
{
  char *v = pkg_version_str_alloc(pkg);
  printf ("%s - %s\n", pkg->name, v);
  free(v);
}

void
package_list_installed_callback (pkg_t *pkg, void *data)
{
  if (pkg->state_status == SS_INSTALLED)
    list_pkg(pkg);
}

void
package_list_callback (pkg_t *pkg, void *data)
{
  static int install_count = 0;
  static int total_count = 0;

  if (pkg->state_status == SS_INSTALLED)
    install_count++;

  total_count++;

  printf ("\rPackage count: %d Installed, %d Total Available", install_count, total_count);
  fflush (stdout);

  if (!find_pkg)
  {
    /* store the first package to print out later */
    find_pkg = pkg;
  }
}

void
package_list_upgradable_callback (pkg_t *pkg, void *data)
{
  list_pkg(pkg);
}

void
print_package (pkg_t *pkg)
{
  char *v = pkg_version_str_alloc(pkg);
  printf (
      "Name:         %s\n"
      "Version:      %s\n"
      "Repository:   %s\n"
      "Architecture: %s\n"
      "Description:  %s\n"
      "Tags:         %s\n"
      "Size:         %ld\n"
      "Status:       %d\n",
      pkg->name,
      v,
      pkg->src->name,
      pkg->architecture,
      pkg->description,
      pkg->tags? pkg->tags : "",
      pkg->size,
      pkg->state_status);
  free(v);
}


void
opkg_test (void)
{
  int err;
  pkg_t *pkg;

  err = opkg_update_package_lists (progress_callback, "Updating...");
  printf ("\nopkg_update_package_lists returned %d\n", err);

  opkg_list_packages (package_list_callback, NULL);
  printf ("\n");

  if (find_pkg)
  {
    printf ("Finding package \"%s\"\n", find_pkg->name);
    pkg = opkg_find_package (find_pkg->name, find_pkg->version, find_pkg->architecture, find_pkg->src->name);
    if (pkg)
    {
      print_package (pkg);
    }
    else
      printf ("Package \"%s\" not found!\n", find_pkg->name);
  }
  else
    printf ("No package available to test find_package.\n");

  err = opkg_install_package (TEST_PACKAGE, progress_callback, "Installing...");
  printf ("\nopkg_install_package returned %d\n", err);

  err = opkg_upgrade_package (TEST_PACKAGE, progress_callback, "Upgrading...");
  printf ("\nopkg_upgrade_package returned %d\n", err);

  err = opkg_remove_package (TEST_PACKAGE, progress_callback, "Removing...");
  printf ("\nopkg_remove_package returned %d\n", err);

  printf ("Listing upgradable packages...\n");
  opkg_list_upgradable_packages (package_list_upgradable_callback, NULL);

  err = opkg_upgrade_all (progress_callback, "Upgrading all...");
  printf ("\nopkg_upgrade_all returned %d\n", err);

}

int
main (int argc, char **argv)
{
  pkg_t *pkg;
  int err;

  if (argc < 2)
  {
    printf ("Usage: %s command\n"
            "\nCommands:\n"
	    "\tupdate - Update package lists\n"
	    "\tfind [package] - Print details of the specified package\n"
	    "\tinstall [package] - Install the specified package\n"
	    "\tupgrade [package] - Upgrade the specified package\n"
	    "\tlist upgrades - List the available upgrades\n"
	    "\tlist all - List all available packages\n"
	    "\tlist installed - List all the installed packages\n"
	    "\tremove [package] - Remove the specified package\n"
	    "\trping - Reposiroties ping, check the accessibility of repositories\n"
	    "\ttest - Run test script\n"
    , basename (argv[0]));
    exit (0);
  }

  setenv("OFFLINE_ROOT", "/tmp", 0);

  if (opkg_new ()) {
	  printf("opkg_new() failed. This sucks.\n");
	  print_error_list();
	  return 1;
  }

  switch (argv[1][0])
  {
    case 'f':
      pkg = opkg_find_package (argv[2], NULL, NULL, NULL);
      if (pkg)
      {
	print_package (pkg);
      }
      else
	printf ("Package \"%s\" not found!\n", find_pkg->name);
      break;
    case 'i':
      err = opkg_install_package (argv[2], progress_callback, "Installing...");
      printf ("\nopkg_install_package returned %d\n", err);
      break;

    case 'u':
      if (argv[1][2] == 'd')
      {
        err = opkg_update_package_lists (progress_callback, "Updating...");
        printf ("\nopkg_update_package_lists returned %d\n", err);
        break;
      }
      else
      {
        if (argc < 3)
        {
          err = opkg_upgrade_all (progress_callback, "Upgrading all...");
          printf ("\nopkg_upgrade_all returned %d\n", err);
        }
        else
        {
          err = opkg_upgrade_package (argv[2], progress_callback, "Upgrading...");
          printf ("\nopkg_upgrade_package returned %d\n", err);
        }
      }
      break;

    case 'l':
      if (argc < 3)
      {
        printf ("Please specify one either all, installed or upgrades\n");
      }
      else
      {
        switch (argv[2][0])
        {
          case 'u':
            printf ("Listing upgradable packages...\n");
            opkg_list_upgradable_packages (package_list_upgradable_callback, NULL);
            break;
          case 'a':
            printf ("Listing all packages...\n");
            opkg_list_packages (package_list_callback, NULL);
            printf ("\n");
            break;
          case 'i':
            printf ("Listing installed packages...\n");
            opkg_list_packages (package_list_installed_callback, NULL);
            break;
          default:
            printf ("Unknown list option \"%s\"\n", argv[2]);
        }
      }
      break;

    case 'r':
      if (argv[1][1] == 'e')
      {
      	err = opkg_remove_package (argv[2], progress_callback, "Removing...");
      	printf ("\nopkg_remove_package returned %d\n", err);
	break;
      }else if (argv[1][1] == 'p')
      {
        err = opkg_repository_accessibility_check();
	printf("\nopkg_repository_accessibility_check returned (%d)\n", err);
        break;
      }

    default:
      printf ("Unknown command \"%s\"\n", argv[1]);
  }


  opkg_free ();

  return 0;
}
