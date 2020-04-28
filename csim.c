/* Created by : Safiya Jan
   Andrew ID  : sjan
*/

#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "cachelab.h"
#include <math.h>


typedef struct cache_line_header cache_line;
typedef struct cache_sets_header cache_sets;
typedef struct cache_header* cache_t;


  //Structure for cache line
  struct cache_line_header{
  	int valid_bit;
  	int tag;
  	int set_index;
  	int block_offset;
  	int timestamp;
  }; 

  //Stucture for array of sets
  struct cache_sets_header{
  	cache_line *line;
  	//int set_index;
  };

  //Struct for cache
  struct cache_header{
  	int num_of_tagbits;
    int num_of_blockbits;
  	int num_of_setbits;
  	int num_of_sets;
  	int num_of_lines;
  	int hit_count;
  	int miss_count;
  	int eviction_count;
  	cache_sets *sets;
  };

 
  //Frees all allocated memory
  void free_cache(cache_t cache_sim)
  {
    
    int sets = cache_sim -> num_of_sets;
    
    for (int i = 0; i < sets; i++)
    {
       cache_sets required_set = cache_sim->sets[i];
       if (required_set.line != NULL)
       {
        free (required_set.line);
       }
    }

    if (cache_sim->sets != NULL)
       {
        free(cache_sim->sets);
       }
    
  }

  //Returns a new cache
  cache_t cache_new(int set, int line, int blockbits)
  {
   

    int sets = pow(2,set);
    int lines = line;
    
    //Creates new cache and initializes all the fields
  	cache_t new_cache = malloc(sizeof(cache_t));
  	new_cache -> num_of_tagbits = 64 - set - blockbits;
  	new_cache -> num_of_blockbits = blockbits;
  	new_cache -> num_of_setbits = set;
  	new_cache -> num_of_sets = sets;
  	new_cache -> hit_count = 0;
  	new_cache -> miss_count = 0;
  	new_cache -> eviction_count = 0;
    new_cache -> num_of_lines = line;
    new_cache -> sets = (cache_sets*)malloc(sizeof(cache_sets)*sets);
   
    cache_sets* temp_cache = new_cache->sets;

    //Adds array of lines to each set index
    for(int i = 0; i < sets; i++)
    {
    	temp_cache[i].line = (cache_line*)malloc(sizeof(cache_line)*lines);

    	for(int k = 0; k<lines; k++)
    	{
    	   temp_cache[i].line[k].set_index = i;
    	   temp_cache[i].line[k].valid_bit = 0;
    	   temp_cache[i].line[k].timestamp = 0;
    	}
    }

    return new_cache;
  }

//Finds the line to evict in the given set by checking the lowest timestamp
int find_evict_line(cache_sets set, int E)
{
   int minindex = 0;
   int mintimestamp = set.line[0].timestamp;
   for (int j = 1; j < E; j++)
   {
   		if (set.line[j].timestamp < mintimestamp)
   		{
   			minindex = j;
   			mintimestamp = set.line[j].timestamp;
   		}
   }
   return minindex;
 
}

//Finds an empty line in the given set by checking if valid bit == 0
int find_empty_line(cache_sets set, int E)
{
   
   for (int j = 0; j < E; j++)
   {
   		if (set.line[j].valid_bit == 0)
   		{
   			return j;
   		}
   }
   return -1;
}

//Finds the maximum timestamp in the set and returns its index
int maxstamp(cache_sets set, int E)
{
  
   int maxtimestamp = set.line[0].timestamp;
   for (int j = 1; j < E; j++)
   {
      if (set.line[j].timestamp > maxtimestamp)
      {
        maxtimestamp = set.line[j].timestamp;
      }
   }
   return maxtimestamp;
  
}




//Function that simulates the cache
cache_t cache_simulator(cache_t cache_sim, unsigned long long int address)
{
	
  /*We assume that the cache is full so we when we reach a valid bit of 0
  we know that the cache = 0 */
  
  int full_cache = 1;
	
	//Calculating number of bits each field
	int setbits = cache_sim -> num_of_setbits;
	int blockbits = cache_sim -> num_of_blockbits;
	int tag_size = 64 - (setbits+blockbits);

	//Calculating number of lines
	int E = cache_sim->num_of_lines;

  //Creating var so that we know if we have had any hits in this iteration
  int hitcount = 0;
	
	
  //Calculating setindex and tag of the address
	int simtag = address >> (setbits+blockbits);
  int simsetindex = (address<<(tag_size)) >> (tag_size + blockbits);    
  
  //Getting the required set
  cache_sets required_set = cache_sim->sets[simsetindex];

    int validbit = 0;
   
    for (int j = 0; j < E; j++)
    {
      validbit = required_set.line[j].valid_bit;
    
      if (validbit == 1 && simtag == required_set.line[j].tag)
      {
      	
        /* When a hit is recieved, that line becomes the last to be evicted, so we set its timestamp to the 
        max timstemp in that set.
        We then decrement the time stamp of each valid line in the set */
      	cache_sim->hit_count++;
        int timestamp = required_set.line[j].timestamp;
        required_set.line[j].timestamp = maxstamp(required_set, E);

        for (int k = 0; k < E; k++)
        {
          if (required_set.line[k].valid_bit == 1 && required_set.line[k].tag != simtag && required_set.line[k].timestamp > timestamp )
          {
            required_set.line[k].timestamp--;
          } 
        }
      	hitcount++;

      }
      //If we get a valid bit == 0, we know that the cache is not full
      else if ((validbit == 0) && full_cache)
      {
        full_cache = 0;
      }
	}
  
	if (hitcount == 0)
	{
		cache_sim->miss_count++;
	}
	if (full_cache == 1 && hitcount == 0)
  {
      /* We come here, the cache is full and we need to evict */

      cache_sim->eviction_count++;
      int evictlineindex = find_evict_line(required_set, E);

      /* We set the values of line that we need to evict and set its timestamp to the highest timestamp
      as it will be the last to be evicted
      We then decrement the timestamp of each line*/

      required_set.line[evictlineindex].set_index = simsetindex;
      required_set.line[evictlineindex].tag = simtag;
      required_set.line[evictlineindex].timestamp = maxstamp(required_set,E);
      
      for (int k = 0; k < E; k++)
        {
          if (required_set.line[k].valid_bit == 1 && required_set.line[k].tag != simtag )
          {
            required_set.line[k].timestamp--;
          }
        }
      required_set.line[evictlineindex].valid_bit = 1;
  }

  else if (full_cache == 0 && hitcount == 0)
	{
		
		int emptyindex = find_empty_line(required_set, E);

    /* We come here when the cache is not full and we have not hit.
    Here we just set all the fields of the empty line and set its timestamp to the highest
    timestamp in the set + 1 */

		required_set.line[emptyindex].set_index = simsetindex;
		required_set.line[emptyindex].timestamp = maxstamp(required_set, E)+1;
		required_set.line[emptyindex].tag = simtag;
		required_set.line[emptyindex].valid_bit = 1;
    
	}

	return cache_sim;
}


int main(int argc, char **argv)
{
  int ret;
  int set = 0;
  int line = 0;
  int blockbits = 0;
  char *filename;

  /*int hit_count = 0;
  int miss_count = 0;
  int eviction_count = 0;*/
 
  //Reading Command Line Arguments
  while((ret = getopt(argc,argv,"s:E:b:t:")) != -1)
  {
  	switch(ret)
  	{
  		case 's':
  			set = atoi(optarg);
  			break;
  		case 'E':
  			line = atoi(optarg);
  			break;
  		case 'b':
  			blockbits = atoi(optarg);
  			break;
  		case 't':
  			filename = optarg;
  			break;
  		/*case '?':
  			if (optopt == 's' || optopt == 'E' || optopt == 'b' || optopt == 't')
  				fprintf(stderr, "Option -%c needs an argument\n",optopt);
  			else
  				fprintf(stderr, "Unknown option -%c \n",optopt);
  			break;*/
  		default:
  			fprintf(stderr, "getopt\n");

  	}
  }
  printf("set = %d\n", set);
  printf("line = %d\n", line);
  printf("blockbits = %d\n", blockbits);
  printf("filename = %s\n", argv[8]);
  
  char type_access;
  unsigned long long address;
  int num_bytes;

  /*
  int sets = pow(2,s);
  int lines = line;
  int block_size */

  cache_t newcache = cache_new(set, line, blockbits);
  

  FILE *file_ptr = fopen(filename,"r");
  while(fscanf(file_ptr," %c %llx,%d", &type_access, &address, &num_bytes) == 3)
  {
     newcache = cache_simulator(newcache,address);
  }

  //Printing Cache Summary
  printSummary(newcache->hit_count,newcache->miss_count,newcache->eviction_count);
  free_cache(newcache);
  fclose(file_ptr);

  return 0;
}


