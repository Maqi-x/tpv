#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NOB_IMPLEMENTATION
#include "external/nob/nob.h"

#define CHECK(call) do { int ret = call; if (ret != 0) return ret; } while (0)

#define BUILD_DIR "build/"
#define OUT_DIR "out/"

typedef enum BuildMode {
    Debug = 0,
    Release,
} BuildMode;

typedef struct BuildCmdOptions BuildCmdOptions;
typedef struct RebuildCmdOptions RebuildCmdOptions;
typedef struct RunCmdOptions RunCmdOptions;
typedef struct CleanCmdOptions CleanCmdOptions;
typedef struct RebuildRunCmdOptions RebuildRunCmdOptions;
typedef struct InstallCmdOptions InstallCmdOptions;

typedef union CmdOptions {
    struct BuildCmdOptions {
        BuildMode mode;
        bool use_asan;
        bool use_ubsan;
        // const char* cc;
        // const char* additional_compile_flags;
        // const char* additional_link_flags;
    } build;
    struct CleanCmdOptions {} clean;
    struct RebuildCmdOptions {
        BuildCmdOptions build_options;
        CleanCmdOptions clean_options;
    } rebuild;
    struct RunCmdOptions {
        BuildCmdOptions build_options;
        const char* run_flags;
    } run;
    struct RebuildRunCmdOptions {
        RebuildCmdOptions rebuild_options;
        RunCmdOptions run_options;
    } rebuild_run;
    struct InstallCmdOptions {
        BuildCmdOptions build_options;
    } install;
} CmdOptions;

// helper
#ifdef _WIN32
bool delete_dir(const char *path) {
    WIN32_FIND_DATAA fd;
    char search_path[MAX_PATH];
    snprintf(search_path, MAX_PATH, "%s\\*", path);

    HANDLE h = FindFirstFileA(search_path, &fd);
    if (h == INVALID_HANDLE_VALUE) {
        return false;
    }

    do {
        if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0)
            continue;

        char full_path[MAX_PATH];
        snprintf(full_path, MAX_PATH, "%s\\%s", path, fd.cFileName);

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (!delete_dir(full_path)) {
                FindClose(h);
                return false;
            }
        } else {
            if (!DeleteFileA(full_path)) {
                FindClose(h);
                return false;
            }
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);

    return RemoveDirectoryA(path);
}

#else // POSIX

bool delete_dir(const char *path) {
    DIR *d = opendir(path);
    if (!d) return false;

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char full_path[4096];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            if (!delete_dir(full_path)) {
                closedir(d);
                return false;
            }
        } else {
            if (remove(full_path) != 0) {
                closedir(d);
                return false;
            }
        }
    }
    closedir(d);

    return rmdir(path) == 0;
}

#endif

// helper
bool read_entire_dir_recursive(const char *parent, Nob_File_Paths *children, size_t base_len) {
    Nob_File_Paths entries = {0};
    if (!nob_read_entire_dir(parent, &entries)) return false;

    for (size_t i = 0; i < entries.count; i++) {
        const char* name = entries.items[i];
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;

        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", parent, name);

        Nob_File_Type type = nob_get_file_type(path);

        if (type == NOB_FILE_DIRECTORY) {
            read_entire_dir_recursive(path, children, base_len);
        } else {
            size_t len = strlen(path);
            if (len > 2 && strcmp(path + len - 2, ".c") == 0) {
                const char *rel_path = path + base_len; // skip "src/"
                if (*rel_path == '/') rel_path++;       // skip leading '/'
                nob_da_append(children, nob_temp_strdup(rel_path));
            }
        }
    }

    nob_da_free(entries);
    return true;
}

typedef struct Flags {
    const char** items;
    size_t count;
    size_t capacity;
} Flags;

const char* get_project_root() {
    return getenv("__TPV_PROJECT_ROOT");
}

int chdir_to_project_root() {
    return chdir(get_project_root());
}

const char* get_build_subdir_name(BuildCmdOptions* opts) {
    return opts->mode == Debug ? "debug" : "release";
}

const char* get_output_bin_name(BuildCmdOptions* opts) {
    return opts->mode == Debug ? "out/tpv-debug.elf" : "out/tpv-release.elf";
}

int build_external_libs(Flags* additional_compile_flags, Flags* additional_link_flags) {
    // noop at the moment
    return 0;
}

bool needs_rebuild(const char *src, const char *obj) {
    struct stat src_stat, obj_stat;
    if (stat(src, &src_stat) != 0) return true;  // just in case
    if (stat(obj, &obj_stat) != 0) return true;  // rebuild if object file does not exists
    return src_stat.st_mtime > obj_stat.st_mtime;
}

int build(BuildCmdOptions* opts) {
    chdir_to_project_root();
    nob_mkdir_if_not_exists(BUILD_DIR);
    nob_mkdir_if_not_exists(OUT_DIR);

    Flags compile_flags = {0}, link_flags = {0};
    build_external_libs(&compile_flags, &link_flags);

    nob_da_append(&compile_flags, "-Iinclude");
    nob_da_append(&compile_flags, "-Iexternal");

    chdir_to_project_root();

    Nob_File_Paths sources = {0};
    read_entire_dir_recursive("src", &sources, strlen("src"));

    char object_files_dir[PATH_MAX];
    snprintf(
        object_files_dir, sizeof(object_files_dir),
        "%s/build/%s", get_project_root(), get_build_subdir_name(opts));
    nob_mkdir_if_not_exists(object_files_dir);

    Nob_File_Paths objects = {0};

    for (size_t i = 0; i < sources.count; ++i) {
        char source_path[PATH_MAX];
        snprintf(source_path, sizeof source_path, "./src/%s", sources.items[i]);

        char path_normalized[PATH_MAX];
        strncpy(path_normalized, sources.items[i], sizeof path_normalized);
        path_normalized[sizeof(path_normalized) - 1] = '\0';
        
        int len = strlen(path_normalized);
        if (len > 2 && strcmp(path_normalized + len - 2, ".c") == 0) {
            path_normalized[len - 2] = '\0'; // remove .c
            len -= 2;
        }
        
        for (int i = 0; i < len; ++i) {
            if (path_normalized[i] == '/') path_normalized[i] = '_';
        }
        
        char* out_obj_path = nob_temp_sprintf("%s/%s.o", object_files_dir, path_normalized);

        if (!needs_rebuild(source_path, out_obj_path)) {
            nob_log(NOB_INFO, "Skipping %s.c (up to date)", path_normalized);
            nob_da_append(&objects, out_obj_path);
            continue;
        }

        Nob_Cmd cc = {0};
        nob_cc(&cc);
        nob_cmd_extend(&cc, &compile_flags);
        nob_cmd_append(&cc, "-Wall", "-Wextra");
        if (opts->mode == Release) {
            nob_cmd_append(&cc, "-O3", "-flto", "-DNDEBUG");
        } else if (opts->mode == Debug) {
            nob_cmd_append(&cc, "-O0", "-g");
        }
        if (opts->use_asan) {
            nob_cmd_append(&cc, "-fsanitize=address");
        }
        if (opts->use_ubsan) {
            nob_cmd_append(&cc, "-fsanitize=undefined");
        }
        nob_cmd_append(&cc, source_path, "-c", "-o", out_obj_path);

        if (!nob_cmd_run(&cc, .async = false)) {
            nob_log(NOB_ERROR, "Failed to compile %s.c", path_normalized);
            nob_cmd_free(cc);
            return 1;
        }

        nob_cmd_free(cc);
        nob_da_append(&objects, out_obj_path);
    }
    nob_da_free(compile_flags);

    Nob_Cmd link = {0};
    nob_cc(&link);
    if (opts->mode == Release) {
        nob_cmd_append(&link, "-flto");
    }
    if (opts->use_asan) {
        nob_cmd_append(&link, "-fsanitize=address");
    }
    if (opts->use_ubsan) {
        nob_cmd_append(&link, "-fsanitize=undefined");
    }

    const char* out = get_output_bin_name(opts);

    nob_cmd_extend(&link, &objects);
    nob_cmd_extend(&link, &link_flags);
    nob_cmd_append(&link, "-o", out);

    nob_da_free(link_flags);
    if (!nob_cmd_run(&link, .async = false)) {
        nob_log(NOB_ERROR, "Failed to link executable %s.", out);
        nob_cmd_free(link);
        return 1;
    }

    nob_cmd_free(link);
    nob_log(NOB_INFO, "TPV compiled and linked successfully");
    return 0;
}

int clean(CleanCmdOptions* opts) {
    chdir_to_project_root();
    if (!delete_dir("build")) return 1;
    if (!delete_dir("out")) return 1;
    return 0;
}

int rebuild(RebuildCmdOptions* opts) {
    CleanCmdOptions clean_options = {};
    CHECK(clean(&clean_options));

    return build(&opts->build_options);
}

int run(RunCmdOptions* opts) {
    CHECK(build(&opts->build_options));

    Nob_Cmd run = {0};
    nob_cmd_append(&run, get_output_bin_name(&opts->build_options));
    if (!nob_cmd_run(&run, .async = false)) {
        nob_log(NOB_ERROR, "Failed to run %s", get_output_bin_name(&opts->build_options));
        return 1;
    }

    return 0;
}

int rebuild_run(RebuildRunCmdOptions* opts) {
    CHECK(rebuild(&opts->rebuild_options));
    return run(&opts->run_options);
}

int install(InstallCmdOptions* opts) {
    CHECK(build(&opts->build_options));

    Nob_Cmd cp = {0};
    nob_cmd_append(&cp, "cp");
    nob_cmd_append(&cp, get_output_bin_name(&opts->build_options));
    nob_cmd_append(&cp, "/usr/bin/tpv");

    if (!nob_cmd_run(&cp, .async = false)) {
        nob_log(NOB_ERROR, "Failed to install %s", get_output_bin_name(&opts->build_options));
        return 1;
    }

    return 0;
}

char* shift(int* argc, char*** argv) {
    if (*argc == 0)
        return NULL;
    char* result = (*argv)[0];
    (*argv)++;
    (*argc)--;
    return result;
}

char* peek(int* argc, char*** argv) {
    if (*argc == 0)
        return NULL;
    char* result = (*argv)[0];
    return result;
}

bool starts_with(const char* str, const char* prefix) {
    size_t str_len = strlen(str);
    size_t prefix_len = strlen(prefix);
    if (str_len < prefix_len) return false;
    return memcmp(str, prefix, prefix_len) == 0;
}

bool implies_build_command(const char* command) {
    if (command == NULL) return true;
    return
        strcmp(command, "build")       == 0
     || strcmp(command, "rebuild")     == 0
     || strcmp(command, "run")         == 0
     || strcmp(command, "rebuild-run") == 0
     || strcmp(command, "install")     == 0;
}

int main(int argc, char** argv) {
    const char* prog_name = shift(&argc, &argv); 

    CmdOptions cmd_options = {0};

    const char* command = NULL;
    const char* opt;
    while ((opt = peek(&argc, &argv)) != NULL) {
        shift(&argc, &argv);

        if (!starts_with(opt, "--")) {
            bool command_exists = false
                || strcmp(opt, "build")       == 0
                || strcmp(opt, "rebuild")     == 0
                || strcmp(opt, "clean")       == 0
                || strcmp(opt, "run")         == 0
                || strcmp(opt, "rebuild-run") == 0
                || strcmp(opt, "install")     == 0;

            if (command != NULL) {
                nob_log(NOB_ERROR, "Unexpected argument: %s", opt);
                return 1;
            } else if (!command_exists) {
                nob_log(NOB_ERROR, "Unknown command: %s. Exptected build, run or clean", opt);
                return 1;
            }
            command = opt;
        }

        if (strcmp(opt, "--mode") == 0) {
            const char* mode = shift(&argc, &argv);

            if (mode == NULL) {
                nob_log(NOB_ERROR, "--mode requires an argument\n");
                return 1;
            }

            if (strcmp(mode, "debug") == 0) {
                cmd_options.build.mode = Debug;
            } else if (strcmp(mode, "release") == 0) {
                cmd_options.build.mode = Release;
            } else {
                nob_log(NOB_ERROR, "Unknown build mode: %s. Expeced debug or release.", mode);
                return 1;
            }
        } else if (strcmp(opt, "--use-asan") == 0 || strcmp(opt, "--force-use-asan") == 0) {
            if (!implies_build_command(command)) {
                nob_log(NOB_ERROR, "--use-asan: This flag works only with build and run commands");
                return 1;
            }
            if (cmd_options.build.mode == Release && strcmp(opt, "--force-use-asan") != 0) {
                nob_log(NOB_ERROR, "--use-asan: You are about to use asan in release build, which is not recommended. Use --force-use-asan to force");
                return 1;
            }
            cmd_options.build.use_asan = true;
        } else if (strcmp(opt, "--use-ubsan") == 0 || strcmp(opt, "--force-use-ubsan") == 0) {
            if (!implies_build_command(command)) {
                nob_log(NOB_ERROR, "--use-ubsan: This flag works only with build and run commands");
                return 1;
            }
            if (cmd_options.build.mode == Release && strcmp(opt, "--force-use-ubsan") != 0) {
                nob_log(NOB_ERROR, "--use-ubsan: You are about to use ubsan in release build, which is not recommended. Use --force-use-ubsan to force");
                return 1;
            }
            cmd_options.build.use_ubsan = true;
        }
    }

    if (command == NULL) command = "build";

    if (strcmp(command, "build") == 0) {
        return build(&cmd_options.build);
    } else if (strcmp(command, "rebuild") == 0) {
        return rebuild(&cmd_options.rebuild);
    } else if (strcmp(command, "run") == 0) {
        return run(&cmd_options.run);
    } else if (strcmp(command, "rebuild-run") == 0) {
        return rebuild_run(&cmd_options.rebuild_run);
    } else if (strcmp(command, "clean") == 0) {
        return clean(&cmd_options.clean);
    } else if (strcmp(command, "install") == 0) {
        return install(&cmd_options.install);
    } else {
        nob_log(NOB_ERROR, "Unknown command: %s.", command);
        return 1;
    }
}
