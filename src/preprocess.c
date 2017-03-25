/*
 * Copyright (C)  2016  Felix "KoffeinFlummi" Wiegand
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#include <wchar.h>
#else
#include <errno.h>
#include <fts.h>
#endif

#include "docopt.h"
#include "filesystem.h"
#include "utils.h"
#include "preprocess.h"


bool matches_includepath(char *path, char *includepath, char *includefolder) {
    /*
     * Checks if a given file can be matched to an include path by traversing
     * backwards through the filesystem until a $PBOPREFIX$ file is found.
     * If the prefix file, together with the diretory strucure, matches the
     * included path, true is returned.
     */

    int i;
    char cwd[2048];
    char prefixpath[2048];
    char prefixedpath[2048];
    char *ptr;
    FILE *f_prefix;

    strncpy(cwd, path, 2048);
    ptr = cwd + strlen(cwd);

    while (strcmp(includefolder, cwd) != 0) {
        while (*ptr != PATHSEP)
            ptr--;
        *ptr = 0;

        strncpy(prefixpath, cwd, 2048);
        strcat(prefixpath, PATHSEP_STR);
        strcat(prefixpath, "$PBOPREFIX$");

        f_prefix = fopen(prefixpath, "rb");
        if (!f_prefix)
            continue;

        fgets(prefixedpath, sizeof(prefixedpath), f_prefix);
        fclose(f_prefix);

        if (prefixedpath[strlen(prefixedpath) - 1] == '\n')
            prefixedpath[strlen(prefixedpath) - 1] = 0;
        if (prefixedpath[strlen(prefixedpath) - 1] == '\r')
            prefixedpath[strlen(prefixedpath) - 1] = 0;
        if (prefixedpath[strlen(prefixedpath) - 1] == '\\')
            prefixedpath[strlen(prefixedpath) - 1] = 0;

        strcat(prefixedpath, path + strlen(cwd));

        for (i = 0; i < strlen(prefixedpath); i++) {
            if (prefixedpath[i] == '/')
                prefixedpath[i] = '\\';
        }

        // compensate for missing leading slash in PBOPREFIX
        if (prefixedpath[0] != '\\')
            return (strcmp(prefixedpath, includepath+1) == 0);
        else
            return (strcmp(prefixedpath, includepath) == 0);
    }

    return false;
}


int find_file_helper(char *includepath, char *origin, char *includefolder, char *actualpath, char *cwd) {
    /*
     * Finds the file referenced in includepath in the includefolder. origin
     * describes the file in which the include is used (used for relative
     * includes). actualpath holds the return pointer. The 4th arg is used for
     * recursion on Windows and should be passed as NULL initially.
     *
     * Returns 0 on success, 1 on error and 2 if no file could be found.
     *
     * Please note that relative includes always return a path, even if that
     * file does not exist.
     */

    // relative include, this shit is easy
    if (includepath[0] != '\\') {
        strncpy(actualpath, origin, 2048);
        char *target = actualpath + strlen(actualpath) - 1;
        while (*target != PATHSEP && target >= actualpath)
            target--;
        strncpy(target + 1, includepath, 2046 - (target - actualpath));

#ifndef _WIN32
        int i;
        for (i = 0; i < strlen(actualpath); i++) {
            if (actualpath[i] == '\\')
                actualpath[i] = '/';
        }
#endif

        return 0;
    }

    char filename[2048];
    char *ptr = includepath + strlen(includepath);

    while (*ptr != '\\')
        ptr--;
    ptr++;

    strncpy(filename, ptr, 2048);

#ifdef _WIN32
    if (cwd == NULL)
        return find_file_helper(includepath, origin, includefolder, actualpath, includefolder);

    WIN32_FIND_DATA file;
    HANDLE handle = NULL;
    char mask[2048];

    GetFullPathName(includefolder, 2048, includefolder, NULL);

    GetFullPathName(cwd, 2048, mask, NULL);
    sprintf(mask, "%s\\*", mask);

    handle = FindFirstFile(mask, &file);
    if (handle == INVALID_HANDLE_VALUE)
        return 1;

    do {
        if (strcmp(file.cFileName, ".") == 0 || strcmp(file.cFileName, "..") == 0)
            continue;

        GetFullPathName(cwd, 2048, mask, NULL);
        sprintf(mask, "%s\\%s", mask, file.cFileName);
        if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (!find_file_helper(includepath, origin, includefolder, actualpath, mask))
                return 0;
        } else {
            if (strcmp(filename, file.cFileName) == 0 && matches_includepath(mask, includepath, includefolder)) {
                strncpy(actualpath, mask, 2048);
                return 0;
            }
        }
    } while (FindNextFile(handle, &file));

    FindClose(handle);
#else
    FTS *tree;
    FTSENT *f;
    char *argv[] = { includefolder, NULL };

    tree = fts_open(argv, FTS_LOGICAL | FTS_NOSTAT, NULL);
    if (tree == NULL)
        return 1;

    while ((f = fts_read(tree))) {
        switch (f->fts_info) {
            case FTS_DNR:
            case FTS_ERR:
                fts_close(tree);
                return 2;
            case FTS_NS: continue;
            case FTS_DP: continue;
            case FTS_D: continue;
            case FTS_DC: continue;
        }

        if (strcmp(filename, f->fts_name) == 0 && matches_includepath(f->fts_path, includepath, includefolder)) {
            strncpy(actualpath, f->fts_path, 2048);
            fts_close(tree);
            return 0;
        }
    }

    fts_close(tree);
#endif

    // check for file without pboprefix
    strncpy(filename, includefolder, sizeof(filename));
    strncat(filename, includepath, sizeof(filename) - strlen(filename) - 1);
#ifndef _WIN32
    int i;
    for (i = 0; i < strlen(filename); i++) {
        if (filename[i] == '\\')
            filename[i] = '/';
    }
#endif
    if (access(filename, F_OK) != -1) {
        strcpy(actualpath, filename);
        return 0;
    }

    return 2;
}


int find_file(char *includepath, char *origin, char *actualpath) {
    /*
     * Finds the file referenced in includepath in the includefolder. origin
     * describes the file in which the include is used (used for relative
     * includes). actualpath holds the return pointer. The 4th arg is used for
     * recursion on Windows and should be passed as NULL initially.
     *
     * Returns 0 on success, 1 on error and 2 if no file could be found.
     *
     * Please note that relative includes always return a path, even if that
     * file does not exist.
     */

    int i;
    int success;
    extern char include_folders[MAXINCLUDEFOLDERS][512];

    for (i = 0; i < MAXINCLUDEFOLDERS && include_folders[i][0] != 0; i++) {
        success = find_file_helper(includepath, origin, include_folders[i], actualpath, NULL);

        if (success != 2)
            return success;
    }

    return 2;
}


int resolve_includes(char *source, FILE *f_target, struct constant *constants, struct lineref *lineref) {
    /*
     * Writes the contents of source into the target file pointer, while
     * recursively resolving constants and includes using the includefolder
     * for finding included files.
     *
     * Returns 0 on success, a positive integer on failure.
     */

    extern int current_operation;
    extern char current_target[2048];
    extern char include_stack[MAXINCLUDES][1024];
    int file_index;
    int line = 0;
    int i = 0;
    int j = 0;
    int level_comment = 0;
    int success;
    size_t buffsize;
    char *buffer;
    char *ptr;
    char in_string = 0;
    char includepath[2048];
    char actualpath[2048];
    void *temp;
    FILE *f_source;

    current_operation = OP_PREPROCESS;
    strcpy(current_target, source);

    for (i = 0; i < MAXINCLUDES && include_stack[i][0] != 0; i++) {
        if (strcmp(source, include_stack[i]) == 0) {
            errorf("Circular dependency detected, printing include stack:\n", source);
            fprintf(stderr, "    !!! %s\n", source);
            for (j = MAXINCLUDES - 1; j >= 0; j--) {
                if (include_stack[j][0] == 0)
                    continue;
                fprintf(stderr, "        %s\n", include_stack[j]);
            }
            return 1;
        }
    }

    if (i == MAXINCLUDES) {
        errorf("Too many nested includes.\n");
        return 1;
    }

    strcpy(include_stack[i], source);

    f_source = fopen(source, "rb");
    if (!f_source) {
        errorf("Failed to open %s.\n", source);
        return 1;
    }

    // Skip byte order mark if it exists
    if (fgetc(f_source) == 0xef)
        fseek(f_source, 3, SEEK_SET);
    else
        fseek(f_source, 0, SEEK_SET);

    file_index = lineref->num_files;
    if (strchr(source, PATHSEP) == NULL)
        strcpy(lineref->file_names[file_index], source);
    else
        strcpy(lineref->file_names[file_index], strrchr(source, PATHSEP) + 1);

    lineref->num_files++;
    if (lineref->num_files >= MAXINCLUDES) {
        errorf("Number of included files exceeds MAXINCLUDES.\n");
        return 1;
    }

    // @todo predefined constants

    while (true) {
        // get line and add next lines if line ends with a backslash
        buffer = NULL;
        while (buffer == NULL || (strlen(buffer) >= 2 && buffer[strlen(buffer) - 2] == '\\')) {
            ptr = NULL;
            buffsize = 0;
            if (getline(&ptr, &buffsize, f_source) == -1) {
                free(ptr);
                ptr = NULL;
                break;
            }

            line++;

            if (buffer == NULL) {
                buffer = ptr;
            } else {
                buffer = (char *)realloc(buffer, strlen(buffer) + 2 + buffsize);
                strcpy(buffer + strlen(buffer) - 2, ptr);
                free(ptr);
            }

            // Add trailing new line if necessary
            if (strlen(buffer) >= 1 && buffer[strlen(buffer) - 1] != '\n')
                strcat(buffer, "\n");

            // fix windows line endings
            if (strlen(buffer) >= 2 && buffer[strlen(buffer) - 2] == '\r') {
                buffer[strlen(buffer) - 2] = '\n';
                buffer[strlen(buffer) - 1] = 0;
            }
        }

        if (buffer == NULL)
            break;

        // Check for block comment delimiters
        for (i = 0; i < strlen(buffer); i++) {
            if (in_string != 0) {
                if (buffer[i] == in_string && buffer[i-1] != '\\')
                    in_string = 0;
                else
                    continue;
            } else {
                if (level_comment == 0 &&
                        (buffer[i] == '"' || buffer[i] == '\'') &&
                        (i == 0 || buffer[i-1] != '\\'))
                    in_string = buffer[i];
            }

            if (buffer[i] == '/' && buffer[i+1] == '/') {
                buffer[i+1] = 0;
                buffer[i] = '\n';
            } else if (buffer[i] == '/' && buffer[i+1] == '*') {
                level_comment++;
                buffer[i] = ' ';
                buffer[i+1] = ' ';
            } else if (buffer[i] == '*' && buffer[i+1] == '/') {
                level_comment--;
                if (level_comment < 0)
                    level_comment = 0;
                buffer[i] = ' ';
                buffer[i+1] = ' ';
            }

            if (level_comment > 0) {
                buffer[i] = ' ';
                continue;
            }
        }

        // trim leading spaces
        trim_leading(buffer, strlen(buffer) + 1);

        // resolve includes
        if (level_comment == 0 && strlen(buffer) >= 11 && strncmp(buffer, "#include", 8) == 0) {
            for (i = 0; i < strlen(buffer) ; i++) {
                if (buffer[i] == '<' || buffer[i] == '>')
                    buffer[i] = '"';
            }
            if (strchr(buffer, '"') == NULL) {
                errorf("Failed to parse #include in line %i in %s.\n", line, source);
                return 5;
            }
            strncpy(includepath, strchr(buffer, '"') + 1, sizeof(includepath));
            if (strchr(includepath, '"') == NULL) {
                errorf("Failed to parse #include in line %i in %s.\n", line, source);
                return 6;
            }
            *strchr(includepath, '"') = 0;
            if (find_file(includepath, source, actualpath)) {
                errorf("Failed to find %s.\n", includepath);
                return 7;
            }

            success = resolve_includes(actualpath, f_target, constants, lineref);
            if (success) {
                errorf("Failed to preprocess %s.\n", actualpath);
                return success;
            }

            current_operation = OP_PREPROCESS;
            strcpy(current_target, source);

            for (i = 0; i < MAXINCLUDES && include_stack[i][0] != 0; i++);
            include_stack[i - 1][0] = 0;
        } else {
            fputs(buffer, f_target);

            lineref->file_index[lineref->num_lines] = file_index;
            lineref->line_number[lineref->num_lines] = line;

            lineref->num_lines++;
            if (lineref->num_lines % LINEINTERVAL == 0) {
                temp = malloc(sizeof(uint32_t) * (lineref->num_lines + LINEINTERVAL));
                memcpy(temp, lineref->line_number, sizeof(uint32_t) * lineref->num_lines);
                free(lineref->line_number);
                lineref->line_number = (uint32_t *)temp;

                temp = malloc(sizeof(uint32_t) * (lineref->num_lines + LINEINTERVAL));
                memcpy(temp, lineref->file_index, sizeof(uint32_t) * lineref->num_lines);
                free(lineref->file_index);
                lineref->file_index = (uint32_t *)temp;
            }
        }
        free(buffer);
    }

    fclose(f_source);

    return 0;
}


int preprocess(char *source, FILE *f_target, struct constant *constants, struct lineref *lineref) {
    FILE *f_temp;
    FILE *f_cpp;
    int i;
    int success;
    int datasize;
    size_t buffsize;
    char buffer[4096];
    char *ptr;
    char cpp_in[2048];
    char cpp_out[2048];
    char *mcpp_argv[] = { "mcpp", cpp_in, cpp_out };

#ifdef _WIN32
    if (!GetTempFileName(".", "amk", 0, cpp_in) || !GetTempFileName(".", "amk", 0, cpp_out)) {
        errorf("Failed to get temp files.\n");
        return 1;
    }
#else
    tmpnam(cpp_in);
    tmpnam(cpp_out);
#endif

    f_temp = tmpfile();
    if (resolve_includes(source, f_temp, constants, lineref)) {
        errorf("Failed to resolve includes for %s.\n", source);
        return 1;
    }

    // write to file to pass to cpp
    f_cpp = fopen(cpp_in, "wb+"); 
    if (!f_cpp) {
        errorf("Failed to open temp file.\n");
        return 2;
    }

    fseek(f_temp, 0, SEEK_END);
    datasize = ftell(f_temp);

    fseek(f_temp, 0, SEEK_SET);
    for (i = 0; datasize - i >= sizeof(buffer); i += sizeof(buffer)) {
        fread(buffer, sizeof(buffer), 1, f_temp);
        fwrite(buffer, sizeof(buffer), 1, f_cpp);
    }
    fread(buffer, datasize - i, 1, f_temp);
    fwrite(buffer, datasize - i, 1, f_cpp);

    fclose(f_cpp);
    fclose(f_temp);

    // call cpp
    success = mcpp_lib_main(sizeof(mcpp_argv) / sizeof(mcpp_argv[0]), mcpp_argv);
    if (success) {
        errorf("MCPP failed to preprocess %s.\n", source);
        return success;
    }

    // write to output, discarding line markers
    f_cpp = fopen(cpp_out, "rb");
    if (f_cpp == NULL) {
        errorf("Failed to open temp file.\n");
        return 3;
    }

    while (true) {
        ptr = NULL;
        buffsize = 0;
        success = getline(&ptr, &buffsize, f_cpp);
        if (success <= 0 || ptr == NULL) {
            if (false && ptr && success < 0)
                free(ptr);
            break;
        }
        if (ptr[0] != '#') {
            if (ptr[strlen(ptr) - 2] == '\r') {
                ptr[strlen(ptr) - 2] = '\n';
                ptr[strlen(ptr) - 1] = 0;
            }
            fwrite(ptr, strlen(ptr), 1, f_target);
        }
        free(ptr);
    }

    fclose(f_cpp);
#ifdef _WIN32
    _rmtmp();
#endif

    remove_file(cpp_in);
    remove_file(cpp_out);

    return 0;
}
