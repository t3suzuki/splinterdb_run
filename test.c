#include <stdio.h>
#include <string.h>

#include "splinterdb/default_data_config.h"
#include "splinterdb/splinterdb.h"

#define DB_FILE_NAME    "my_db"
#define DB_FILE_SIZE_MB 1024
#define CACHE_SIZE_MB   64

#define USER_MAX_KEY_SIZE ((int)100)

#define KEY_SIZE (20)
#define VAL_SIZE (80)


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

int
main()
{
   data_config splinter_data_cfg;
   default_data_config_init(USER_MAX_KEY_SIZE, &splinter_data_cfg);

   splinterdb_config splinterdb_cfg;
   memset(&splinterdb_cfg, 0, sizeof(splinterdb_cfg));
   splinterdb_cfg.filename   = DB_FILE_NAME;
   splinterdb_cfg.disk_size  = (DB_FILE_SIZE_MB * 1024 * 1024);
   splinterdb_cfg.cache_size = (CACHE_SIZE_MB * 1024 * 1024);
   splinterdb_cfg.data_cfg   = &splinter_data_cfg;

   splinterdb *spl_handle = NULL; // To a running SplinterDB instance

   int rc = splinterdb_create(&splinterdb_cfg, &spl_handle);
   printf("Created SplinterDB instance, dbname '%s'.\n\n", DB_FILE_NAME);

   char key_buffer[KEY_SIZE];
   char val_buffer[VAL_SIZE];
   memset(key_buffer, 'k', KEY_SIZE);
   memset(val_buffer, 'v', VAL_SIZE);
   
   for (int i=0; i<100; i++) {
     enc(key_buffer, i);
     enc(val_buffer, i);
     slice key = slice_create(KEY_SIZE, key_buffer);
     slice val = slice_create(VAL_SIZE, val_buffer);
     splinterdb_insert(spl_handle, key, val);
   }

   splinterdb_lookup_result result;
   splinterdb_lookup_result_init(spl_handle, &result, 0, NULL);

   int ok = 0;
   for (int i=0; i<100; i++) {
     enc(key_buffer, i);
     slice key = slice_create(KEY_SIZE, key_buffer);
     slice val;
     rc = splinterdb_lookup(spl_handle, key, &result);
     rc = splinterdb_lookup_result_value(&result, &val);
     if (!rc) {
       enc(val_buffer, i);
       if (memcmp(slice_data(val), val_buffer, VAL_SIZE) != 0) {
	 printf("Found mykey: '%s', value: %s\n",
		key_buffer,
		slice_data(val));
       } else {
	 ok++;
       }
     }
   }

   printf("ok = %d\n", ok);
   
   return rc;
}
