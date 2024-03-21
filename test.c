#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "splinterdb/default_data_config.h"
#include "splinterdb/splinterdb.h"

#define DB_FILE_NAME    "/home/tomoya-s/mountpoint/tomoya-s/my_db"
#define DB_FILE_SIZE_MB 65536ULL
#define CACHE_SIZE_MB   1024ULL

#define USER_MAX_KEY_SIZE ((int)100)

#define KEY_SIZE (16)
#define VAL_SIZE (128)

#define N_TH (2)
#define N_ITEM (16*1024*1024)

char enc_map[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!?";
void
enc(char *str, uint64_t i)
{
  memset(str, 'A', 16);
  int ind = 0;
  while (i > 0) {
    str[ind] = enc_map[i % 64];
    ind ++;
    i /= 64;
  }
}

typedef struct {
  splinterdb *spl_handle;
} arg_t;

void
lookup_worker(void *arg)
{
  splinterdb *spl_handle = ((arg_t *)arg)->spl_handle;
   char key_buffer[KEY_SIZE];
   char val_buffer[VAL_SIZE];
   memset(key_buffer, 'k', KEY_SIZE);
   memset(val_buffer, 'v', VAL_SIZE);

   splinterdb_lookup_result result;
   splinterdb_lookup_result_init(spl_handle, &result, 0, NULL);
   int rc;
   int ok = 0;
   for (int i_loop=0; i_loop=8; i_loop++) {
     for (int i=0; i<N_ITEM; i++) {
       int r = rand() % N_ITEM;
       enc(key_buffer, r);
       slice key = slice_create(KEY_SIZE, key_buffer);
       slice val;
       rc = splinterdb_lookup(spl_handle, key, &result);
       rc = splinterdb_lookup_result_value(&result, &val);
       if (!rc) {
	 enc(val_buffer, r);
	 if (memcmp(slice_data(val), val_buffer, VAL_SIZE) != 0) {
	   printf("Found mykey: '%s', value: %s\n",
		  key_buffer,
		  (char *)slice_data(val));
	 } else {
	   ok++;
	 }
       }
     }
   }
   printf("ok = %d\n", ok);
}

int
main()
{
  platform_set_log_streams(stdout, stderr);
  
   data_config splinter_data_cfg;
   default_data_config_init(USER_MAX_KEY_SIZE, &splinter_data_cfg);

   splinterdb_config splinterdb_cfg;
   memset(&splinterdb_cfg, 0, sizeof(splinterdb_cfg));
   splinterdb_cfg.filename   = DB_FILE_NAME;
   splinterdb_cfg.disk_size  = (DB_FILE_SIZE_MB * 1024 * 1024);
   splinterdb_cfg.cache_size = (CACHE_SIZE_MB * 1024 * 1024);
   splinterdb_cfg.data_cfg   = &splinter_data_cfg;
   splinterdb_cfg.io_flags = O_RDWR | O_CREAT;

   splinterdb *spl_handle = NULL; // To a running SplinterDB instance

   int rc = splinterdb_create(&splinterdb_cfg, &spl_handle);
   printf("Created SplinterDB instance, dbname '%s'.\n\n", DB_FILE_NAME);

   char key_buffer[KEY_SIZE];
   char val_buffer[VAL_SIZE];
   memset(key_buffer, 'k', KEY_SIZE);
   memset(val_buffer, 'v', VAL_SIZE);
   
   for (int i=0; i<N_ITEM; i++) {
     enc(key_buffer, i);
     enc(val_buffer, i);
     slice key = slice_create(KEY_SIZE, key_buffer);
     slice val = slice_create(VAL_SIZE, val_buffer);
     splinterdb_insert(spl_handle, key, val);
   }


   printf("run workers\n");
   
   pthread_t pth[N_TH];
   arg_t arg[N_TH];
   for (int i_th=0; i_th<N_TH; i_th++) {
     arg[i_th].spl_handle = spl_handle;
     pthread_create(&pth[i_th], NULL, (void *(*)(void *))lookup_worker, &arg[i_th]);
   }

   for (int i_th=0; i_th<N_TH; i_th++) {
     pthread_join(pth[i_th], NULL);
   }
   
   return rc;
}
