#include "temp.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int setup_database(const char* db_path, sqlite3** db) {
    int rc = sqlite3_open(db_path, db);
    if (rc) {
        fprintf(stderr, "can't open database: %s\n", sqlite3_errmsg(*db));
        return 1;
    }

    return create_table(*db);
}

int create_table(sqlite3* db) {
    int rc = 0, row = 0;
    char* zErrMsg = 0;
    const char* sql;
    const char* tail;
    sqlite3_stmt* res;

    rc = sqlite3_prepare_v2(db,
                            "SELECT name FROM sqlite_master WHERE type='table' "
                            "AND name='TEMPERATURE_HISTORY';",
                            -1, &res, &tail);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "failed to check table setup: %s\n",
                sqlite3_errmsg(db));
        sqlite3_close(db);

        return 1;
    }

    row = sqlite3_step(res);

    if (row == SQLITE_DONE) {
        sql = "CREATE TABLE TEMPERATURE_HISTORY("
              "TIMESTAMP DATETIME DEFAULT CURRENT_TIMESTAMP,"
              "TEMPERATURE REAL NOT NULL DEFAULT 0,"
              "SENSOR VARCHAR(40) NOT NULL,"
              "PRIMARY KEY (TIMESTAMP, SENSOR)"
              ");";

        rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);

        if (rc != SQLITE_OK) {
            sqlite3_free(zErrMsg);
            sqlite3_close(db);

            return 2;
        }
    }

    return 0;
}

int db_store_temp(sqlite3* db, int timestamp, float val, char* sensor) {
    int rc;
    char* sql;
    char* zErrMsg = 0;
    const char* query =
        "INSERT INTO TEMPERATURE_HISTORY (TIMESTAMP,TEMPERATURE, "
        "SENSOR) VALUES (%d, %f, '%s');";

    int query_len = snprintf(NULL, 0, query, timestamp, val, sensor);
    sql = malloc(query_len + 1);

    sprintf(sql, query, timestamp, val, sensor);

    /* Execute SQL statement */
    rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        sqlite3_free(zErrMsg);
        free(sql);
        return 1;
    }

    free(sql);
    return 0;
}

db_temp_row_t* db_get_temp(sqlite3* db, char* sensor, int limit,
                           int* res_count) {
    const char* tail;
    sqlite3_stmt* res;
    int row = 0;

    *res_count = 0;

    const char* query = "SELECT * FROM TEMPERATURE_HISTORY WHERE SENSOR = '%s' "
                        "ORDER BY `TIMESTAMP` DESC LIMIT %d;";
    int query_len = snprintf(NULL, 0, query, sensor, limit);

    char* sql = malloc((sizeof(char) * query_len) + 1);
    db_temp_row_t* result = malloc(sizeof(db_temp_row_t) * limit);

    sprintf(sql, query, sensor, limit);

    sqlite3_prepare_v2(db, sql, 1000, &res, &tail);

    for (int i = 0; i < limit; i++) {
        row = sqlite3_step(res);
        if (row == SQLITE_ROW) {
            result[i].timestamp = sqlite3_column_int(res, 0);
            result[i].temperature = sqlite3_column_double(res, 1);
            *res_count = *res_count + 1;
        } else {
            break;
        }
    }

    sqlite3_finalize(res);
    free(sql);

    return result;
}
