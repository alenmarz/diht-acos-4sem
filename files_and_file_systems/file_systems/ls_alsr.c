#include <dirent.h>
#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#if (defined _GNU_SOURCE || defined __gnu_hurd__ || defined __GNU__ || \
       defined __linux__ || defined __MINGW32__ || defined __GLIBC__)
    #include <bsd/string.h>
#else
    #include <string.h>
#endif

/* Possible values:
 *
 * FAT32 65535
 * NTFS  4294967295
 * ext2  1000000000000000000, bad perfomance if more than 10000
 * ext3  unlimited, ULLONG_MAX in our case
 * ext4  4294967295
 * btrfs 18446744073709551615
 * HSF+  2100000000 (?)
 *
 * FIXME: information sources are wikipedia.org and stackoverflow.com
 * Let us suppose 100000 will be enough (at least it worked for my laptop)
 */
#ifndef MAXIMUM_NUMBER_OF_FILES_PER_DIRECTORY
  #define MAXIMUM_NUMBER_OF_FILES_PER_DIRECTORY 100000
#endif

/*
 * Proper value is (PATH_MAX + 1) but let us hope 1000 as default value
 * is enough
 *
 * You can redefine this constant using gcc -D (clang -D) or equivalent
 * options of your compiller
 */
#ifndef PATH_TO_FILE_ARRAY_SIZE
    #define PATH_TO_FILE_ARRAY_SIZE 1000
#endif


/*
 * Keeps all information about file (or, rather, hardlink)
 */
typedef struct {
  struct stat   file_stat;
  struct dirent file_dirent;
  char          path[PATH_TO_FILE_ARRAY_SIZE];
} file_info_t;


/*
 * Convert inode status information into a symbolic string and prints it
 */
void print_file_mode_string(mode_t file_mode) {
    char buff[12];
    strmode(file_mode, buff);
    printf("%s", buff);
}

/*
 * Evaluetes and prints username from user id
 *
 * Acording to useradd and stackowerflow username size limit is 16 chars
 */
void print_user_name(uid_t file_uid) {
    struct passwd *user_info = getpwuid(file_uid);
    printf("%32s", user_info -> pw_name);
}

/* Evaluetes and prints group name from gid
 *
 * Acording to stackowerflow groupname size limit varray varies between
 * 8 and 16 chars for most popular POSIX systems
 *
 * Let us hope 16 will be anough
 */
void print_group_name(gid_t file_gid) {
    struct group *file_group = getgrgid(file_gid);
    printf("%16s", file_group -> gr_name);
}

/* Prints file least modification time in format:
 *
 * mmm dd yyyy HH:MM
 *
 * It differs from ls -l time format but, I think, it's still OK
 */
void print_modification_time(time_t file_mtime) {
    struct tm *file_mtm = localtime(&file_mtime);
    char time_string[20];
    strftime(time_string, 19, "%b %d %Y %H:%M", file_mtm);
    printf("%17s", time_string);
}

/*
 * Prints the contents of the symbolic link referred to by path
 */
void print_symlink(char *path) {
    char resolved[PATH_TO_FILE_ARRAY_SIZE];
    int resolved_size = readlink(path, resolved, PATH_TO_FILE_ARRAY_SIZE);
    if (resolved_size == -1) return;
    resolved[resolved_size] = '\0';
    printf(" -> %s", resolved);
}

/* Useful function which allows to get file stats (struct stat)
 * from it's dirent record (struct dirent) and directory path
 *
 * @return if getting file stats was successful
 */
int get_file_stat(file_info_t *file_info) {
    if (file_info == NULL) return 0;
    return lstat(file_info -> path, &(file_info -> file_stat)) == 0;
}

/*
 * Prints file info in simlar to ls -l way
 */
void print_file_info(file_info_t file_info) {
    print_file_mode_string(file_info.file_stat.st_mode);
    // maximum number of hard links is 65535, 5 chars will be enough then
    printf(" %5u ", (unsigned int)(file_info.file_stat.st_nlink));
    print_user_name(file_info.file_stat.st_uid);
    printf(" ");
    print_group_name(file_info.file_stat.st_gid);
    // ext4 max file sixe is 16TB so 14 chars will be enough
    printf(" %14llu ", file_info.file_stat.st_size);
    print_modification_time(file_info.file_stat.st_mtime);
    printf(" %s", file_info.file_dirent.d_name);
    print_symlink(file_info.path);
    printf("\n");
}

/*
 * Compares file size of two directory entries (struct dirent *)
 * Sorts in DESC order
 */
int file_size_cmp(const void *f1_info, const void *f2_info) {
    return (((file_info_t *)(f2_info)) -> file_stat).st_size -
           (((file_info_t *)(f1_info)) -> file_stat).st_size;
}

/*
 * Prints information about directory name, total files
 * count (including . and ..), and file_info for eaxh file (see above)
 */
void print_directory_content(char *path)
{
    DIR *directory = opendir(path);
    if (directory == NULL) return;

    file_info_t *infos = (file_info_t *)calloc(
        MAXIMUM_NUMBER_OF_FILES_PER_DIRECTORY,
        sizeof(struct stat)
    );

    size_t infos_cnt = 0;
    struct dirent * directory_entry;
    while ((directory_entry = readdir(directory)) != NULL) {
      infos[infos_cnt].file_dirent = *directory_entry;
      sprintf(infos[infos_cnt].path, "%s/%s", path, directory_entry -> d_name);
      if (!get_file_stat(infos + infos_cnt)) continue;
      infos_cnt++;
    }

    qsort(infos, infos_cnt, sizeof(file_info_t), file_size_cmp);

    printf("%s:\n%lu files total\n", path, infos_cnt);
    for (size_t idx = 0; idx < infos_cnt; idx++)
        print_file_info(infos[idx]);
    printf("\n");

    closedir(directory);

    for (size_t idx = 0; idx < infos_cnt; idx++) {
        if (!S_ISDIR(infos[idx].file_stat.st_mode))           continue;
        if (strcmp(infos[idx].file_dirent.d_name, ".") == 0)  continue;
        if (strcmp(infos[idx].file_dirent.d_name, "..") == 0) continue;
        print_directory_content(infos[idx].path);
    }
    free(infos);
}

int main(int argc, char **argv) {
    if (argc == 1) print_directory_content(".");
    for ( size_t arg_idx = 1; arg_idx < (size_t)argc; arg_idx++)
      print_directory_content(argv[arg_idx]);

    return 0;
}
