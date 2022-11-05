#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>

#define PAGE_SIZE 0x1000
#define ROUND_UP(x,y) ((x) % (y) == 0 ? (x) : (x) + ((y) - (x) % (y)))

/// The number of times we've seen each letter in the input, initially zero
size_t letter_counts[26] = {0};

pthread_mutex_t lock; //declare lock variable

//Each created thread will contain its own divided data and size, we create a structure to store this information
typedef struct threadInfo{
char* file_data;
off_t file_size;
} threadInfo_t;

//declaration of the lettercount function
void* lettercount(void* data);

/**
 * This function should divide up the file_data between the specified number of
 * threads, then have each thread count the number of occurrences of each letter
 * in the input data. Counts should be written to the letter_counts array. Make
 * sure you count only the 26 different letters, treating upper- and lower-case
 * letters as the same. Skip all other characters.
 *
 * \param num_threads   The number of threads your program must use to count
 *                      letters. Divide work evenly between threads
 * \param file_data     A pointer to the beginning of the file data array
 * \param file_size     The number of bytes in the file_data array
 */
void count_letters(int num_threads, char* file_data, off_t file_size) {
  int rc = pthread_mutex_init(&lock, NULL); //initialize lock
  assert(rc == 0); //check for errors
  
  off_t fileDivision = (off_t) (floor (file_size/num_threads)); //dividing the file_data over the num_threads

  /* testing
  printf("num_threads: %d\n", num_threads);
  printf("fileSize %lld\n", file_size);
  printf("FileDivision: %lld\n", fileDivision);
  */
  
  off_t remainder = file_size % num_threads; //checking if there's a remainder
 // printf("remainder: %lld\n", remainder);

  if(remainder != 0){
  //if remainder 
  pthread_t threads[num_threads];
  threadInfo_t threadsData[num_threads];
  
  for(int threadTrack = 0; threadTrack < num_threads; threadTrack++){
    //handling remainder
    if(threadTrack == (num_threads - 1)){
    threadsData[threadTrack].file_size = fileDivision + remainder; //default size plus remainder
    threadsData[threadTrack].file_data = &file_data[(threadTrack) * fileDivision]; //data contained in the remainder
    int check = pthread_create(&threads[threadTrack], NULL, lettercount, &threadsData[threadTrack]); //create the thread
    if(check != 0){ //checking errors
      perror("Creating thread failed");
      return;
      }
      break;
    }

    threadsData[threadTrack].file_size = fileDivision; //storing each thread size (except the remainder thread)
    threadsData[threadTrack].file_data = &file_data[threadTrack * fileDivision]; //storing each thread data (except the remainder thread)

    int check = pthread_create(&threads[threadTrack], NULL, lettercount, &threadsData[threadTrack]); //create the thread
    if(check != 0){ //checking errors
    perror("Creating thread failed");
    return;
    } 
  }

  for(int tracker = 0; tracker < num_threads; tracker++){ //joining all threads (including remainder thread)
    int check2 = pthread_join(threads[tracker], NULL);
    if(check2 != 0){ //checking errors
    perror("Joining threads failed");
    return;
      }
    }
  
} else{
  pthread_t threads[num_threads]; //create array of threads
  threadInfo_t threadsData[num_threads]; //create array of information

  //storing the information each thread will check and create threads
  for(int threadTrack = 0; threadTrack < num_threads; threadTrack++){
    threadsData[threadTrack].file_size = fileDivision;
    threadsData[threadTrack].file_data = &file_data[threadTrack * fileDivision];

  int check = pthread_create(&threads[threadTrack], NULL, lettercount, &threadsData[threadTrack]); //create the thread
  if(check != 0){
    perror("Creating thread failed");
    return;
    } 
  }
  for(int tracker = 0; tracker < num_threads; tracker++){
    int check2 = pthread_join(threads[tracker], NULL);
    if(check2 != 0){
    perror("Joining threads failed");
    return;
    }
  }
}
}

/**
 * Procedure that every thread uses to count the number of letters in the file.
 * \param data void parameter that can be casted as the threadInfo_t data structure.
 */
void* lettercount(void* data){

  threadInfo_t* data1 = ((threadInfo_t*) data); //casting data as our info structure (we cannot declare it as threadInfo_t*)
  for(size_t i = 0; i< data1->file_size; i++) {
    char c = data1->file_data[i];
    if(c >= 'a' && c <= 'z') {
      pthread_mutex_lock(&lock); //lock procedure
      letter_counts[c - 'a']++;
      pthread_mutex_unlock(&lock); //unlock procedure
    } else if(c >= 'A' && c <= 'Z') {
      pthread_mutex_lock(&lock);
      letter_counts[c - 'A']++;
      pthread_mutex_unlock(&lock);
    }
  }
return data; //return a void*
}

/**
 * Show instructions on how to run the program.
 * \param program_name  The name of the command to print in the usage info
 */
void show_usage(char* program_name) {
  fprintf(stderr, "Usage: %s <N> <input file>\n", program_name);
  fprintf(stderr, "    where <N> is the number of threads (1, 2, 4, or 8)\n");
  fprintf(stderr, "    and <input file> is a path to an input text file.\n");
}

int main(int argc, char** argv) {
  // Check parameter count
  if(argc != 3) {
    show_usage(argv[0]);
    exit(1);
  }
  
  // Read thread count
  int num_threads = atoi(argv[1]);
  if(num_threads != 1 && num_threads != 2 && num_threads != 4 && num_threads != 8) {
    fprintf(stderr, "Invalid number of threads: %s\n", argv[1]);
    show_usage(argv[0]);
    exit(1);
  }
  
  // Open the input file
  int fd = open(argv[2], O_RDONLY);
  if(fd == -1) {
    fprintf(stderr, "Unable to open input file: %s\n", argv[2]);
    show_usage(argv[0]);
    exit(1);
  }
  
  // Get the file size
  off_t file_size = lseek(fd, 0, SEEK_END);
  if(file_size == -1) {
    fprintf(stderr, "Unable to seek to end of file\n");
    exit(2);
  }

  // Seek back to the start of the file
  if(lseek(fd, 0, SEEK_SET)) {
    fprintf(stderr, "Unable to seek to the beginning of the file\n");
    exit(2);
  }
  
  // Load the file with mmap
  char* file_data = mmap(NULL, ROUND_UP(file_size, PAGE_SIZE), PROT_READ, MAP_PRIVATE, fd, 0);
  if(file_data == MAP_FAILED) {
    fprintf(stderr, "Failed to map file\n");
    exit(2);
  }
  
  // Call the function to count letter frequencies
  count_letters(num_threads, file_data, file_size);
  
  // Print the letter counts
  for(int i=0; i<26; i++) {
    printf("%c: %lu\n", 'a' + i, letter_counts[i]);
  }
  
  return 0;
}
