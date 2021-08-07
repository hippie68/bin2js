/* Fast .bin to .js converter for use with sleirsgoevy's PS4 jailbreaks.
 * Please report bugs at https://github.com/hippie68/bin2js/issues. */

#if defined(_WIN32) || defined(_WIN64)
#include "include/dirent.h"
#define DIR_SEPARATOR '\\'
#else
#include <dirent.h>
#define DIR_SEPARATOR '/'
#endif

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int option_c = 0, option_r = 0;
char *outputfile = NULL;

void print_usage(char *program_name) {
  char *basename = strrchr(program_name, DIR_SEPARATOR);
  if (basename != NULL && strlen(basename) > 1) {
    basename++;
  } else {
    basename = program_name;
  }
  fprintf(stderr,
    "Usage: %s [-cr] file1/directory1 [file2/directory2 ...]\n"
    "   Or: %s [-c] file1 -o file2\n\n"
    "  Converts PS4 binary payload files to JavaScript payload files, following\n"
    "  the pattern \"filename.bin\" => \"filename.js\" unless an output file is\n"
    "  explicitly specified (option -o). Existing output files will be overwritten.\n\n"
    "  Options:\n"
    "    -c  Compress JavaScript code further, to one single line\n"
    "    -h  Display this help info\n"
    "    -o  Write to the specified output file only\n"
    "    -r  Recursively traverse subdirectories\n"
    "    --  All arguments following this option are treated as non-options\n"
    ,basename, basename);
}

// Deletes a command line argument
void delete_argument(int *argc, char *argv[], int index) {
  for (int i = index; i < *argc; i++) {
    argv[i] = argv[i + 1];
  }
  *argc = *argc - 1;
}

// Parses command line options
void get_options(int *argc, char *argv[]) {
  if (*argc > 1) {
    for (int i = 1; i < *argc; i++) {

      // Stop parsing when encountering option "--"
      if (strcmp(argv[i], "--") == 0) {
        delete_argument(argc, argv, i);
        break;
      }

      if (argv[i][0] == '-') {
        for (int o = 1; argv[i][o] != '\0'; o++) {
          switch (argv[i][o]) {
            case 'c':
              option_c = 1;
              break;
            case 'h':
              print_usage(argv[0]);
              exit(0);
              break;
            case 'o':
              if (i < *argc) {
                outputfile = argv[i + 1];
                delete_argument(argc, argv, i + 1);
              }
              break;
            case 'r':
              option_r = 1;
              break;
            default:
              fprintf(stderr, "Unknown option: \"%c\"\n", argv[i][o]);
              exit(1);
          }
        }
        delete_argument(argc, argv, i);
        i--;
      }
    }
  } else {
    print_usage(argv[0]);
    exit(1);
  }
}

// Replaces a filename string's ".bin" extension with ".js"
char *js_extension(char *bin_filename) {
  static char js_filename[4096];
  if (strcmp(strrchr(bin_filename, '.'), ".bin") == 0) {
    strcpy(js_filename, bin_filename);
    int len = strlen(bin_filename);
    js_filename[len - 3] = 'j';
    js_filename[len - 2] = 's';
    js_filename[len - 1] = '\0';
  } else {
    snprintf(js_filename, 4096, "%s%s", bin_filename, ".js");
  }
  if (strcmp(bin_filename, js_filename) != 0)
    return js_filename;
  else
    return NULL;
}

// Converts a binary payload file to a JavaScript payload file
int convert_bin_to_js(char *bin_filename, char *js_filename) {
  int byte;
  long int payload_size;

  // Open files
  FILE *bin_file, *js_file;
  bin_file = fopen(bin_filename, "rb");
  if (bin_file == NULL) {
    fprintf(stderr, "Could not read file \"%s\".\n", bin_filename);
    return 1;
  }
  js_file = fopen(js_filename, "w");
  if (js_file == NULL) {
    fprintf(stderr, "Could not create file \"%s\".\n", js_filename);
    return 1;
  }

  // Get payload size
  fseek(bin_file, 0, SEEK_END);
  payload_size = ftell(bin_file);
  fseek(bin_file, 0, SEEK_SET);
  if (payload_size == 0) {
    fprintf(stderr, "Size of file \"%s\" is zero; nothing to do.\n",
      bin_filename);
    return 1;
  }

  // Write compressed JavaScript code
  if (option_c) {
    fprintf(js_file, "window.mira_blob_2_len=%ld;", payload_size);
    fprintf(js_file, "window.mira_blob_2=malloc(window.mira_blob_2_len);");
    fprintf(js_file, "write_mem(window.mira_blob_2,[");
  // Write regular JavaScript code
  } else {
    fprintf(js_file, "window.mira_blob_2_len = %ld;\n", payload_size);
    fprintf(js_file, "window.mira_blob_2 = malloc(window.mira_blob_2_len);\n");
    fprintf(js_file, "write_mem(window.mira_blob_2, [");
  }

  // First payload byte
  byte = getc(bin_file); // Read byte
  fprintf(js_file, "%d", byte); // Write byte

  // Remaining payload bytes
  for (long int i = 1; i < payload_size; i++) {
    // Read byte
    byte = getc(bin_file);
    if (byte == EOF) {
      fprintf(stderr,
        "EOF/error while reading from file \"%s\".\n",
        bin_filename);
      exit(1);
    }
    // Write byte
    if (fprintf(js_file, ",%d", byte) < 0) {
      fprintf(stderr,
        "Error while writing to file \"%s\".\n",
        js_filename);
      exit(1);
    }
  }

  // Write final JavaScript code characters
  fprintf(js_file, "]);\n");

  fclose(bin_file);
  return fclose(js_file);
}

// Finds all .bin files in the specified directory and calls
// convert_bin_to_js() to batch-convert the files to .js files
int convert_bin_to_js_batch(char *directory_name) {
  DIR *dir;
  struct dirent *directory_entry;
  static int error_count = 0;

  // Remove a possibly trailing directory separator
  {
    int len = strlen(directory_name);
    if (len > 1 && directory_name[len - 1] == DIR_SEPARATOR) {
      directory_name[len - 1] = '\0';
    }
  }

  dir = opendir(directory_name);
  if (!dir) return 0;

  while ((directory_entry = readdir(dir)) != NULL) {
    // Entry is directory
    if (directory_entry->d_type == DT_DIR) {

      // Option -r: recursively traverse directories
      if (option_r
        && strcmp(directory_entry->d_name, "..") // Ignore ".."
        && strcmp(directory_entry->d_name, ".")) // and "." directories
      {
        char nextdir[4096];
        snprintf(nextdir, 4096, "%s%c%s", directory_name, DIR_SEPARATOR,
          directory_entry->d_name);
        convert_bin_to_js_batch(nextdir);
      }

    // Entry is file
    } else {
      char *ext = strrchr(directory_entry->d_name, '.');
      if (ext != NULL && strcasecmp(ext, ".bin") == 0) {
        char bin_filename[4096];
        snprintf(bin_filename, 4096, "%s%c%s", directory_name, DIR_SEPARATOR,
          directory_entry->d_name);
        char *js_filename = js_extension(bin_filename);
        printf("%s\n", js_filename);
        error_count += convert_bin_to_js(bin_filename, js_filename);
      }
    }
  }

  return error_count;
}

// Returns number of failed conversions
int main(int argc, char *argv[]) {
  get_options(&argc, argv);

  // If no argument left after get_options, print usage and abort
  if (argc < 2) {
    print_usage(argv[0]);
    return 1;
  }

  // Single file mode with output file specified (option -o)
  if (outputfile != NULL) {
    if (opendir(argv[1])) {
      fprintf(stderr, "Not a file: \"%s\"\n", argv[1]);
      print_usage(argv[0]);
      return 1;
    } else if (argc > 2) {
      fprintf(stderr, "Too many arguments while using option \"-o\".\n");
      print_usage(argv[0]);
      return 1;
    } else {
      return convert_bin_to_js(argv[1], outputfile);
    }
  }

  // Batch mode
  if (outputfile == NULL) {
    int error_count = 0;
    for (int i = 1; i < argc; i++) {
      // Directory
      if (opendir(argv[i])) {
        error_count += convert_bin_to_js_batch(argv[i]);
      // File
      } else {
        error_count += convert_bin_to_js(argv[i], js_extension(argv[i]));
      }
    }
    return error_count;
  }
}
