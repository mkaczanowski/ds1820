#ifndef UTIL_H
#define UTIL_H
#pragma once

#include <json-c/json.h>

extern const char* mime_types[];

const char* get_filename_ext(const char* filename);
const char* get_mime_type(const char* url);

const char* config_lookup_string(struct json_object* cfg, const char* key);
int config_lookup_int(struct json_object* cfg, const char* key);

#endif
