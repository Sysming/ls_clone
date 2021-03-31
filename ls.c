#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <pwd.h>
#include <grp.h>
#include <time.h>

#include <dirent.h>
#include <errno.h>

#define MAX_FILES 512

struct file_informations {
    char    name[255];             // name of the file
    char    permissions[11];       // permissions format `-rwxrwxrwx`
    nlink_t nlink;                 // number of hard links
    char    owner[64];             // owner of the file
    char    group[64];             // group name
    off_t   size;                  // size in bytes
    char    lastmodif[32];         // last modification
};

struct files_i {
    struct file_informations file[MAX_FILES];
    size_t count;
    size_t maxsize;
};


int get_file_info(char *filename, char *dir_name, struct file_informations *file_i);
void file_permissions(mode_t mode, struct file_informations *fi);
void push_file_info_to_stack(struct files_i *file_i_array, char *filename, char *dir_name);
void print_all_files(struct files_i *file_i_array);
void print_file_informations(struct file_informations file);
void sort_by_name_alphabetical(struct files_i *files);
int is_in_working_directory(char *filepath);


void file_permissions(mode_t mode, struct file_informations *fi) {
    switch(mode & S_IFMT) {
        case S_IFBLK:   fi->permissions[0] = 'b'; break;
        case S_IFREG:   fi->permissions[0] = '-'; break;
        case S_IFDIR:   fi->permissions[0] = 'd'; break;
        case S_IFCHR:   fi->permissions[0] = 'c'; break;
        case S_IFIFO:   fi->permissions[0] = 'p'; break;
        case S_IFLNK:   fi->permissions[0] = 'l'; break;
        case S_IFSOCK:  fi->permissions[0] = 's'; break;
    }
    fi->permissions[1] = (mode & S_IRUSR) ? 'r': '-';
    fi->permissions[2] = (mode & S_IWUSR) ? 'w': '-';
    fi->permissions[3] = (mode & S_IXUSR) ? 'x': '-';
    fi->permissions[4] = (mode & S_IRGRP) ? 'r': '-';
    fi->permissions[5] = (mode & S_IWGRP) ? 'w': '-';
    fi->permissions[6] = (mode & S_IXGRP) ? 'x': '-';
    fi->permissions[7] = (mode & S_IROTH) ? 'r': '-';
    fi->permissions[8] = (mode & S_IWOTH) ? 'w': '-';
    fi->permissions[9] = (mode & S_IXOTH) ? 'x': '-';
    fi->permissions[10] = '\0';
}

int get_file_info(char *filename, char *dir_name, struct file_informations *file_i) {
    struct stat sb;                         // declare a struct stat buffer

    /* Deal with the filepath */
    char filepath[512];
    strcpy(filepath, dir_name);
    strcat(filepath, "/");
    strcat(filepath, filename);
    if(lstat(filepath, &sb) == -1) {
        perror("lstat");
        return 1;
    }

    strcpy(file_i->name, filename);         // copy the filename
    file_permissions(sb.st_mode, file_i);   // deal with the permissions
    /* File size and hard link number */
    file_i->size = sb.st_size;
    file_i->nlink = sb.st_nlink;

    /* Owner and Group Name */
    struct passwd *pass;
    struct group *grp;
    pass = getpwuid(sb.st_uid);
    grp = getgrgid(sb.st_gid);              //
    strcpy(file_i->owner, pass->pw_name);   // copy the owner name
    strcpy(file_i->group, grp->gr_name);    // copy the group name

    /* Last modification time */
    struct tm lt;                           // declare a struct tm
    time_t t = sb.st_mtime;                 // store the time of last modification
    localtime_r(&t, &lt);                   // converts time_t t to a tm struct
    strftime(file_i->lastmodif, 32, "%b %e %H:%M", &lt);
    return 0;
}


void push_file_info_to_stack(struct files_i *file_i_array, char *filename, char *dir_name) {
    struct file_informations file;
    get_file_info(filename, dir_name, &file);
    file_i_array->file[file_i_array->count] = file;
    file_i_array->count++;
}

void print_all_files(struct files_i *file_i_array) {
    for(size_t i = 0; i < file_i_array->count; i++) {
        print_file_informations(file_i_array->file[i]);
    }
}

void print_file_informations(struct file_informations file) {
    printf("%s %ld %s %s %ld %s %s \n", file.permissions, file.nlink, file.owner, file.group, file.size, file.lastmodif, file.name);
}

void sort_by_name_alphabetical(struct files_i *files) {
    size_t i,j;
    struct file_informations file_temp;
    for(i = 0 ;i < files->count - 1; i++){
        for( j= i+1 ; j < files->count;j++){
            if(strcasecmp(files->file[i].name, files->file[j].name) > 0){
                file_temp = files->file[i];
                files->file[i] = files->file[j];
                files->file[j] = file_temp;
             }
         }
     }
}

int is_in_working_directory(char *filepath) {
    size_t i = 0;
    int is_in_working_directory = 1;
    while(i < strlen(filepath)) {
        if(filepath[i] == '/'){
            is_in_working_directory = 0;
            break;
        }
        i++;
    }
    return is_in_working_directory;
}

int main(int argc, char *argv[]) {
    int i;
    errno = 0;
    struct file_informations file;
    DIR *d;
    struct dirent *dir;
    for(i = 1; i < argc; i++) {
        d = opendir(argv[i]);
        if(d == NULL) {
            if(errno == ENOTDIR) {
                int working_directory = is_in_working_directory(argv[i]);   // check if we are in the working_directory
                if(working_directory)
                    get_file_info(argv[i], ".", &file);
                else
                    get_file_info(argv[i], "", &file);
                print_file_informations(file);
            }
            else
                fprintf(stderr, "lsl: cannot access '%s': No such file or directory\n", argv[i]);
        }
        closedir(d);
    }
    for(i = 1; i < argc; i++) {
        struct files_i fi;
        fi.count = 0;
        d = opendir(argv[i]);
        if (d) {
            while ((dir = readdir(d)) != NULL) {
              if(dir->d_name[0] != '.')
                push_file_info_to_stack(&fi, dir->d_name, argv[i]);
            }
            closedir(d);
            sort_by_name_alphabetical(&fi);
            printf("\n%s:\n", argv[i]);
            print_all_files(&fi);
        }
    }
    if(argc == 1) {
        struct files_i fi;
        fi.count = 0;
        d = opendir(".");
        if (d) {
            while ((dir = readdir(d)) != NULL) {
              if(dir->d_name[0] != '.')
                push_file_info_to_stack(&fi, dir->d_name, ".");
            }
            closedir(d);
            sort_by_name_alphabetical(&fi);
            printf("%s:\n", ".");
            print_all_files(&fi);
        }
    }
    return EXIT_SUCCESS;
 }
