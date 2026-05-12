# file-partition

This C program is a file splitter utility. It takes a single file and divides its content into smaller, separate "chunk" files based on a user-defined number of parts.

## Here is a detailed breakdown of what the code does:

1. Argument Validation
    The program expects exactly two command-line arguments:

    The filename of the source file.

    The number of parts (block size) you want to split the file into.
    It checks if the number of parts is between 1 and 16. If not, it terminates with an error.

2. File Metadata & Constraints
    The code uses the sys/stat.h library to check the source file's size:

    Size Limit: It enforces a maximum file size of 1 MB (1,000,000 bytes). If the file is larger, the program exits.

    Math Logic: It calculates how many bytes should go into each part. Since the file size might not be perfectly divisible by the number of blocks, it calculates a remainder (r). This remainder is added to the last chunk to ensure no data is lost.

3. Memory Management
    The program reads the entire file into memory at once:

    It allocates memory using malloc based on the file size.

    It opens the file in binary mode ("rb"), reads the data into the allocated buffer, and then closes the source file immediately.

4. The Splitting Process
    The code runs a for loop based on the number of blocks requested:
        
    Dynamic Naming: It creates new filenames with the format sigmoid.png.0, sigmoid.png.1, etc., using snprintf.

    Chunk Writing:
    For the first $n-1$ blocks, it writes a standard chunk size (new_file_size).For the final block, it writes the standard size plus the remainder (r) to account for any leftover bytes.

    Pointer Arithmetic: After writing each chunk, it moves the pointer (fptr) forward to the next section of the memory buffer.

5. Cleanup
Finally, the program releases the memory it borrowed using free(ptr) and returns 0, indicating a successful execution.
