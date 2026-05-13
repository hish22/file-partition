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
    long int file_size;
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

    printf("filename: %s \n parts_bound: %x \n block_size: %x\n file_size: %d \n",
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

/* reconstruct file from partitioned files */
void merge(char *filename, int rotation) {
    
    // Vaildate metadata file type
    char ext[4];
    int ext_length = strlen(filename);
    sprintf(ext,"%s",&filename[ext_length - 3]);

    if(strcmp(ext,"pmd")) {
        printf("Failed to read pmd file!\n");
        exit(1);
    }

    // allocate memory for pmd file
    void *ptr = malloc(sizeof(struct pmd));
    struct pmd *file_pmd_ptr = (struct pmd*) ptr;

    // read metadata file into memory
    FILE *fh = fopen(filename,"rb");
    if(fh != NULL) {
        fread(ptr,sizeof(struct pmd),1,fh);
        fclose(fh);
    }

    // Vaildate if rotation file is in the limit bound
    if(rotation >= file_pmd_ptr->parts_bound) {
        printf("Rotation value must be in the limit bound\n");
        free(ptr);
        exit(1);
    }

    // allocate memory based on file size
    void *fptr = malloc(file_pmd_ptr->file_size);
    char *ffptr = (char*) fptr;

    // Read from disk to memory
    for(int p=0; p < file_pmd_ptr->parts_bound; p++) {
        char partfilename[1024];
        snprintf(partfilename,sizeof(partfilename),"%s.%d",file_pmd_ptr->filename,rotation);
        FILE *pfh = fopen(partfilename,"rb");
        if(pfh != NULL) {
            if (p+1 == file_pmd_ptr->parts_bound) {
                fread(ffptr, file_pmd_ptr->block_size+file_pmd_ptr->extra_remain, 1, pfh);
                fclose(pfh);
            } else {
                fread(ffptr, file_pmd_ptr->block_size, 1, pfh);
                fclose(pfh);
            }
        }
        ffptr += file_pmd_ptr->block_size;
        if(++rotation == file_pmd_ptr->parts_bound) {
            rotation = 0;
        }
    }


    // Create File from memory
    FILE *fh_c = fopen(file_pmd_ptr->filename,"wb");
    if (fh_c != NULL) {
        fwrite(fptr,file_pmd_ptr->file_size,1,fh_c);
        fclose(fh_c);
    }

    free(fptr);
    free(ptr);
}

int main(int argv, char **argc) {
    
    if (argc[1][0] == '-') {
        switch (argc[1][1]) {
        case 'p':
            if (argv != 4) {
                printf("Please inpout 3 arguments (usage: %s -p filename blockpart)\n",argc[0]);
                exit(1);
            }
            partition(argc[2],atoi(argc[3]));
            break;
        
        case 'm':
            if (argv == 3) {
                merge(argc[2],0);
            } else if (argv == 5 && (argc[3][0] == '-' && argc[3][1] == 'r')) {
                merge(argc[2],atoi(argc[4]));
            } else {
                printf("Please input 5 arguments (usage: %s -m filename -r rotation rotation_filename) or\n",argc[0]);
                printf("Please input 2 arguments (usage: %s -m filename)\n",argc[0]);
                exit(1);
            }
            break;
        
        default:
            printf("Failed to understand the flag!\n");
            break;
        }
    } else {
        printf("Please use a flag! (usage: -p or -m)");
        exit(1);
    }

    return 0;

}