#ifndef TEMP_H_
#define TEMP_H_

#include <sqlite3.h>

typedef struct {
    int timestamp;
    float temperature;
} db_temp_row_t;

int setup_database(const char* db_path, sqlite3** db);
int create_table(sqlite3* db);
int db_store_temp(sqlite3* db, int timestamp, float val, char* sensor);
db_temp_row_t* db_get_temp(sqlite3* db, char* sensor, int count, int* resCount);

#endif /* TEMP_H_ */
