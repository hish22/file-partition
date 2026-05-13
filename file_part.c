#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdlib.h>

int main(int argv, char **argc) {

    // Input from user
    if (argv != 3) {
        printf("Please inpout 2 arguments (usage: %s filename blockpart)\n",argc[0]);
        exit(1);
    }
    int block_size = atoi(argc[2]);

    // size vaildation
    if (block_size > 16 || block_size <= 0) {
        printf("Please enter parts size less than 16\n");
        exit(1);
    }

    // file metadata
    struct stat st;
    stat(argc[1],&st);

    // 1000000 = 1 MB
    if(st.st_size > 1000000) {
        printf("Size of file is large!\n");
        exit(1);
    }
    int r = (st.st_size % block_size);
    int new_file_size = (st.st_size - r) / block_size;

    // Memory allocation (allocate based on file size)
    void *ptr = malloc(st.st_size);
    
    // Open the file
    FILE *fh = fopen(argc[1],"rb");
    if (fh != NULL) {
        fread(ptr, st.st_size, 1, fh);
        fclose(fh);
    }

    char *fptr = (char*) ptr;

    // Create parts into different files
    for(int i=0; i < block_size; i++) {
        char filename[1024]; 
        snprintf(filename, sizeof(filename), "%s.%d", argc[1], i);
        FILE *fh = fopen(filename,"wb");
        if (fh != NULL) {
            if (i+1 == block_size) {
                fwrite(fptr, new_file_size+r, 1, fh);
                fclose(fh);
            } else {
                fwrite(fptr, new_file_size, 1, fh);
                fclose(fh);
            }
        }
        fptr += new_file_size;
    }

    // Create File again
    // FILE *fh_c = fopen("sigmoid_copy.png","wb");
    // for (int j=0; j < block_size; j++) {
    //     if (fh != NULL) {
    //         if (j+1 == block_size) {
    //             fwrite(fptr, new_file_size+r, 1, fh);
    //             fclose(fh);
    //         } else {
    //             fwrite(fptr, new_file_size, 1, fh);
    //             fclose(fh);
    //         }
    //     }
    //     fptr += new_file_size;
    // }

    // fclose(fh_c);

    free(ptr);

    return 0;

}