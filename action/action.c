#include "action.h"
#include "../sqlite/temp.h"
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct json_object* get_temp_action(sqlite3* db, temperature_t format,
                                    map_t* postParams) {
    int resCount;
    int count = atoi(map_get(postParams, "count"));
    char* sensor = map_get(postParams, "sensor");

    db_temp_row_t* temps = db_get_temp(db, sensor, count, &resCount);

    struct json_object* arr = json_object_new_array();
    for (int i = 0; i < resCount; i++) {
        struct json_object* jobj = json_object_new_object();
        json_object_object_add(jobj, "timestamp",
                               json_object_new_int(temps[0].timestamp));
        json_object_object_add(jobj, "sensor", json_object_new_string(sensor));

        double temp = temps[i].temperature;
        if (format == FARENHEIT) {
            temp = (temp * (9.0 / 5.0)) + 32;
        }
        json_object_object_add(jobj, "temperature",
                               json_object_new_double(temp));

        json_object_array_add(arr, jobj);
    }

    return arr;
}

struct json_object* update_settings_action(struct json_object** cfg,
                                           map_t* data) {
    char* settings = map_get(data, "data");

    struct json_object* old_cfg = *cfg;
    struct json_object* new_cfg = json_tokener_parse(settings);

    // racy and leaking! but it's just a proof of concept
    *cfg = new_cfg;

    struct json_object* jobj = json_object_new_object();
    json_object_object_add(jobj, "result", json_object_new_int(1));

    return jobj;
}
