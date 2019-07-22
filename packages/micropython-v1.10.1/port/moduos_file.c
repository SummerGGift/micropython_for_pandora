/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 SummerGift <zhangyuan@rt-thread.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "py/mpconfig.h"

#if MICROPY_PY_MODUOS_FILE

#include <stdint.h>
#include <string.h>
#include <dfs_posix.h>

#include "py/runtime.h"
#include "py/objstr.h"
#include "py/mperrno.h"
#include "moduos_file.h"


mp_obj_t mp_posix_mount(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_posix_mount_obj, 2, mp_posix_mount);

mp_obj_t mp_posix_umount(mp_obj_t mnt_in) {

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(mp_posix_umount_obj, mp_posix_umount);

mp_obj_t mp_posix_chdir(mp_obj_t path_in) {
    const char *changepath = mp_obj_str_get_str(path_in);
    if (chdir(changepath) != 0) {
        rt_kprintf("No such directory: %s\n", changepath);
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(mp_posix_chdir_obj, mp_posix_chdir);

mp_obj_t mp_posix_getcwd(void) {
    char buf[MICROPY_ALLOC_PATH_MAX + 1];
    getcwd(buf, sizeof(buf));
    return mp_obj_new_str(buf, strlen(buf));
}
MP_DEFINE_CONST_FUN_OBJ_0(mp_posix_getcwd_obj, mp_posix_getcwd);

#include <dfs_file.h>
static struct dfs_fd fd;
static struct dirent dirent;
mp_obj_t mp_posix_listdir(size_t n_args, const mp_obj_t *args) {

    mp_obj_t dir_list = mp_obj_new_list(0, NULL);

    struct stat stat;
    int length;
    char *fullpath, *path;
    const char *pathname;

    if (n_args == 0) {
#ifdef DFS_USING_WORKDIR
        extern char working_directory[];
        pathname = working_directory;
#else
        pathname = "/";
#endif
    } else {
       pathname = mp_obj_str_get_str(args[0]);
    }

    fullpath = NULL;
    if (pathname == NULL)
    {
#ifdef DFS_USING_WORKDIR
        extern char working_directory[];
        /* open current working directory */
        path = rt_strdup(working_directory);
#else
        path = rt_strdup("/");
#endif
        if (path == NULL)
            mp_raise_OSError(MP_ENOMEM); /* out of memory */
    }
    else
    {
        path = (char *)pathname;
    }

    /* list directory */
    if (dfs_file_open(&fd, path, O_DIRECTORY) == 0)
    {
        do {
            memset(&dirent, 0, sizeof(struct dirent));
            length = dfs_file_getdents(&fd, &dirent, sizeof(struct dirent));
            if (length > 0) {
                memset(&stat, 0, sizeof(struct stat));

                /* build full path for each file */
                fullpath = dfs_normalize_path(path, dirent.d_name);
                if (fullpath == NULL)
                    break;

                if (dfs_file_stat(fullpath, &stat) == 0) {
                    mp_obj_tuple_t *t = MP_OBJ_TO_PTR(mp_obj_new_tuple(3, NULL));
                    t->items[0] = mp_obj_new_str(dirent.d_name, strlen(dirent.d_name));
                    t->items[1] = MP_OBJ_NEW_SMALL_INT(MP_S_IFDIR);
                    t->items[2] = MP_OBJ_NEW_SMALL_INT(0); // no inode number
                    mp_obj_t next = MP_OBJ_FROM_PTR(t);
                    mp_obj_t *items;
                    mp_obj_get_array_fixed_n(next, 3, &items);
                    mp_obj_list_append(dir_list, items[0]);
                } else {
                    rt_kprintf("BAD file: %s\n", dirent.d_name);
                }
                rt_free(fullpath);
            }
        } while (length > 0);

        dfs_file_close(&fd);
    }
    else
    {
//        rt_kprintf("No such directory\n");
    }
    if (pathname == NULL)
        rt_free(path);

    return dir_list;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_posix_listdir_obj, 0, 1, mp_posix_listdir);

mp_obj_t mp_posix_mkdir(mp_obj_t path_in) {
    const char *createpath = mp_obj_str_get_str(path_in);
    int res = mkdir(createpath, 0);
    if (res != 0) {
        mp_raise_OSError(MP_EEXIST);
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(mp_posix_mkdir_obj, mp_posix_mkdir);

mp_obj_t mp_posix_remove(uint n_args, const mp_obj_t *arg) {
    int index;
    if (n_args == 0) {
        rt_kprintf("Usage: rm FILE...\n");
        rt_kprintf("Remove (unlink) the FILE(s).\n");
        return mp_const_none;
    }
    for (index = 0; index < n_args; index++) {
        //rt_kprintf("Remove %s.\n", mp_obj_str_get_str(arg[index]));
        unlink(mp_obj_str_get_str(arg[index]));
    }
    // TODO  recursive deletion
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR(mp_posix_remove_obj, 0, mp_posix_remove);

mp_obj_t mp_posix_rename(mp_obj_t old_path_in, mp_obj_t new_path_in) {
    const char *old_path = mp_obj_str_get_str(old_path_in);
    const char *new_path = mp_obj_str_get_str(new_path_in);
    int res = rename(old_path, new_path);
    if (res != 0) {
        mp_raise_OSError(MP_EPERM);
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(mp_posix_rename_obj, mp_posix_rename);

mp_obj_t mp_posix_rmdir(uint n_args, const mp_obj_t *arg) {
    int index;
    if (n_args == 0) {
        rt_kprintf("Usage: rm FILE...\n");
        rt_kprintf("Remove (unlink) the FILE(s).\n");
        return mp_const_none;
    }
    for (index = 0; index < n_args; index++) {
        //rt_kprintf("Remove %s.\n", mp_obj_str_get_str(arg[index]));
        rmdir(mp_obj_str_get_str(arg[index]));
    }
    // TODO  recursive deletion
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_VAR(mp_posix_rmdir_obj, 0, mp_posix_rmdir);

mp_obj_t mp_posix_stat(mp_obj_t path_in) {
    struct stat buf;
    const char *createpath = mp_obj_str_get_str(path_in);
    int res = stat(createpath, &buf);
    if (res != 0) {
        mp_raise_OSError(MP_EPERM);
    }
    mp_obj_tuple_t *t = MP_OBJ_TO_PTR(mp_obj_new_tuple(10, NULL));
    t->items[0] = MP_OBJ_NEW_SMALL_INT(buf.st_mode); // st_mode
    t->items[1] = MP_OBJ_NEW_SMALL_INT(buf.st_ino); // st_ino
    t->items[2] = MP_OBJ_NEW_SMALL_INT(buf.st_dev); // st_dev
    t->items[3] = MP_OBJ_NEW_SMALL_INT(buf.st_nlink); // st_nlink
    t->items[4] = MP_OBJ_NEW_SMALL_INT(buf.st_uid); // st_uid
    t->items[5] = MP_OBJ_NEW_SMALL_INT(buf.st_gid); // st_gid
    t->items[6] = mp_obj_new_int_from_uint(buf.st_size); // st_size
    t->items[7] = MP_OBJ_NEW_SMALL_INT(buf.st_atime); // st_atime
    t->items[8] = MP_OBJ_NEW_SMALL_INT(buf.st_mtime); // st_mtime
    t->items[9] = MP_OBJ_NEW_SMALL_INT(buf.st_ctime); // st_ctime
    return MP_OBJ_FROM_PTR(t);
}
MP_DEFINE_CONST_FUN_OBJ_1(mp_posix_stat_obj, mp_posix_stat);

#if defined(MICROPY_UOS_ENABLE_FILESYNC)
#include <tiny_md5.h>

static void md5_str(uint8_t md5[16], char str[32])
{
    int i = 0,j = 0;
    uint8_t temp;

    for (i = 0, j = 0; i < 16; i ++, j += 2)
    {
        temp = md5[i] / 16;
        str[j] = temp >= 10 ? (temp - 10) + 'a' : temp + '0';
        temp = md5[i] % 16;
        str[j + 1] = temp >= 10 ? (temp - 10) + 'a' : temp + '0';
    }
}

static void calcmd5(char* spec, void *buffer, int size, uint8_t value[16])
{
    tiny_md5_context c;
    int fd;

    if (buffer == RT_NULL)
    {
        return;
    }
    fd = open(spec, O_RDONLY, 0);
    if (fd < 0)
    {
        return;
    }

    tiny_md5_starts(&c);
    while (1)
    {
        int len = read(fd, buffer, size);
        if (len < 0)
        {
            close(fd);
            return;
        }
        else if (len == 0)
            break;

        tiny_md5_update(&c, (unsigned char*)buffer, len);
    }
    tiny_md5_finish(&c, value);
    close(fd);
}

mp_obj_t mp_posix_file_md5(mp_obj_t path_in) {
    const char *createpath = mp_obj_str_get_str(path_in);
    
    void *md5_buff = malloc(512);
    if (md5_buff == RT_NULL)
    {
        mp_raise_OSError(MP_ENOMEM);
    }

    vstr_t vstr;
    vstr_init_len(&vstr, 16);
    
    uint8_t md5[16];
    char str[33];
    calcmd5((char *)createpath, md5_buff, 512, md5);
    md5_str(md5, str);
    str[32] = '\0';
    free(md5_buff);
    
    return mp_obj_new_str(str, strlen(str));
}
MP_DEFINE_CONST_FUN_OBJ_1(mp_posix_file_md5_obj, mp_posix_file_md5);
#endif

mp_import_stat_t mp_posix_import_stat(const char *path) {

    struct stat stat;

    if (dfs_file_stat(path, &stat) == 0) {
        if (S_ISDIR(stat.st_mode)) {
            return MP_IMPORT_STAT_DIR;
        } else {
            return MP_IMPORT_STAT_FILE;
        }
    } else {
        return MP_IMPORT_STAT_NO_EXIST;
    }
}

#endif //MICROPY_MODUOS_FILE
