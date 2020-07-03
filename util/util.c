#include "util.h"
#include <json-c/json.h>
#include <string.h>

const char* mime_types[] = {"text/html", "text/css", "image/png",
                            "text/javascript", "application/json"};

const char* get_filename_ext(const char* filename) {
    const char* dot = strrchr(filename, '.');
    if (!dot || dot == filename) {
        return "";
    }

    return dot + 1;
}

const char* get_mime_type(const char* url) {
    const char* ext = get_filename_ext(url);
    const char* mimetype;

    if (strcmp(ext, "html") == 0) {
        mimetype = strdup(mime_types[0]);
    } else if (strcmp(ext, "css") == 0) {
        mimetype = strdup(mime_types[1]);
    } else if (strcmp(ext, "png") == 0) {
        mimetype = strdup(mime_types[2]);
    } else if (strcmp(ext, "js") == 0) {
        mimetype = strdup(mime_types[3]);
    } else if (strcmp(ext, "json") == 0) {
        mimetype = strdup(mime_types[4]);
    } else {
        mimetype = strdup(mime_types[0]);
    }

    return mimetype;
}

const char* config_lookup_string(struct json_object* cfg, const char* key) {
    struct json_object* tmp;
    json_object_object_get_ex(cfg, key, &tmp);

    return json_object_get_string(tmp);
}

int config_lookup_int(struct json_object* cfg, const char* key) {
    struct json_object* tmp;
    json_object_object_get_ex(cfg, key, &tmp);

    return json_object_get_int(tmp);
}
