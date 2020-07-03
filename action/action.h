#ifndef ACTION_H_
#define ACTION_H_

#include "../map/map_lib.h"
#include <json-c/json.h>
#include <sqlite3.h>
#include <stdio.h>

typedef enum temperature_t { CELCIUS, FARENHEIT } temperature_t;

struct json_object* get_temp_action(sqlite3* db, temperature_t format,
                                    map_t* post_args);
struct json_object* update_settings_action(struct json_object** cfg,
                                           map_t* post_args);

#endif /* ACTION_H_ */
