#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

/* pmd metadata */
struct pmd{
    char filename[256];
    int parts_bound;
    int block_size;
    int extra_remain;
    int file_size;
};


/* generate pmd */
void generate_pmd(char filename[256],int parts_bound,int block_size, 
    int extra_remain,int file_size) {

    // request new pmd block
    struct pmd new_pmd;

    // fill pmd block with data
    strncpy(new_pmd.filename,filename,sizeof(new_pmd.filename));
    new_pmd.parts_bound = parts_bound;
    new_pmd.block_size = block_size;
    new_pmd.extra_remain = extra_remain;
    new_pmd.file_size = file_size;

    printf("filename: %s \n parts_bound: %x \n block_size: %x\n file_size: %x \n",
        filename,parts_bound,block_size,file_size);

    // array of chars
    char filename_buffer[1024];
    sprintf(filename_buffer,"%s.pmd",filename);

    // write to disk
    FILE *fh = fopen(filename_buffer,"wb");
    if (fh != NULL) {
        fwrite(&new_pmd, sizeof(new_pmd), 1, fh);
        fclose(fh);
    }
}

/* Partition of files into parts */
void partition(char *_filename,int parts_number) {   
    // size vaildation
    if (parts_number > 16 || parts_number <= 0) {
        printf("Please enter parts size less than 16\n");
        exit(1);
    }

    // file metadata
    struct stat st;
    if (stat(_filename,&st) == -1) {
        printf("File doesn't exist!\n");
        exit(1);
    }

    // 1000000 = 1 MB
    if(st.st_size > 1000000) {
        printf("Size of file is large!\n");
        exit(1);
    }
    int r = (st.st_size % parts_number);
    int block_size = (st.st_size - r) / parts_number;

    // Memory allocation (allocate based on file size)
    void *ptr = malloc(st.st_size);
    
    // Open the file
    FILE *fh = fopen(_filename,"rb");
    if (fh != NULL) {
        fread(ptr, st.st_size, 1, fh);
        fclose(fh);
    }

    char *fptr = (char*) ptr;

    // Create parts into different files
    for(int i=0; i < parts_number; i++) {
        char filename[1024];
        snprintf(filename, sizeof(filename), "%s.%d", _filename, i);
        FILE *fh = fopen(filename,"wb");
        if (fh != NULL) {
            if (i+1 == parts_number) {
                fwrite(fptr, block_size+r, 1, fh);
                fclose(fh);
            } else {
                fwrite(fptr, block_size, 1, fh);
                fclose(fh);
            }
        }
        fptr += block_size;
    }
    generate_pmd(_filename, parts_number, block_size, r, st.st_size);
    free(ptr);
}

int main(int argv, char **argc) {

    // Input from user
    if (argv != 4) {
        printf("Please inpout 3 arguments (usage: %s flag filename blockpart)\n",argc[0]);
        exit(1);
    }
    
    if (argc[1][0] == '-') {
        switch (argc[1][1])
        {
        case 'p':
            partition(argc[2],atoi(argc[3]));
            break;
        
        case 'm':
        ;
        
        default:
            printf("Failed to understand the flag!\n");
            break;
        }
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

    return 0;

}