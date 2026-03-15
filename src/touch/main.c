/**
 * @file main.c
 * @brief File creation and timestamp modification utility
 *
 * A cross-platform command-line utility inspired by the Unix 'touch' command.
 * Creates new files and modifies access/modification timestamps on existing files.
 * Designed specifically for Windows systems with support for various timestamp
 * modification options.
 *
 * @author monakaibrahim-cmyk
 * @version 1.0.0
 *
 * @usage touch [OPTIONS] FILE
 *
 * @example
 * @code
 * touch file.txt              // Create file or update timestamp
 * touch -a file.txt          // Change only access time
 * touch -m file.txt          // Change only modification time
 * touch -c file.txt         // Do not create if file doesn't exist
 * touch -r ref.txt file.txt  // Use timestamps from reference file
 * touch -t 202403151200 file.txt  // Use specific timestamp
 * @endcode
 */

 #define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <direct.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

/**
 * @defgroup ColorCodes ANSI Color Codes
 * @brief Terminal escape sequences for colored output
 *
 * These macros define ANSI escape sequences for formatting terminal
 * output with colors and text styling.
 *
 * @{
 */
#define RESET   "\x1B[0m"   /**< Reset all formatting */
#define BLACK   "\x1B[30m"  /**< Black text color */
#define RED     "\x1B[31m"  /**< Red text color */
#define GREEN   "\x1B[32m"  /**< Green text color */
#define YELLOW  "\x1B[33m"  /**< Yellow text color */
#define BLUE    "\x1B[34m"  /**< Blue text color */
#define MAGENTA "\x1B[35m"  /**< Magenta text color */
#define CYAN    "\x1B[36m"  /**< Cyan text color */
#define WHITE   "\x1B[37m"  /**< White text color */
/** @} */

/**
 * @brief Command-line options structure
 *
 * This structure holds all configurable options that can be passed
 * via the command line to modify the behavior of the touch utility.
 */
typedef struct
{
    int _atime_only;      /**< Modify access time only */
    int _mtime_only;      /**< Modify modification time only */
    int _no_create;       /**< Do not create file if it doesn't exist */
    int _no_deref;        /**< Do not dereference symbolic links */
    char* _date;          /**< Date string for timestamp */
    char* _ref_file;      /**< Reference file to copy timestamps from */
    char* _timestamp;     /**< Explicit timestamp string */
} Options;

/**
 * @brief Creates directory structure recursively
 *
 * This function traverses the given path and creates each directory
 * component if it does not already exist. It handles both forward
 * slashes (/) and backslashes (\) as path separators, normalizing
 * all separators to backslashes for Windows compatibility.
 *
 * The function works by iterating through each character in the path,
 * treating each backslash as a potential directory boundary. When a
 * boundary is found, it attempts to create the directory up to that
 * point. Errors are ignored if the directory already exists (EEXIST).
 *
 * @param[in] path The path containing directory components to create
 *
 * @return int Returns 0 on success
 * @return int Returns -1 on failure, with errno set appropriately
 *
 * @warning The path buffer is modified during execution
 *
 * @example
 * @code
 * recursive_mkdir("a\\b\\c");  // Creates a, a\b, and a\b\c
 * @endcode
 */
int recursive_mkdir(const char* path)
{
    char temp[MAX_PATH];
    char* p = NULL;
    size_t length;

    /* Copy the input path to a temporary buffer to avoid modifying the original */
    strncpy(temp, path, MAX_PATH);
    length = strlen(temp);
    
    /* Normalize all path separators to backslashes for Windows */
    for (p = temp; *p; p++)
    {
        if (*p == '/' || *p == '\\')
        {
            *p = '\\';
        }
    }

    /* Walk through the path and create each directory component */
    for (p = temp + 1; *p; p++)
    {
        if (*p == '\\')
        {
            /* Temporarily terminate the string at this position */
            *p = '\0';

            /* Attempt to create the directory */
            if (_mkdir(temp) != 0 && errno != EEXIST)
            {
                /* Restore the separator and return error */
                *p = '\\';
                return -1;
            }

            /* Restore the separator and continue */
            *p = '\\';
        }
    }

    /* Create the final directory in the path */
    if (_mkdir(temp) != 0 && errno != EEXIST)
    {
        return -1;
    }

    return 0;
}

/**
 * @brief Determines if a path is absolute
 *
 * This function checks whether the given path represents an absolute
 * path on Windows systems. An absolute path is identified by:
 * - A drive letter followed by a colon (e.g., "C:")
 * - A UNC path starting with double backslash (e.g., "\\server\share")
 *
 * @param[in] path The path string to check
 *
 * @return int Returns 1 if the path is absolute
 * @return int Returns 0 if the path is relative
 *
 * @note Returns 1 for NULL or empty strings as a safety measure
 */
int _absolute(const char* path)
{
    /* Handle NULL or too-short strings as absolute for safety */
    if (path == NULL || strlen(path) < 2)
    {
        return 1;
    }

    /* Check for Windows drive letter (e.g., C:) */
    if (isalpha(path[0]) && path[1] == ':')
    {
        return 1;
    }

    /* Check for UNC path (e.g., \\server\share) */
    if (path[0] == '\\' && path[1] == '\\')
    {
        return 1;
    }

    return 0;
}

/**
 * @brief Sets file timestamps
 *
 * This function modifies the access and/or modification timestamps
 * of an open file handle. The timestamps can be set from:
 * - Current system time
 * - A reference file's timestamps
 * - An explicit timestamp string in the format YYYYMMDDhhmm
 *
 * @param[in] hFile    Handle to the file whose timestamps to modify
 * @param[in] options  Pointer to Options structure containing timestamp settings
 *
 * @note If neither _atime_only nor _mtime_only is set, both timestamps
 *       will be modified
 */
void _timestamps(HANDLE hFile, Options* options)
{
    FILETIME access, write;
    SYSTEMTIME system_time;

    /* Determine the source of timestamps based on options */
    if (options->_ref_file)
    {
        /* Open the reference file to copy its timestamps */
        HANDLE hRef = CreateFile(
            options->_ref_file,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

        if (hRef != INVALID_HANDLE_VALUE)
        {
            /* Read timestamps from the reference file */
            GetFileTime(hRef, NULL, &access, &write);
            CloseHandle(hRef);
        }
    }
    else if (options->_timestamp)
    {
        /* Parse the explicit timestamp string (format: YYYYMMDDhhmm) */
        sscanf(
            options->_timestamp,
            "%04hd%02hd%02hd%02hd%02hd",
            &system_time.wYear,
            &system_time.wMonth,
            &system_time.wDay,
            &system_time.wHour,
            &system_time.wMinute
        );
        /* Convert system time to file time format */
        SystemTimeToFileTime(&system_time, &access);
        write = access;
    }
    else
    {
        /* Use current system time for both timestamps */
        GetSystemTimeAsFileTime(&access);
        write = access;
    }

    /* Apply timestamps based on options (atime_only, mtime_only, or both) */
    FILETIME *pAccess = options->_mtime_only ? NULL : &access;
    FILETIME *pWrite  = options->_atime_only ? NULL : &write;

    /* Set the file timestamps */
    SetFileTime(hFile, NULL, pAccess, pWrite);
}

/**
 * @brief Displays help information
 *
 * This function prints usage information and a list of all
 * supported command-line options to standard output.
 *
 * @note The function exits after printing help information
 */
void _help()
{
    printf("Usage: %stouch%s [%sOPTION%s] %sFile%s\n", GREEN, RESET, YELLOW, RESET, YELLOW, RESET);
    printf("%s%-10s%s : %s\n", YELLOW, "-a", RESET, "Change only the access time (atime).");
    printf("%s%-10s%s : %s\n", YELLOW, "-m", RESET, "Change only the modification time (mtime).");
    printf("%s%-10s%s : %s\n", YELLOW, "-c", RESET, "Do not create a new file if it does not already exist.");
    printf("%s%-10s%s : %s\n", YELLOW, "-h", RESET, "Affect each symbolic link instead of any referenced file.");
    printf("%s%-10s%s : %s\n", YELLOW, "-r FILE", RESET, "Use the specified FILE's timestamps instead of the current time.");
    printf("%s%-10s%s : %s\n", YELLOW, "-t STAMP", RESET, "Use a specific time STAMP in the format [[CC]YY]MMDDhhmm[.ss].");
    printf("%s%-10s%s : %s\n", YELLOW, "--help", RESET, "Display a help message and exit.");
    printf("%s%-10s%s : %s\n", YELLOW, "--version", RESET, "Output version information and exit.");
}

/**
 * @brief Displays version information
 *
 * This function prints the program name, version number,
 * and copyright information to standard output.
 *
 * @note The function exits after printing version information
 */
void _version()
{
    time_t now = time(NULL);
    struct tm* t = localtime(&now);

    int current = t->tm_year + 1900;
    int last = t->tm_year + 1900 - 1;

    const char* version = "1.0";

    printf("%stouch%s v%s\n", GREEN, RESET, version);
    printf("Copyright %sGNU General Public License (GPL)%s %d-%d\n", GREEN, RESET, last, current);
}

/**
 * @brief Main entry point for the touch command
 *
 * This function serves as the entry point for the file creation
 * and timestamp modification utility. It processes command-line
 * arguments, handles various options, and creates/modifies files
 * as specified.
 *
 * The function performs the following operations:
 * - Parses and validates command-line arguments
 * - Handles path normalization (converts forward slashes to backslashes)
 * - Creates parent directory structure if needed
 * - Creates or opens the target file
 * - Sets file timestamps according to options
 *
 * @param[in] argc Number of command-line arguments
 * @param[in] argv Array of command-line argument strings
 *                - argv[0]: Program name
 *                - argv[1..n]: Options and target file path
 *
 * @return int Returns 0 on successful execution
 * @return int Returns 1 on error (usage error, file operation failure,
 *             or memory allocation failure)
 *
 * @example
 * @code
 * // Command line: touch myfile.txt
 * // Creates myfile.txt in current directory
 * //
 * // Command line: touch -c -r reference.txt target.txt
 * // Updates target.txt timestamps from reference.txt without creating
 * @endcode
 */
int main(int argc, char** argv)
{
    int status = 0;
    HANDLE hFile = NULL;
    
    /* Allocate memory for options structure */
    Options* options = malloc(sizeof(Options));

    /* Check if memory allocation succeeded */
    if (options == NULL)
    {
        status = 1;
        goto FAILED_MEMORY_ALLOCATION;
    }

    /* Initialize all options to default values (zeros) */
    memset(options, 0, sizeof(Options));

    char* target = NULL;

    /* Parse command-line arguments to identify options and target file */
    for (int i = 1; i < argc; i++)
    {
        /* -a: Change only the access time */
        if (strcmp(argv[i], "-a") == 0)
        {
            options->_atime_only = 1;
        }
        /* -m: Change only the modification time */
        else if (strcmp(argv[i], "-m") == 0)
        {
            options->_mtime_only = 1;
        }
        /* -c/--no-create: Do not create file if it doesn't exist */
        else if ((strcmp(argv[i], "-c") == 0) || (strcmp(argv[i], "--no-create") == 0))
        {
            options->_no_create = 1;
        }
        /* -d/--date: Specify date string for timestamp */
        else if (strcmp(argv[i], "-d") == 0 || strncmp(argv[i], "--date=", 7) == 0)
        {
			char *value = (argv[i][0] == '-') ? (i + 1 < argc ? argv[++i] : NULL) : (argv[i] + 7);

			if (value == NULL)
            {
				printf("%stouch%s: option '%s-d%s|%s--date=%s' requires an argument.", GREEN, RESET, YELLOW, RESET, YELLOW, RESET);
				status = 1;
				goto CLEANUP;
			}

			/* Free existing date string if present */
			if (options->_date != NULL)
            {
                free(options->_date);
            }

			/* Allocate and copy the date string */
			if ((options->_date = malloc(strlen(value) + 1)) == NULL)
            {
                status = 1;
                goto FAILED_MEMORY_ALLOCATION;
            }

			strcpy(options->_date, value);
		}
        /* -h/--no-dereference: Affect symbolic link instead of target */
        else if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--no-dereference") == 0))
        {
            options->_no_deref = 1;
        }
        /* -r/--reference: Use timestamps from reference file */
        else if (strcmp(argv[i], "-r") == 0 || strncmp(argv[i], "--reference=", 12) == 0)
        {
			char *value = (argv[i][0] == '-') ? (i + 1 < argc ? argv[++i] : NULL) : (argv[i] + 12);

			if (value == NULL)
            {
                printf("%stouch%s: option '%s-r%s|%s--reference=%s' requires an argument.", GREEN, RESET, YELLOW, RESET, YELLOW, RESET);
				status = 1;
				goto CLEANUP;
			}

			/* Free existing reference file string if present */
			if (options->_ref_file != NULL)
            {
                free(options->_ref_file);
            }
				
			/* Allocate and copy the reference file path */
			if ((options->_ref_file = malloc(strlen(value) + 1)) == NULL)
            {
                status = 1;
                goto FAILED_MEMORY_ALLOCATION;
            }

			strcpy(options->_ref_file, value);
		}
        /* -t: Use specific timestamp (format: [[CC]YY]MMDDhhmm[.ss]) */
        else if (strcmp(argv[i], "-t") == 0)
        {
            if (++i >= argc)
            {
				printf("%stouch%s: option '%s-t%s' requires an argument.", GREEN, RESET, YELLOW, RESET);
				status = 1;
				goto CLEANUP;
			}

            char* value = argv[++i];
            options->_timestamp = malloc(strlen(value) + 1);

            if (!options->_timestamp)
            {
                status = 1;
                goto FAILED_MEMORY_ALLOCATION;
            }

            strcpy(options->_timestamp, value);
        }
        /* --help: Display help information */
        else if (strcmp(argv[i], "--help") == 0)
        {
            _help();
            goto CLEANUP;
        }
        /* --version: Display version information */
        else if (strcmp(argv[i], "--version") == 0)
        {
            _version();
            goto CLEANUP;
        }
        /* Assume positional argument is the target file */
        else if (argv[i][0] != '-')
        {
            target = argv[i];
        }
    }

    /* Validate that a target file was provided */
    if (!target)
    {
        printf("Usage: %stouch%s [%sOPTION%s] %sFILE%s", GREEN, RESET, YELLOW, RESET, YELLOW, RESET);
        status = 1;
        goto CLEANUP;
    }

    char path[MAX_PATH];

    /* Build the full path - absolute paths are used as-is, relative paths are prefixed with current directory */
    if (_absolute(target))
    {
        /* Use the target path directly for absolute paths */
        strncpy(path, target, MAX_PATH - 1);
        path[sizeof(MAX_PATH) - 1] = '\0';
    }
    else
    {
        /* Get current directory and append the relative path */
        GetCurrentDirectory(MAX_PATH, path);
        strncat(path, "\\", MAX_PATH - strlen(path) - 1);
        strncat(path, target, MAX_PATH - strlen(path) - 1);
    }

    /* Normalize all forward slashes to backslashes for Windows */
    for (int i = 0; path[i]; i++)
    {
        if (path[i] == '/')
        {
            path[i] = '\\';
        }
    }

    /* Extract directory portion from the full path */
    char dir[MAX_PATH];
    strncpy(dir, path, MAX_PATH - 1);

    /* Find the last backslash to separate directory from filename */
    char* slash = strrchr(dir, '\\');

    /* Create parent directories if they don't exist */
    if (slash != NULL)
    {
        /* Terminate the directory string at the last separator */
        *slash = '\0';
        
        if (recursive_mkdir(dir) != 0)
        {
            printf("%sError%s: Could not create directory '%s%s%s'.", RED, RESET, YELLOW, dir, RESET);
            status = 1;
            goto CLEANUP;
        }
    }

    /* Determine file creation parameters based on options */
    DWORD flags = options->_no_deref ? FILE_FLAG_OPEN_REPARSE_POINT : 0;
    DWORD disp = options->_no_create ? OPEN_EXISTING : OPEN_ALWAYS;

#ifdef DEBUG
    /* Debug output to trace file path */
    printf("DEBUG: Attempting to create at: %s\n", path);
#endif

    /* Create or open the target file */
    hFile = CreateFile(
        path,
        FILE_WRITE_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE, 
        NULL,
        disp,
        FILE_ATTRIBUTE_NORMAL | flags,
        NULL
    );

    /* Check if file creation/opening succeeded */
    if (hFile == INVALID_HANDLE_VALUE)
    {
        /* If -c option was used and file doesn't exist, that's OK */
        if (options->_no_create)
        {
            goto CLEANUP;
        }

        /* Report error for other failure cases */
        printf("%sError%s: Cannot open/create file. Code: %s%lu%s\n", RED, RESET, YELLOW, GetLastError(), RESET);
        status = 1;
        goto CLEANUP;
    }

    /* Apply timestamps to the file */
    _timestamps(hFile, options);
	printf("Touched: %s%s%s", GREEN, argv[1], RESET);
    goto CLEANUP;

FAILED_MEMORY_ALLOCATION:
    printf("%sMemory allocation failed!%s", RED, RESET);

CLEANUP:
    /* Close the file handle if it was opened */
    if (hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hFile);
    }
    
    /* Free all allocated memory in the options structure */
    if (options)
    {
        free(options->_date);
        free(options->_ref_file);
        free(options->_timestamp);
        free(options);
        options = NULL;
    }
    
    return status;
}
