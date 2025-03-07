/*
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License"); 
** you may not use this file except in compliance with the License. 
** You may obtain a copy of the License at 
**
**     http://www.apache.org/licenses/LICENSE-2.0 
**
** Unless required by applicable law or agreed to in writing, software 
** distributed under the License is distributed on an "AS IS" BASIS, 
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
** See the License for the specific language governing permissions and 
** limitations under the License.
*/

#include <sys/capability.h>
#include <sys/prctl.h>
#include <selinux/android.h>
#include <selinux/avc.h>

#include "installd.h"


#define BUFFER_MAX    1024  /* input buffer for commands */
#define TOKEN_MAX     8     /* max number of arguments in buffer */
#define REPLY_MAX     256   /* largest reply allowed */

static int do_ping(char **arg, char reply[REPLY_MAX])
{
    return 0;
}

static int do_install(char **arg, char reply[REPLY_MAX])
{
    return install(arg[0], atoi(arg[1]), atoi(arg[2]), arg[3]); /* pkgname, uid, gid, seinfo */
}

static int do_dexopt(char **arg, char reply[REPLY_MAX])
{
    /* apk_path, uid, is_public, pkgname, instruction_set, vm_safe_mode, should_relocate */
    return dexopt(arg[0], atoi(arg[1]), atoi(arg[2]), arg[3], arg[4], atoi(arg[5]), 0);
}

static int do_mark_boot_complete(char **arg, char reply[REPLY_MAX])
{
    return mark_boot_complete(arg[0] /* instruction set */);
}

static int do_move_dex(char **arg, char reply[REPLY_MAX])
{
    return move_dex(arg[0], arg[1], arg[2]); /* src, dst, instruction_set */
}

static int do_rm_dex(char **arg, char reply[REPLY_MAX])
{
    return rm_dex(arg[0], arg[1]); /* pkgname, instruction_set */
}

static int do_remove(char **arg, char reply[REPLY_MAX])
{
    return uninstall(arg[0], atoi(arg[1])); /* pkgname, userid */
}

static int do_rename(char **arg, char reply[REPLY_MAX])
{
    return renamepkg(arg[0], arg[1]); /* oldpkgname, newpkgname */
}

static int do_fixuid(char **arg, char reply[REPLY_MAX])
{
    return fix_uid(arg[0], atoi(arg[1]), atoi(arg[2])); /* pkgname, uid, gid */
}

static int do_free_cache(char **arg, char reply[REPLY_MAX]) /* TODO int:free_size */
{
    return free_cache((int64_t)atoll(arg[0])); /* free_size */
}

static int do_rm_cache(char **arg, char reply[REPLY_MAX])
{
    return delete_cache(arg[0], atoi(arg[1])); /* pkgname, userid */
}

static int do_rm_code_cache(char **arg, char reply[REPLY_MAX])
{
    return delete_code_cache(arg[0], atoi(arg[1])); /* pkgname, userid */
}

static int do_get_size(char **arg, char reply[REPLY_MAX])
{
    int64_t codesize = 0;
    int64_t datasize = 0;
    int64_t cachesize = 0;
    int64_t asecsize = 0;
    int res = 0;

        /* pkgdir, userid, apkpath */
    res = get_size(arg[0], atoi(arg[1]), arg[2], arg[3], arg[4], arg[5],
            arg[6], &codesize, &datasize, &cachesize, &asecsize);

    /*
     * Each int64_t can take up 22 characters printed out. Make sure it
     * doesn't go over REPLY_MAX in the future.
     */
    snprintf(reply, REPLY_MAX, "%" PRId64 " %" PRId64 " %" PRId64 " %" PRId64,
            codesize, datasize, cachesize, asecsize);
    return res;
}

static int do_rm_user_data(char **arg, char reply[REPLY_MAX])
{
    return delete_user_data(arg[0], atoi(arg[1])); /* pkgname, userid */
}

static int do_mk_user_data(char **arg, char reply[REPLY_MAX])
{
    return make_user_data(arg[0], atoi(arg[1]), atoi(arg[2]), arg[3]);
                             /* pkgname, uid, userid, seinfo */
}

static int do_mk_user_config(char **arg, char reply[REPLY_MAX])
{
    return make_user_config(atoi(arg[0])); /* userid */
}

static int do_rm_user(char **arg, char reply[REPLY_MAX])
{
    return delete_user(atoi(arg[0])); /* userid */
}

static int do_movefiles(char **arg, char reply[REPLY_MAX])
{
    return movefiles();
}

static int do_linklib(char **arg, char reply[REPLY_MAX])
{
    return linklib(arg[0], arg[1], atoi(arg[2]));
}

static int do_idmap(char **arg, char reply[REPLY_MAX])
{
    return idmap(arg[0], arg[1], atoi(arg[2]));
}

static int do_restorecon_data(char **arg, char reply[REPLY_MAX] __attribute__((unused)))
{
    return restorecon_data(arg[0], arg[1], atoi(arg[2]));
                             /* pkgName, seinfo, uid*/
}

static int do_patchoat(char **arg, char reply[REPLY_MAX]) {
    /* apk_path, uid, is_public, pkgname, instruction_set, vm_safe_mode, should_relocate */
    return dexopt(arg[0], atoi(arg[1]), atoi(arg[2]), arg[3], arg[4], 0, 1);
}

struct cmdinfo {
    const char *name;
    unsigned numargs;
    int (*func)(char **arg, char reply[REPLY_MAX]);
};

struct cmdinfo cmds[] = {
    { "ping",                 0, do_ping },
    { "install",              4, do_install },
    { "dexopt",               6, do_dexopt },
    { "markbootcomplete",     1, do_mark_boot_complete },
    { "movedex",              3, do_move_dex },
    { "rmdex",                2, do_rm_dex },
    { "remove",               2, do_remove },
    { "rename",               2, do_rename },
    { "fixuid",               3, do_fixuid },
    { "freecache",            1, do_free_cache },
    { "rmcache",              2, do_rm_cache },
    { "rmcodecache",          2, do_rm_code_cache },
    { "getsize",              7, do_get_size },
    { "rmuserdata",           2, do_rm_user_data },
    { "movefiles",            0, do_movefiles },
    { "linklib",              3, do_linklib },
    { "mkuserdata",           4, do_mk_user_data },
    { "mkuserconfig",         1, do_mk_user_config },
    { "rmuser",               1, do_rm_user },
    { "idmap",                3, do_idmap },
    { "restorecondata",       3, do_restorecon_data },
    { "patchoat",             5, do_patchoat },
};

static int readx(int s, void *_buf, int count)
{
    char *buf = _buf;
    int n = 0, r;
    if (count < 0) return -1;
    while (n < count) {
        r = read(s, buf + n, count - n);
        if (r < 0) {
            if (errno == EINTR) continue;
            ALOGE("read error: %s\n", strerror(errno));
            return -1;
        }
        if (r == 0) {
            ALOGE("eof\n");
            return -1; /* EOF */
        }
        n += r;
    }
    return 0;
}

static int writex(int s, const void *_buf, int count)
{
    const char *buf = _buf;
    int n = 0, r;
    if (count < 0) return -1;
    while (n < count) {
        r = write(s, buf + n, count - n);
        if (r < 0) {
            if (errno == EINTR) continue;
            ALOGE("write error: %s\n", strerror(errno));
            return -1;
        }
        n += r;
    }
    return 0;
}


/* Tokenize the command buffer, locate a matching command,
 * ensure that the required number of arguments are provided,
 * call the function(), return the result.
 */
static int execute(int s, char cmd[BUFFER_MAX])
{
    char reply[REPLY_MAX];
    char *arg[TOKEN_MAX+1];
    unsigned i;
    unsigned n = 0;
    unsigned short count;
    int ret = -1;

    // ALOGI("execute('%s')\n", cmd);

        /* default reply is "" */
    reply[0] = 0;

        /* n is number of args (not counting arg[0]) */
    arg[0] = cmd;
    while (*cmd) {
        if (isspace(*cmd)) {
            *cmd++ = 0;
            n++;
            arg[n] = cmd;
            if (n == TOKEN_MAX) {
                ALOGE("too many arguments\n");
                goto done;
            }
        }
        cmd++;
    }

    for (i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        if (!strcmp(cmds[i].name,arg[0])) {
            if (n != cmds[i].numargs) {
                ALOGE("%s requires %d arguments (%d given)\n",
                     cmds[i].name, cmds[i].numargs, n);
            } else {
                ret = cmds[i].func(arg + 1, reply);
            }
            goto done;
        }
    }
    ALOGE("unsupported command '%s'\n", arg[0]);

done:
    if (reply[0]) {
        n = snprintf(cmd, BUFFER_MAX, "%d %s", ret, reply);
    } else {
        n = snprintf(cmd, BUFFER_MAX, "%d", ret);
    }
    if (n > BUFFER_MAX) n = BUFFER_MAX;
    count = n;

    // ALOGI("reply: '%s'\n", cmd);
    if (writex(s, &count, sizeof(count))) return -1;
    if (writex(s, cmd, count)) return -1;
    return 0;
}

/**
 * Initialize all the global variables that are used elsewhere. Returns 0 upon
 * success and -1 on error.
 */
void free_globals() {
    size_t i;

    for (i = 0; i < android_system_dirs.count; i++) {
        if (android_system_dirs.dirs[i].path != NULL) {
            free(android_system_dirs.dirs[i].path);
        }
    }

    free(android_system_dirs.dirs);
}

int initialize_globals() {
    // Get the android data directory.
    if (get_path_from_env(&android_data_dir, "ANDROID_DATA") < 0) {
        return -1;
    }

    // Get the android app directory.
    if (copy_and_append(&android_app_dir, &android_data_dir, APP_SUBDIR) < 0) {
        return -1;
    }

    // Get the android protected app directory.
    if (copy_and_append(&android_app_private_dir, &android_data_dir, PRIVATE_APP_SUBDIR) < 0) {
        return -1;
    }

    // Get the android app native library directory.
    if (copy_and_append(&android_app_lib_dir, &android_data_dir, APP_LIB_SUBDIR) < 0) {
        return -1;
    }

    // Get the sd-card ASEC mount point.
    if (get_path_from_env(&android_asec_dir, "ASEC_MOUNTPOINT") < 0) {
        return -1;
    }

    // Get the android media directory.
    if (copy_and_append(&android_media_dir, &android_data_dir, MEDIA_SUBDIR) < 0) {
        return -1;
    }

    // Take note of the system and vendor directories.
    android_system_dirs.count = 4;

    android_system_dirs.dirs = calloc(android_system_dirs.count, sizeof(dir_rec_t));
    if (android_system_dirs.dirs == NULL) {
        ALOGE("Couldn't allocate array for dirs; aborting\n");
        return -1;
    }

    dir_rec_t android_root_dir;
    if (get_path_from_env(&android_root_dir, "ANDROID_ROOT") < 0) {
        ALOGE("Missing ANDROID_ROOT; aborting\n");
        return -1;
    }

    android_system_dirs.dirs[0].path = build_string2(android_root_dir.path, APP_SUBDIR);
    android_system_dirs.dirs[0].len = strlen(android_system_dirs.dirs[0].path);

    android_system_dirs.dirs[1].path = build_string2(android_root_dir.path, PRIV_APP_SUBDIR);
    android_system_dirs.dirs[1].len = strlen(android_system_dirs.dirs[1].path);

    android_system_dirs.dirs[2].path = "/vendor/app/";
    android_system_dirs.dirs[2].len = strlen(android_system_dirs.dirs[2].path);

    android_system_dirs.dirs[3].path = "/oem/app/";
    android_system_dirs.dirs[3].len = strlen(android_system_dirs.dirs[3].path);

    return 0;
}

int initialize_directories() {
    int res = -1;

    // Read current filesystem layout version to handle upgrade paths
    char version_path[PATH_MAX];
    snprintf(version_path, PATH_MAX, "%s.layout_version", android_data_dir.path);

    int oldVersion;
    if (fs_read_atomic_int(version_path, &oldVersion) == -1) {
        oldVersion = 0;
    }
    int version = oldVersion;

    // /data/user
    char *user_data_dir = build_string2(android_data_dir.path, SECONDARY_USER_PREFIX);
    // /data/data
    char *legacy_data_dir = build_string2(android_data_dir.path, PRIMARY_USER_PREFIX);
    // /data/user/0
    char *primary_data_dir = build_string3(android_data_dir.path, SECONDARY_USER_PREFIX, "0");
    if (!user_data_dir || !legacy_data_dir || !primary_data_dir) {
        res = -11;
        goto fail;
    }

    // Make the /data/user directory if necessary
    if (access(user_data_dir, R_OK) < 0) {
#ifndef MY_DEBUG_ROOT   //## sheng
        if (mkdir(user_data_dir, 0711) < 0) {
            res = -2;
            goto fail;
        }
        if (chown(user_data_dir, AID_SYSTEM, AID_SYSTEM) < 0) {
            res = -3;
            goto fail;
        }
        if (chmod(user_data_dir, 0711) < 0) {
            res = -4;
            goto fail;
        }
#else
        if (mkdir(user_data_dir, 0777) < 0) {
            res = -77;
            goto fail;
        }
#endif
    }
    // Make the /data/user/0 symlink to /data/data if necessary
    if (access(primary_data_dir, R_OK) < 0) {
        if (symlink(legacy_data_dir, primary_data_dir)) {
            res = -18;
            goto fail;
        }
    }

    if (version == 0) {
        // Introducing multi-user, so migrate /data/media contents into /data/media/0
        ALOGD("Upgrading /data/media for multi-user");

#ifndef MY_DEBUG_ROOT
        // Ensure /data/media
        if (fs_prepare_dir(android_media_dir.path, 0770, AID_MEDIA_RW, AID_MEDIA_RW) == -1) {
#else
        if (fs_prepare_dir(android_media_dir.path, 0777, 2000, 2000) == -1) {
#endif
            res = -5;
            goto fail;
        }

        // /data/media.tmp
        char media_tmp_dir[PATH_MAX];
        snprintf(media_tmp_dir, PATH_MAX, "%smedia.tmp", android_data_dir.path);

        // Only copy when upgrade not already in progress
        if (access(media_tmp_dir, F_OK) == -1) {
            if (rename(android_media_dir.path, media_tmp_dir) == -1) {
                ALOGE("Failed to move legacy media path: %s", strerror(errno));
                goto fail;
            }
        }
#ifndef MY_DEBUG_ROOT   //## SHENG
        // Create /data/media again
        if (fs_prepare_dir(android_media_dir.path, 0770, AID_MEDIA_RW, AID_MEDIA_RW) == -1) {
            res = -6;
            goto fail;
        }
#else
        // Create /data/media again
        if (fs_prepare_dir(android_media_dir.path, 0777, 2000, 2000) == -1) {
            res = -12;
            goto fail;
        }
#endif
        if (selinux_android_restorecon(android_media_dir.path, 0)) {
            res = -7;
            goto fail;
        }

        // /data/media/0
        char owner_media_dir[PATH_MAX];
        snprintf(owner_media_dir, PATH_MAX, "%s0", android_media_dir.path);

        // Move any owner data into place
        if (access(media_tmp_dir, F_OK) == 0) {
            if (rename(media_tmp_dir, owner_media_dir) == -1) {
                ALOGE("Failed to move owner media path: %s", strerror(errno));
            res = -13;
                goto fail;
            }
        }

        // Ensure media directories for any existing users
        DIR *dir;
        struct dirent *dirent;
        char user_media_dir[PATH_MAX];

        dir = opendir(user_data_dir);
        if (dir != NULL) {
            while ((dirent = readdir(dir))) {
                if (dirent->d_type == DT_DIR) {
                    const char *name = dirent->d_name;

                    // skip "." and ".."
                    if (name[0] == '.') {
                        if (name[1] == 0) continue;
                        if ((name[1] == '.') && (name[2] == 0)) continue;
                    }

                    // /data/media/<user_id>
                    snprintf(user_media_dir, PATH_MAX, "%s%s", android_media_dir.path, name);
#ifndef MY_DEBUG_ROOT
                    if (fs_prepare_dir(user_media_dir, 0770, AID_MEDIA_RW, AID_MEDIA_RW) == -1) {
#else
                    if (fs_prepare_dir(user_media_dir, 0777, 2000, 2000) == -1) {
#endif
                        res = -8;
                        goto fail;
                    }
                }
            }
            closedir(dir);
        }

        version = 1;
    }

    // /data/media/obb
    char media_obb_dir[PATH_MAX];
    snprintf(media_obb_dir, PATH_MAX, "%sobb", android_media_dir.path);

    if (version == 1) {
        // Introducing /data/media/obb for sharing OBB across users; migrate
        // any existing OBB files from owner.
        ALOGD("Upgrading to shared /data/media/obb");

        // /data/media/0/Android/obb
        char owner_obb_path[PATH_MAX];
        snprintf(owner_obb_path, PATH_MAX, "%s0/Android/obb", android_media_dir.path);

        // Only move if target doesn't already exist
        if (access(media_obb_dir, F_OK) != 0 && access(owner_obb_path, F_OK) == 0) {
            if (rename(owner_obb_path, media_obb_dir) == -1) {
                ALOGE("Failed to move OBB from owner: %s", strerror(errno));
            res = -14;
                goto fail;
            }
        }

        version = 2;
    }

    if (ensure_media_user_dirs(0) == -1) {
        ALOGE("Failed to setup media for user 0");
            res = -15;
        goto fail;
    }
#ifndef MY_DEBUG_ROOT
    if (fs_prepare_dir(media_obb_dir, 0770, AID_MEDIA_RW, AID_MEDIA_RW) == -1) {
#else
    if (fs_prepare_dir(media_obb_dir, 0777, 2000, 2000) == -1) {
#endif
        res = -9;
        goto fail;
    }

    if (ensure_config_user_dirs(0) == -1) {
        ALOGE("Failed to setup misc for user 0");
            res = -16;
        goto fail;
    }

    if (version == 2) {
        ALOGD("Upgrading to /data/misc/user directories");

        char misc_dir[PATH_MAX];
        snprintf(misc_dir, PATH_MAX, "%smisc", android_data_dir.path);

        char keychain_added_dir[PATH_MAX];
        snprintf(keychain_added_dir, PATH_MAX, "%s/keychain/cacerts-added", misc_dir);

        char keychain_removed_dir[PATH_MAX];
        snprintf(keychain_removed_dir, PATH_MAX, "%s/keychain/cacerts-removed", misc_dir);

        DIR *dir;
        struct dirent *dirent;
        dir = opendir(user_data_dir);
        if (dir != NULL) {
            while ((dirent = readdir(dir))) {
                const char *name = dirent->d_name;

                // skip "." and ".."
                if (name[0] == '.') {
                    if (name[1] == 0) continue;
                    if ((name[1] == '.') && (name[2] == 0)) continue;
                }

                uint32_t user_id = atoi(name);

                // /data/misc/user/<user_id>
                if (ensure_config_user_dirs(user_id) == -1) {
                    res = -10;
                    goto fail;
                }

                char misc_added_dir[PATH_MAX];
                snprintf(misc_added_dir, PATH_MAX, "%s/user/%s/cacerts-added", misc_dir, name);

                char misc_removed_dir[PATH_MAX];
                snprintf(misc_removed_dir, PATH_MAX, "%s/user/%s/cacerts-removed", misc_dir, name);

#ifndef MY_DEBUG_ROOT
                uid_t uid = multiuser_get_uid(user_id, AID_SYSTEM);
#else
                uid_t uid = multiuser_get_uid(user_id, 2000);
#endif
                gid_t gid = uid;
                if (access(keychain_added_dir, F_OK) == 0) {
                    if (copy_dir_files(keychain_added_dir, misc_added_dir, uid, gid) != 0) {
                        ALOGE("Some files failed to copy");
                    }
                }
                if (access(keychain_removed_dir, F_OK) == 0) {
                    if (copy_dir_files(keychain_removed_dir, misc_removed_dir, uid, gid) != 0) {
                        ALOGE("Some files failed to copy");
                    }
                }
            }
            closedir(dir);

            if (access(keychain_added_dir, F_OK) == 0) {
                delete_dir_contents(keychain_added_dir, 1, 0);
            }
            if (access(keychain_removed_dir, F_OK) == 0) {
                delete_dir_contents(keychain_removed_dir, 1, 0);
            }
        }

        version = 3;
    }

    // Persist layout version if changed
    if (version != oldVersion) {
        if (fs_write_atomic_int(version_path, version) == -1) {
            ALOGE("Failed to save version to %s: %s", version_path, strerror(errno));
            res = -17;
            goto fail;
        }
    }

    // Success!
    res = 0;

fail:
    free(user_data_dir);
    free(legacy_data_dir);
    free(primary_data_dir);
    return res;
}

static void drop_privileges() {
#ifndef MY_DEBUG_ROOT   //## SHENG 降权
    if (prctl(PR_SET_KEEPCAPS, 1) < 0) {
        ALOGE("prctl(PR_SET_KEEPCAPS) failed: %s\n", strerror(errno));
        exit(1);
    }

    if (setgid(AID_INSTALL) < 0) {
        ALOGE("setgid() can't drop privileges; exiting.\n");
        exit(1);
    }

    if (setuid(AID_INSTALL) < 0) {
        ALOGE("setuid() can't drop privileges; exiting.\n");
        exit(1);
    }

    struct __user_cap_header_struct capheader;
    struct __user_cap_data_struct capdata[2];
    memset(&capheader, 0, sizeof(capheader));
    memset(&capdata, 0, sizeof(capdata));
    capheader.version = _LINUX_CAPABILITY_VERSION_3;
    capheader.pid = 0;

    capdata[CAP_TO_INDEX(CAP_DAC_OVERRIDE)].permitted |= CAP_TO_MASK(CAP_DAC_OVERRIDE);
    capdata[CAP_TO_INDEX(CAP_CHOWN)].permitted        |= CAP_TO_MASK(CAP_CHOWN);
    capdata[CAP_TO_INDEX(CAP_SETUID)].permitted       |= CAP_TO_MASK(CAP_SETUID);
    capdata[CAP_TO_INDEX(CAP_SETGID)].permitted       |= CAP_TO_MASK(CAP_SETGID);
    capdata[CAP_TO_INDEX(CAP_FOWNER)].permitted       |= CAP_TO_MASK(CAP_FOWNER);

    capdata[0].effective = capdata[0].permitted;
    capdata[1].effective = capdata[1].permitted;
    capdata[0].inheritable = 0;
    capdata[1].inheritable = 0;

    if (capset(&capheader, &capdata[0]) < 0) {
        ALOGE("capset failed: %s\n", strerror(errno));
        exit(1);
    }
#endif
}

static int log_callback(int type, const char *fmt, ...) {
    va_list ap;
    int priority;

    switch (type) {
    case SELINUX_WARNING:
        priority = ANDROID_LOG_WARN;
        break;
    case SELINUX_INFO:
        priority = ANDROID_LOG_INFO;
        break;
    default:
        priority = ANDROID_LOG_ERROR;
        break;
    }
    va_start(ap, fmt);
    LOG_PRI_VA(priority, "SELinux", fmt, ap);
    va_end(ap);
    return 0;
}

int main(const int argc, const char *argv[]) {
    char buf[BUFFER_MAX];
    struct sockaddr addr;
    socklen_t alen;
    int lsocket, s, count;
    int selinux_enabled = (is_selinux_enabled() > 0);

    ALOGI("installd firing up\n");

    union selinux_callback cb;
    cb.func_log = log_callback;
    selinux_set_callback(SELINUX_CB_LOG, cb);

    if (initialize_globals() < 0) {
        ALOGE("Could not initialize globals; exiting.\n");
        exit(1);
    }
    int my_debug = initialize_directories();
    if (my_debug < 0) {
        ALOGE("Could not create directories; exiting.2 %d\n", my_debug);
        exit(1);
    }

    if (selinux_enabled && selinux_status_open(true) < 0) {
        ALOGE("Could not open selinux status; exiting.\n");
        exit(1);
    }

    drop_privileges();

    lsocket = android_get_control_socket(SOCKET_PATH);
    if (lsocket < 0) {
        ALOGE("Failed to get socket from environment: %s\n", strerror(errno));
        exit(1);
    }
    if (listen(lsocket, 5)) {
        ALOGE("Listen on socket failed: %s\n", strerror(errno));
        exit(1);
    }
    fcntl(lsocket, F_SETFD, FD_CLOEXEC);

    for (;;) {
        alen = sizeof(addr);
        s = accept(lsocket, &addr, &alen);
        if (s < 0) {
            ALOGE("Accept failed: %s\n", strerror(errno));
            continue;
        }
        fcntl(s, F_SETFD, FD_CLOEXEC);

        ALOGI("new connection\n");
        for (;;) {
            unsigned short count;
            if (readx(s, &count, sizeof(count))) {
                ALOGE("failed to read size\n");
                break;
            }
            if ((count < 1) || (count >= BUFFER_MAX)) {
                ALOGE("invalid size %d\n", count);
                break;
            }
            if (readx(s, buf, count)) {
                ALOGE("failed to read command\n");
                break;
            }
            buf[count] = 0;
            if (selinux_enabled && selinux_status_updated() > 0) {
                selinux_android_seapp_context_reload();
            }
            if (execute(s, buf)) break;
        }
        ALOGI("closing connection\n");
        close(s);
    }

    return 0;
}
