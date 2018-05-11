#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

const int MAX_STRING_LENGTH = 1000000;

int num_threads_;
int thread_part_size_;
int strings_number_;
char * filename_;
FILE * file_;
FILE ** files_;

void memory_error() {
    printf("Not enough memory\n");
    exit(1);
}

int cmp_function(const void * a, const void * b) {
    const char *s1, *s2;
    s1 = * (char **)a;
    s2 = * (char **)b;
    return strcmp(s1, s2);
}

char * itoa(int n) {
    char * string = (char *)malloc(11 * sizeof(char));
    if (!string) {
        memory_error();
    }
    sprintf(string, "%d", n);
    return string;
}

void * thread_work(void * _thread_id) {
    
    int thread_id = (int)_thread_id;
    FILE * file = fopen(filename_, "rb");
    files_[thread_id] = fopen(itoa(thread_id), "w+b");
    if (files_[thread_id] == NULL) {
        printf("Creating new file error\n");
        exit(1);
    }

    int first_string_number = thread_id * thread_part_size_;
    int last_string_number = (thread_id == num_threads_ - 1) ? strings_number_ - 1
                                                             : first_string_number + thread_part_size_ - 1;
    int string_number = 0;
    int strings_count = last_string_number - first_string_number + 1;

    char ** strings_array = (char **)malloc(strings_count * sizeof(char *));
    if (!strings_array) {
        memory_error();
    }

    char c;
    while (string_number < first_string_number) {
        if ((c = fgetc(file)) == '\n') {
            string_number++;
        }
    }
    while (string_number <= last_string_number) {
        int index = string_number - first_string_number;
        strings_array[index] = (char *)malloc(MAX_STRING_LENGTH * sizeof(char));
        if (!strings_array[index]) {
            memory_error();
        }
        strings_array[index] = fgets(strings_array[index], MAX_STRING_LENGTH, file);
        if (strings_array[index] == NULL) {
            printf("Reading error\n");
            exit(1);
        }
        string_number++;
    }
    
    fclose(file);

    qsort(strings_array, strings_count, sizeof(char *), cmp_function);
    
    for (int i = 0; i < strings_count; ++i) {
        fputs(strings_array[i], files_[thread_id]);
    }
    
    for (int i = 0; i < strings_count; ++i) {
        free(strings_array[i]);
    }
    
    free(strings_array);

    fclose(files_[thread_id]);
}

// returns NULL when file is empty
char split_text_into_files_and_sort_them() {
    
    FILE * file = fopen(filename_, "rb");
    if (file == NULL) {
        printf("File %s ", filename_, "could not be opened\n");
        return 1;
    }
    
    // let's find the total number of strings
    strings_number_ = 0;
    char c;
    while ((c = fgetc(file)) != EOF) {
        if (c == '\n') {
            ++strings_number_;
        }
    }
    if (strings_number_ == 0) {
        printf("File %s is empty\n", filename_);
        return 1;
    }
    fclose(file);

    thread_part_size_ = strings_number_ / num_threads_;
    if (thread_part_size_ <= 0) {
        num_threads_ = 1;
    }
    
    files_ = (FILE **)malloc(num_threads_ * sizeof(FILE *));
    if (!files_) {
        memory_error();
    }


    pid_t * pids = (pid_t *)calloc(num_threads_, sizeof(pid_t));
    for (int i = 0; i < num_threads_; ++i) {
        pid_t pid = fork();
        if (pid < 0) {
            printf("can't fork new process\n");
            exit(1);
        }
        if (pid == 0) {
            thread_work((void *)i);
            exit(0);
        }
        if (pid > 0) {
            pids[i] = pid;
        }
    }

    for(int i = 0; i < num_threads_; ++i) {
        waitpid(pids[i], NULL, 0);
    }
    free(pids);
    return 0;
}


char * get_new_string(FILE * file) {
   
    char * new_string = (char *)malloc(MAX_STRING_LENGTH * sizeof(char));
    if (!new_string) {
        memory_error();
    }
    if (fgets(new_string, MAX_STRING_LENGTH, file) != NULL) {
 
        int n = strlen(new_string) - 1;
        if (new_string[n] == '\n') {
            new_string[n] = '\0';
            char * _new_string = (char *)malloc((n + 2) * sizeof(char));
            if (!_new_string) {
                memory_error();
            }
            strncpy(_new_string, new_string, n + 1);
            new_string = _new_string;
        }
    } else {
        return NULL;
    }
    return new_string;
}

int find_min_string(char ** strings_array) {
    int min_string_number = -1;
    int cmp_value;
    for (int i = 0; i < num_threads_; ++i) {
        if (min_string_number >= 0 && strings_array[i] != NULL) {
            cmp_value = strcmp(strings_array[i], strings_array[min_string_number]);
        }
        if (
            (min_string_number == -1
            || min_string_number >= 0 && cmp_value < 0) && strings_array[i] != NULL
           ) {
            min_string_number = i;
        }
    }
    return min_string_number;
}

void parent_sort() {
   
    for (int i = 0; i < num_threads_; ++i) {
        files_[i] = fopen(itoa(i), "rb");
    }
    int not_finished_files_number = num_threads_;
    char ** first_file_string = (char **)malloc(num_threads_ * sizeof(char *));
    if (!first_file_string) {
        memory_error();
    }
    for (int i = 0; i < num_threads_; ++i) {
        first_file_string[i] = get_new_string(files_[i]);
    }
    while (not_finished_files_number > 0) {
        int number_of_min_string = find_min_string(first_file_string);
        printf("%s\n", first_file_string[number_of_min_string]);
        first_file_string[number_of_min_string] = get_new_string(files_[number_of_min_string]);
        if (first_file_string[number_of_min_string] == NULL) {
            --not_finished_files_number;
        }
    }
    for (int i = 0; i < num_threads_; ++i) {
        fclose(files_[i]);
        int error_message = remove(itoa(i));
        if (error_message != 0) {
            printf("Error while deleting file %d\n", i);
            exit(1);
        }
    }

    for (int i = 0; i < num_threads_; ++i) {
        free(first_file_string[i]);
    }
    free(first_file_string);
}

void parent_work() {
    int error_code = split_text_into_files_and_sort_them();
    if (error_code) {
        return;
    }
    parent_sort();
}

int main(int argc, char ** argv) {
    num_threads_ = atoi(argv[2]);
    filename_ = argv[1];
    parent_work();
    free(files_);

    return 0;
}
