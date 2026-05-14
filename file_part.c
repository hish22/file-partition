#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NYBBLE 4 
#define BYTE 8
#define NYBY 12
#define WORD 16

#define TRUE 1
#define FALSE 0

#define UNUSED 0

/* pmd metadata */
struct pmd{
    char filename[256];
    int parts_bound;
    int block_size;
    int extra_remain;
    long int real_file_size;
    long int file_size_with_random;
    unsigned short reconstruct_bits;
};


/* generate pmd */
void generate_pmd(char filename[256],int parts_bound,int block_size, 
    int extra_remain,int file_size, unsigned short reconstruct_bits, int random_parts) {

    // request new pmd block
    struct pmd new_pmd;

    // fill pmd block with data
    strncpy(new_pmd.filename,filename,sizeof(new_pmd.filename));
    new_pmd.parts_bound = parts_bound;
    new_pmd.block_size = block_size;
    new_pmd.extra_remain = extra_remain;
    new_pmd.real_file_size = file_size;
    new_pmd.file_size_with_random = file_size + (block_size * random_parts);
    new_pmd.reconstruct_bits = reconstruct_bits;

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

/* build a random block with data */
void generate_rand_block(char filename[1024], int block_size) {
    
    FILE *randomfh = fopen(filename,"wb");

    if (randomfh == NULL) {
        printf("Error writing random file");
        return;
    }

    for(int v=0; v < block_size; v++) {
        unsigned char rand_byte = (unsigned char)(rand() % 256);
        fwrite(&rand_byte,sizeof(unsigned char), 1,randomfh);
    }
    fclose(randomfh);

}

/* Generate random mask */
short generate_random_reconstruct_mask(int random_parts, int real_parts) {
    unsigned short mask = 0x0000;

    int total_parts = random_parts + real_parts;

    int bits_set = 0;

    while (bits_set < random_parts) {
        int bit_pos = rand() % total_parts;
        
        // Check if the bit at bit_pos is already set
        if (!((mask >> bit_pos) & 1)) {
            mask |= (1 << bit_pos);
            bits_set++;
        }
    }

    return mask;
}

/* split block from memory to disk (into files) */
void split_blocks_into_file(char *fptr, char *_filename, unsigned short mask, int parts_number, int random_parts, int block_size, int remain_value) {
    
    // Total block size with random
    int total_parts = parts_number+random_parts;
    
    for(int i=0; i < total_parts; i++) {

        unsigned short new_mask = (mask >> i) & 0x0001;

        printf("mask %d: 0x%x\n",i,new_mask);

        char filename[1024];
        snprintf(filename, sizeof(filename), "%s.%d", _filename, i);

        if(new_mask) {
            generate_rand_block(filename, block_size);
            // random_parts -= 1;
            continue;
        }

        FILE *fh = fopen(filename,"wb");
        if (fh != NULL) {
            if (i+1 == total_parts) {
                fwrite(fptr, block_size+remain_value, 1, fh);
                fclose(fh);
            } else {
                fwrite(fptr, block_size, 1, fh);
                fclose(fh);
            }
        }
        fptr += block_size;
    }
}

/* Partition of files into parts */
void partition(char *_filename, int parts_number, int random_parts) {   
    // parts_number size vaildation
    if (parts_number >= 8 || parts_number <= 0) {
        printf("Please enter parts size less than or 8\n");
        exit(1);
    }

    // random_parts size vaildation
    if (random_parts && (random_parts >= 8 || random_parts <= 0)) {
        printf("Please enter random parts size less than or 8\n");
        exit(1);
    }

    // merge real_parts with random_parts
    int read_random_parts = parts_number + random_parts;

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

    // block_size have no effect by random_number
    int remain_value = (st.st_size % parts_number);
    int block_size = (st.st_size - remain_value) / (parts_number);

    // Memory allocation (allocate based on file size)
    void *ptr = malloc(st.st_size);
    
    // Open the file
    FILE *fh = fopen(_filename,"rb");
    if (fh != NULL) {
        fread(ptr, st.st_size, 1, fh);
        fclose(fh);
    }

    char *fptr = (char*) ptr;

    // build a new random mask
    unsigned short r_mask = generate_random_reconstruct_mask(random_parts, parts_number);

    printf("main mask: 0x%x \n",r_mask); 

    // Create parts into different files
    split_blocks_into_file(fptr, _filename, r_mask, parts_number, random_parts, block_size, remain_value);

    // generate metadata file
    generate_pmd(_filename, read_random_parts, block_size, remain_value, st.st_size, r_mask,random_parts);
    free(ptr);
}

/* read parts into memory to reconstruct file */
void reconstruct_file_into_mem(char* ffptr, struct pmd *file_pmd_ptr, char *org_file,char *filename, int rotation, int is_not_random) {
    for(int p=0; p < file_pmd_ptr->parts_bound; p++) {
        unsigned short new_mask = (file_pmd_ptr->reconstruct_bits >> p) & 0x0001;

        // If this is a random block and we are in "clean" mode, skip it entirely
        if(new_mask && is_not_random) {
            // We increment rotation because the file extension (.0, .1) still advances
            if(++rotation == file_pmd_ptr->parts_bound) rotation = 0;
            continue; 
        }

        char partfilename[1024];
        snprintf(partfilename, sizeof(partfilename), "%s.%d", org_file, rotation);

        FILE *pfh = fopen(partfilename, "rb");
        if(pfh != NULL) {
            size_t size_to_read = file_pmd_ptr->block_size;
            
            /* Logic for extra_remain needs to trigger only on the actual last real part */
            if (p + 1 == file_pmd_ptr->parts_bound) {
                size_to_read += file_pmd_ptr->extra_remain;
            }

            fread(ffptr, size_to_read, 1, pfh);
            fclose(pfh);
            ffptr += size_to_read;
        }

        if(++rotation == file_pmd_ptr->parts_bound) {
            rotation = 0;
        }
    }
}

/* reconstruct file from partitioned files */
void merge(char *filename, char *out_filename, int rotation, int is_not_random) {
    
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

    // file size cond
    long int latest_file_size = is_not_random ? file_pmd_ptr->real_file_size : file_pmd_ptr->file_size_with_random;

    // allocate memory based on file size
    void *fptr = malloc(latest_file_size);
    char *ffptr = (char*) fptr;

    // Choose file name based on randomization or not
    char *o_filename = file_pmd_ptr->filename;
    if(out_filename) {
        o_filename = out_filename;
    }

    // Read from disk to memory
    reconstruct_file_into_mem(ffptr, file_pmd_ptr, file_pmd_ptr->filename, o_filename,rotation, is_not_random);

    printf("File size: %lu\n",latest_file_size);

    // Create File from memory
    FILE *fh_c = fopen(o_filename,"wb");
    if (fh_c != NULL) {
        fwrite(fptr, latest_file_size, 1, fh_c);
        fclose(fh_c);
    }

    printf("File constructed!\n");

    free(fptr);
    free(ptr);
}

int main(int argv, char **argc) {

    srand(time(NULL));

    if (argv < 2) {
        // printf("0x%x \n",generate_random_reconstruct_mask(3,2)); 
        printf("Please choose a flag (-p,-m)\n");
        exit(1);
    }

    if (argc[1][0] == '-') {
        switch (argc[1][1]) {
        case 'p':
            if (argv == 4) {
                partition(argc[2],atoi(argc[3]), UNUSED);
            } else if(argv == 6 && (argc[4][0] == '-' && argc[4][1] == 'R')) {
                partition(argc[2], atoi(argc[3]), atoi(argc[5]));
            } else {
                printf("Please inpout 3 arguments (usage: %s -p filename blockpart)\n",argc[0]);
                exit(1);
            }
            
            break;
        
        case 'm':
            if (argv == 3) {
                merge(argc[2],NULL,0,TRUE);
            } else if (argv == 5 && (argc[3][0] == '-' && argc[3][1] == 'r')) {
                merge(argc[2],NULL,atoi(argc[4]),TRUE);
            } else if (argv == 5 && (argc[3][0] == '-' && argc[3][1] == 'R')) {
                merge(argc[2],argc[4],0,FALSE);
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