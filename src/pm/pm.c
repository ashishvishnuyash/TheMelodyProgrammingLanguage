#include "pm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
  #include <direct.h>
  #define mkdir(p, m) _mkdir(p)
  #define PATH_SEP '\\'
  #define RMDIR_CMD "rmdir /s /q "
#else
  #include <unistd.h>
  #define PATH_SEP '/'
  #define RMDIR_CMD "rm -rf "
#endif

#define MANIFEST    "melody.json"
#define MODULES_DIR "melody_modules"
#define MAX_DEPS    128
#define MAX_NAME    128
#define MAX_SOURCE  256

/* ------------------------------------------------------------------ */
/*  Data structures                                                    */
/* ------------------------------------------------------------------ */

typedef struct {
    char name[MAX_NAME];
    char source[MAX_SOURCE]; /* "github:user/repo@version" */
} Dependency;

typedef struct {
    char name[MAX_NAME];
    char version[32];
    Dependency deps[MAX_DEPS];
    int dep_count;
} Manifest;

/* ------------------------------------------------------------------ */
/*  Helper utilities                                                   */
/* ------------------------------------------------------------------ */

static int file_exists(const char* path) {
    struct stat st;
    return (stat(path, &st) == 0 && (st.st_mode & S_IFREG));
}

static int dir_exists(const char* path) {
    struct stat st;
    return (stat(path, &st) == 0 && (st.st_mode & S_IFDIR));
}

/* ------------------------------------------------------------------ */
/*  manifest_read — parse melody.json into a Manifest struct           */
/* ------------------------------------------------------------------ */

static int manifest_read(Manifest* m) {
    FILE* fp;
    char* buf;
    long len;
    char* p;
    char* end;

    memset(m, 0, sizeof(Manifest));

    fp = fopen(MANIFEST, "r");
    if (!fp) return -1;

    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    rewind(fp);

    buf = (char*)malloc(len + 1);
    if (!buf) { fclose(fp); return -1; }

    fread(buf, 1, len, fp);
    buf[len] = '\0';
    fclose(fp);

    /* Parse "name": "value" */
    p = strstr(buf, "\"name\"");
    if (p) {
        p = strchr(p + 6, '"');  /* skip past "name" to colon area */
        if (p) {
            p++;  /* opening quote of value */
            end = strchr(p, '"');
            if (end) {
                int n = (int)(end - p);
                if (n >= MAX_NAME) n = MAX_NAME - 1;
                strncpy(m->name, p, n);
                m->name[n] = '\0';
            }
        }
    }

    /* Parse "version": "value" */
    p = strstr(buf, "\"version\"");
    if (p) {
        p = strchr(p + 9, '"');
        if (p) {
            p++;
            end = strchr(p, '"');
            if (end) {
                int n = (int)(end - p);
                if (n >= (int)sizeof(m->version)) n = (int)sizeof(m->version) - 1;
                strncpy(m->version, p, n);
                m->version[n] = '\0';
            }
        }
    }

    /* Parse "dependencies": { "name": "source", ... } */
    p = strstr(buf, "\"dependencies\"");
    if (p) {
        p = strchr(p, '{');
        if (p) {
            char* block_end = strchr(p, '}');
            if (block_end) {
                /* Walk through key-value pairs inside the braces */
                char* cursor = p + 1;
                m->dep_count = 0;

                while (cursor < block_end && m->dep_count < MAX_DEPS) {
                    char* key_start = strchr(cursor, '"');
                    if (!key_start || key_start >= block_end) break;
                    key_start++;

                    char* key_end = strchr(key_start, '"');
                    if (!key_end || key_end >= block_end) break;

                    int klen = (int)(key_end - key_start);
                    if (klen >= MAX_NAME) klen = MAX_NAME - 1;
                    strncpy(m->deps[m->dep_count].name, key_start, klen);
                    m->deps[m->dep_count].name[klen] = '\0';

                    /* Find value string */
                    char* val_start = strchr(key_end + 1, '"');
                    if (!val_start || val_start >= block_end) break;
                    val_start++;

                    char* val_end = strchr(val_start, '"');
                    if (!val_end || val_end >= block_end) break;

                    int vlen = (int)(val_end - val_start);
                    if (vlen >= MAX_SOURCE) vlen = MAX_SOURCE - 1;
                    strncpy(m->deps[m->dep_count].source, val_start, vlen);
                    m->deps[m->dep_count].source[vlen] = '\0';

                    m->dep_count++;
                    cursor = val_end + 1;
                }
            }
        }
    }

    free(buf);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  manifest_write — write Manifest struct to melody.json              */
/* ------------------------------------------------------------------ */

static int manifest_write(const Manifest* m) {
    FILE* fp;
    int i;

    fp = fopen(MANIFEST, "w");
    if (!fp) {
        fprintf(stderr, "error: cannot write %s\n", MANIFEST);
        return -1;
    }

    fprintf(fp, "{\n");
    fprintf(fp, "  \"name\": \"%s\",\n", m->name);
    fprintf(fp, "  \"version\": \"%s\",\n", m->version);
    fprintf(fp, "  \"dependencies\": {");

    if (m->dep_count > 0) {
        fprintf(fp, "\n");
        for (i = 0; i < m->dep_count; i++) {
            fprintf(fp, "    \"%s\": \"%s\"", m->deps[i].name, m->deps[i].source);
            if (i < m->dep_count - 1)
                fprintf(fp, ",");
            fprintf(fp, "\n");
        }
        fprintf(fp, "  ");
    }

    fprintf(fp, "}\n}\n");
    fclose(fp);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  pm_init — initialise a new Melody project in cwd                   */
/* ------------------------------------------------------------------ */

int pm_init(void) {
    Manifest m;
    char cwd[1024];
    char* dir_name;

    /* Already initialised? */
    if (file_exists(MANIFEST)) {
        printf("Already initialized.\n");
        return 0;
    }

    /* Determine project name from current directory */
    if (!getcwd(cwd, sizeof(cwd))) {
        fprintf(stderr, "error: cannot determine current directory\n");
        return -1;
    }

    dir_name = strrchr(cwd, PATH_SEP);
    if (dir_name)
        dir_name++;  /* skip past the separator */
    else
        dir_name = cwd;

    /* Build manifest */
    memset(&m, 0, sizeof(Manifest));
    strncpy(m.name, dir_name, MAX_NAME - 1);
    m.name[MAX_NAME - 1] = '\0';
    strncpy(m.version, "0.1.0", sizeof(m.version) - 1);
    m.version[sizeof(m.version) - 1] = '\0';
    m.dep_count = 0;

    /* Write melody.json */
    if (manifest_write(&m) != 0)
        return -1;

    /* Create melody_modules/ directory */
    if (!dir_exists(MODULES_DIR)) {
        if (mkdir(MODULES_DIR, 0755) != 0) {
            fprintf(stderr, "error: cannot create %s/\n", MODULES_DIR);
            return -1;
        }
    }

    /* Append melody_modules/ to .gitignore if not already present */
    {
        int found = 0;
        FILE* fp;

        if (file_exists(".gitignore")) {
            char line[512];
            fp = fopen(".gitignore", "r");
            if (fp) {
                while (fgets(line, sizeof(line), fp)) {
                    /* Strip trailing newline for comparison */
                    char* nl = strchr(line, '\n');
                    if (nl) *nl = '\0';
                    nl = strchr(line, '\r');
                    if (nl) *nl = '\0';

                    if (strcmp(line, "melody_modules/") == 0 ||
                        strcmp(line, "melody_modules")  == 0) {
                        found = 1;
                        break;
                    }
                }
                fclose(fp);
            }
        }

        if (!found) {
            fp = fopen(".gitignore", "a");
            if (fp) {
                fprintf(fp, "melody_modules/\n");
                fclose(fp);
            }
        }
    }

    printf("Initialized Melody project \"%s\" (v%s).\n", m.name, m.version);
    printf("  created %s\n", MANIFEST);
    printf("  created %s/\n", MODULES_DIR);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Source parsing                                                      */
/* ------------------------------------------------------------------ */

typedef struct {
    char user[MAX_NAME];
    char repo[MAX_NAME];
    char version[64]; /* empty = default branch */
    char url[512];    /* https://github.com/user/repo.git */
} ParsedSource;

static int parse_source(const char* source, ParsedSource* out) {
    memset(out, 0, sizeof(ParsedSource));
    if (strncmp(source, "github:", 7) != 0) {
        fprintf(stderr, "Error: unsupported source '%s' (use github:user/repo)\n", source);
        return -1;
    }
    const char* rest = source + 7;
    const char* slash = strchr(rest, '/');
    if (!slash) {
        fprintf(stderr, "Error: invalid format '%s' (expected github:user/repo)\n", source);
        return -1;
    }
    size_t ulen = slash - rest;
    if (ulen >= MAX_NAME) ulen = MAX_NAME - 1;
    memcpy(out->user, rest, ulen);
    out->user[ulen] = '\0';

    const char* repo_start = slash + 1;
    const char* at = strchr(repo_start, '@');
    if (at) {
        size_t rlen = at - repo_start;
        if (rlen >= MAX_NAME) rlen = MAX_NAME - 1;
        memcpy(out->repo, repo_start, rlen);
        out->repo[rlen] = '\0';
        snprintf(out->version, sizeof(out->version), "%s", at + 1);
    } else {
        snprintf(out->repo, MAX_NAME, "%s", repo_start);
        out->version[0] = '\0';
    }
    snprintf(out->url, sizeof(out->url), "https://github.com/%s/%s.git", out->user, out->repo);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Download helpers                                                    */
/* ------------------------------------------------------------------ */

static int download_git(const ParsedSource* src, const char* dest) {
    char cmd[1024];
    if (src->version[0]) {
        snprintf(cmd, sizeof(cmd),
                 "git clone --depth 1 --branch %s %s \"%s\" 2>&1",
                 src->version, src->url, dest);
    } else {
        snprintf(cmd, sizeof(cmd),
                 "git clone --depth 1 %s \"%s\" 2>&1",
                 src->url, dest);
    }
    return system(cmd);
}

static int download_curl(const ParsedSource* src, const char* dest) {
    char zip_path[512], url[512], cmd[1024];
    snprintf(zip_path, sizeof(zip_path), "%s.zip", dest);

    if (src->version[0]) {
        snprintf(url, sizeof(url),
                 "https://github.com/%s/%s/archive/refs/tags/%s.zip",
                 src->user, src->repo, src->version);
    } else {
        snprintf(url, sizeof(url),
                 "https://github.com/%s/%s/archive/refs/heads/main.zip",
                 src->user, src->repo);
    }

    snprintf(cmd, sizeof(cmd), "curl -L -sf -o \"%s\" \"%s\" 2>&1", zip_path, url);
    if (system(cmd) != 0) {
        if (!src->version[0]) {
            snprintf(url, sizeof(url),
                     "https://github.com/%s/%s/archive/refs/heads/master.zip",
                     src->user, src->repo);
            snprintf(cmd, sizeof(cmd), "curl -L -sf -o \"%s\" \"%s\" 2>&1", zip_path, url);
            if (system(cmd) != 0) { remove(zip_path); return -1; }
        } else { remove(zip_path); return -1; }
    }

    mkdir(dest, 0755);
#ifdef _WIN32
    snprintf(cmd, sizeof(cmd),
             "tar -xf \"%s\" --strip-components=1 -C \"%s\" 2>&1", zip_path, dest);
#else
    snprintf(cmd, sizeof(cmd),
             "unzip -qo \"%s\" -d \"%s_tmp\" 2>&1 && "
             "mv \"%s_tmp\"/*/* \"%s\"/ 2>/dev/null; "
             "mv \"%s_tmp\"/*/.* \"%s\"/ 2>/dev/null; "
             "rm -rf \"%s_tmp\"",
             zip_path, dest, dest, dest, dest, dest, dest);
#endif
    int ret = system(cmd);
    remove(zip_path);
    return ret;
}

/* ------------------------------------------------------------------ */
/*  pm_install — install a single package from source                  */
/* ------------------------------------------------------------------ */

int pm_install(const char* source) {
    ParsedSource src;
    if (parse_source(source, &src) != 0) return 1;

    if (!dir_exists(MODULES_DIR)) mkdir(MODULES_DIR, 0755);

    char dest[512];
    snprintf(dest, sizeof(dest), "%s%c%s", MODULES_DIR, PATH_SEP, src.repo);

    if (dir_exists(dest)) {
        printf("Package '%s' already installed\n", src.repo);
        return 0;
    }

    printf("Installing %s/%s...\n", src.user, src.repo);
    int ok = download_git(&src, dest);
    if (ok != 0) {
        printf("  git clone failed, trying curl...\n");
        ok = download_curl(&src, dest);
    }
    if (ok != 0) {
        fprintf(stderr, "Error: failed to download '%s'\n", source);
        fprintf(stderr, "  Make sure 'git' or 'curl' is installed and the repo exists\n");
        return 1;
    }

    Manifest m;
    if (manifest_read(&m) != 0) {
        memset(&m, 0, sizeof(m));
        strcpy(m.name, "my-project");
        strcpy(m.version, "0.1.0");
    }

    int found = 0;
    for (int i = 0; i < m.dep_count; i++) {
        if (strcmp(m.deps[i].name, src.repo) == 0) {
            snprintf(m.deps[i].source, MAX_SOURCE, "%s", source);
            found = 1; break;
        }
    }
    if (!found && m.dep_count < MAX_DEPS) {
        Dependency* d = &m.deps[m.dep_count++];
        snprintf(d->name, MAX_NAME, "%s", src.repo);
        snprintf(d->source, MAX_SOURCE, "%s", source);
    }

    manifest_write(&m);
    printf("Installed '%s' -> %s\n", src.repo, dest);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  pm_install_all — install all dependencies from melody.json         */
/* ------------------------------------------------------------------ */

int pm_install_all(void) {
    Manifest m;
    if (manifest_read(&m) != 0) {
        fprintf(stderr, "Error: no melody.json found (run 'melody init' first)\n");
        return 1;
    }
    if (m.dep_count == 0) {
        printf("No dependencies to install\n");
        return 0;
    }
    if (!dir_exists(MODULES_DIR)) mkdir(MODULES_DIR, 0755);

    int errors = 0;
    for (int i = 0; i < m.dep_count; i++) {
        char dest[512];
        snprintf(dest, sizeof(dest), "%s%c%s", MODULES_DIR, PATH_SEP, m.deps[i].name);
        if (dir_exists(dest)) {
            printf("  %s already installed\n", m.deps[i].name);
            continue;
        }
        if (pm_install(m.deps[i].source) != 0)
            errors++;
    }
    printf("\nInstalled %d/%d packages%s\n",
           m.dep_count - errors, m.dep_count,
           errors ? " (some failed)" : "");
    return errors > 0 ? 1 : 0;
}

/* ------------------------------------------------------------------ */
/*  pm_uninstall — remove a package and update melody.json             */
/* ------------------------------------------------------------------ */

int pm_uninstall(const char* name) {
    char dest[512];
    snprintf(dest, sizeof(dest), "%s%c%s", MODULES_DIR, PATH_SEP, name);

    if (!dir_exists(dest)) {
        fprintf(stderr, "Error: package '%s' is not installed\n", name);
        return 1;
    }

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "%s\"%s\"", RMDIR_CMD, dest);
    if (system(cmd) != 0) {
        fprintf(stderr, "Error: failed to remove '%s'\n", dest);
        return 1;
    }

    Manifest m;
    if (manifest_read(&m) == 0) {
        int idx = -1;
        for (int i = 0; i < m.dep_count; i++) {
            if (strcmp(m.deps[i].name, name) == 0) { idx = i; break; }
        }
        if (idx >= 0) {
            for (int i = idx; i < m.dep_count - 1; i++)
                m.deps[i] = m.deps[i + 1];
            m.dep_count--;
            manifest_write(&m);
        }
    }

    printf("Uninstalled '%s'\n", name);
    return 0;
}
