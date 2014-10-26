#include <stdio.h>
#include <stdlib.h>
#include <libbb/libbb.h>

/*
 * build thus:

 * gcc -o opkg_extract_test opkg_extract_test.c -I./busybox-0.60.2/libbb -L./busybox-0.60.2 -lbb
 *
 */
const char * applet_name;

int main(int argc, char * argv[])
{
  /*
   * see libbb.h and let your imagination run wild
   * or, set the last item below to extract_one_to_buffer, and you get the control file in
   * "returned"
   * or, set the last one to extract_all_to_fs, and, well, guess what happens
   */

    /* enum extract_functions_e dowhat = extract_control_tar_gz | extract_unconditional | extract_one_to_buffer; */
    enum extract_functions_e dowhat = extract_control_tar_gz | extract_all_to_fs | extract_preserve_date;
  char * returned;
  char * filename;

  if(argc < 2){
    fprintf(stderr, "syntax: %s <opkg file> [<file_to_extract>]\n", argv[0]);
    exit(0);
  }

  if (argc < 3){
    filename=NULL;
  } else {
    filename = argv[2];
  }

  returned = deb_extract(argv[1], stdout, dowhat, NULL, filename);

  if(returned)
    fprintf(stderr, "returned %s\n", returned);
  else
    fprintf(stderr, "extract returned nuthin'\n");

  return 0;
}
