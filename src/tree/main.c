/**
 * @file main.c
 * @brief Directory tree visualization utility for Windows
 *
 * @description
 * A Windows command-line utility that displays the directory tree
 * structure starting from a specified path. It recursively traverses
 * directories and presents the hierarchy in an ASCII tree format,
 * similar to the Unix 'tree' command.
 *
 * Features:
 * - Recursive depth-first directory traversal
 * - ASCII tree visualization with branch characters
 * - File and directory counting
 * - Volume information display (drive, serial number, filesystem type)
 * - Colored terminal output using ANSI escape sequences
 *
 * @usage
 *   tree [directory_path]
 *
 * @examples
 *   tree              # Display tree of current directory
 *   tree C:\Users     # Display tree of specified path
 *
 * @note Requires Windows API (WIN32_FIND_DATA, FindFirstFile/FindNextFile)
 *
 * @author monakaibrahim-cmyk
 * @version 1.0.0
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @defgroup ColorCodes Terminal Color Escape Sequences
 * @brief ANSI escape codes for colored console output
 * @{
 */

/* Text color codes - prefix with color macro, suffix with RESET to restore default */
#define BLACK   "\x1B[30m"
#define RED     "\x1B[31m"
#define GREEN   "\x1B[32m"
#define YELLOW  "\x1B[33m"
#define BLUE    "\x1B[34m"
#define MAGENTA "\x1B[35m"
#define CYAN    "\x1B[36m"
#define WHITE   "\x1B[37m"
#define RESET   "\x1B[0m"   /**< Reset all formatting to default */
 /** @} */

/**
 * @brief Linked list node for storing directory entries during traversal
 *
 * Stores a single file/directory entry from Windows FindFirstFile/FindNextFile
 * API. Used to batch-collect directory contents before processing.
 */
typedef struct Node
{
    WIN32_FIND_DATA data;   /**< Windows file data (name, attributes, size, etc.) */
    struct Node* next;      /**< Pointer to next entry in list */
} Node;

/**
 * @brief Frees all nodes in a linked list
 *
 * Iterates through the list and frees each node to prevent memory leaks.
 * Called after directory processing is complete.
 *
 * @param[in] head Pointer to the head of the list to free
 */
void free_list(Node* head)
{
    /* Traverse the linked list and free each node to prevent memory leaks */
    while (head)
    {
        Node* temp = head;
        head = head->next;
        free(temp);
    }
}

/**
 * @brief Recursively lists directory contents in tree format
 *
 * Performs depth-first traversal starting from base directory.
 * Builds a linked list of entries first, then processes them to display
 * the tree with proper ASCII branch formatting:
 *   "├──" = non-last entry
 *   "└──" = last entry
 *   "│"   = vertical continuation
 *
 * @param[in] base       Base directory path to list
 * @param[in] prefix     Current prefix string for tree formatting
 * @param[out] file_count Accumulates total files found
 * @param[out] dir_count Accumulates total directories found
 */
void list_dir(const char* base, const char* prefix, int* file_count, int* dir_count)
{
    /* Construct search pattern: base_path\* to enumerate all entries */
    char search_path[MAX_PATH];
    int sp_len = snprintf(search_path, MAX_PATH, "%s\\*", base);

    if (sp_len < 0 || sp_len >= MAX_PATH)
    {
        return;
    }

    /* Initialize Windows FindFirstFile to begin enumeration */
    WIN32_FIND_DATA ffd;
    HANDLE hFind = FindFirstFile(search_path, &ffd);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        return;
    }

    /* Build a linked list of directory entries first (batch collection)
     * This separates enumeration from processing, allowing sorted output */
    Node *head = NULL, *tail = NULL;

    do
    {
        /* Skip special directory references: "." (current) and ".." (parent) */
        if (strcmp(ffd.cFileName, ".") == 0 || strcmp(ffd.cFileName, "..") == 0)
        {
            continue;
        }

        /* Allocate new node to store this directory entry */
        Node* new_node = (Node*)malloc(sizeof(Node));

        if (!new_node)
        {
            fprintf(stderr, "Error: Memory exhausted while enumerating " YELLOW "'%s' " RESET ". Output may be incomplete.\n", base);
            break;
        }

        new_node->data = ffd;
        new_node->next = NULL;

        /* Append to linked list (tail insertion for O(1) append) */
        if (!head)
        {
            head = tail = new_node;
        }
        else
        { 
            tail->next = new_node;
            tail = new_node;
        }
    }
    while (FindNextFile(hFind, &ffd));
    
    /* Close the find handle - no longer needed after enumeration */
    FindClose(hFind);

    /* Process each entry in the linked list and display tree structure */
    Node* current = head;

    while (current)
    {
        /* Determine if this is the last entry for proper tree branch formatting:
         * ├── = not last, └── = last */
        int is_last = (current->next == NULL);
        
        /* Print the tree branch prefix + filename in green color */
        printf("%s%s " GREEN "%s" RESET "\n", prefix, is_last ? "└──" : "├──", current->data.cFileName);

        /* Check if entry is a directory (vs file) using Windows attribute flags */
        if (current->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            (*dir_count)++;
            
            /* Construct full path for recursive descent into subdirectory */
            char next_path[MAX_PATH];
            int path_len = snprintf(next_path, MAX_PATH, "%s\\%s", base, current->data.cFileName);

            if (path_len < 0 || path_len >= MAX_PATH)
            {
                fprintf(stderr, "Error: Path too long: " YELLOW "'%s'" RESET ". Skipping.\n", current->data.cFileName);
            } 
            else
            {
                /* Calculate prefix for next recursion level:
                 * - If last entry: use spaces "   " (no vertical branch)
                 * - If not last: use "│" (vertical branch continues) */
                int probe = snprintf(NULL, 0, "%s%s   ", prefix, is_last ? " " : "│");

                if (probe < 0)
                {
                    fprintf(stderr, "Error: Encoding failed for prefix at " YELLOW "'%s'" RESET ". Skipping.\n", current->data.cFileName);
                }
                else
                {
                    size_t needed = (size_t)probe + 1;
                    char* next_prefix = malloc(needed);

                    if (next_prefix)
                    {
                        /* Format the prefix and recurse into subdirectory */
                        snprintf(next_prefix, needed, "%s%s   ", prefix, is_last ? " " : "│");
                        list_dir(next_path, next_prefix, file_count, dir_count);
                        free(next_prefix);
                    }
                    else
                    {
                        fprintf(stderr, "Error: Memory exhausted at " YELLOW "'%s'" RESET ".\n", current->data.cFileName);
                    }
                }
            }
        }
        else
        {
            /* Entry is a file - increment file counter */
            (*file_count)++;
        }

        current = current->next;
    }

    /* Clean up linked list memory to prevent leaks */
    free_list(head);
}

/**
 * @brief Main entry point for the tree command
 *
 * Sets up UTF-8 console encoding, resolves the target path,
 * displays volume information, then initiates recursive directory
 * traversal to visualize the tree structure.
 *
 * @param[in] argc Number of command-line arguments
 * @param[in] argv Argument vector:
 *                 - argv[0]: Program name
 *                 - argv[1]: Optional directory path (default: ".")
 *
 * @return EXIT_SUCCESS on successful completion
 * @return EXIT_FAILURE on error (invalid path, resolution failure, etc.)
 */
int main(int argc, char** argv)
{
    /* Configure console to use UTF-8 encoding for proper Unicode display */
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    /* Determine target path: use provided argument or default to current directory */
    const char* path = (argc > 1) ? argv[1] : ".";

    /* Resolve the path to an absolute full path using Windows API */
    char full[MAX_PATH];
    DWORD result = GetFullPathNameA(path, MAX_PATH, full, NULL);

    if (result == 0)
    {
        fprintf(stderr, "Error: GetFullPathNameA failed with code " YELLOW "%lu" RESET "\n", GetLastError());
        return EXIT_FAILURE;
    }
    else if (result >= MAX_PATH)
    {
        fprintf(stderr, "Error: The full path is longer than the buffer size.\n");
        return EXIT_FAILURE;
    }

    /* Retrieve and display volume information (drive letter, serial, filesystem) */
    char root[MAX_PATH];
    char type[32];

    if (GetVolumePathNameA(full, root, MAX_PATH))
    {
        DWORD serial = 0;

        printf("Drive: %s%s%s\n", CYAN, root, RESET);

        if (GetVolumeInformationA(root, NULL, 0, &serial, NULL, NULL, type, sizeof(type)))
        {
            printf("Volume Serial Number: " YELLOW "%04X-%04X" RESET "\n", (UINT)(serial >> 16), (UINT)(serial & 0xFFFF));
            printf("File System Type: " GREEN "%s" RESET "\n\n", type);
        }
    }

    /* Initialize counters for total files and directories discovered */
	int files = 0;
    int dirs = 0;

    /* Print the root path header in green, then begin recursive tree traversal */
	printf(GREEN "%s" RESET "\n", full);

    /* Recursively list directory contents and build the tree visualization */
	list_dir(full, "", &files, &dirs);

    /* Display final statistics: total directories and files counted */
	printf("\n" YELLOW "%d" RESET " directories, " YELLOW "%d" RESET " files\n", dirs, files);
	fflush(stdout);

    return EXIT_SUCCESS;
}
