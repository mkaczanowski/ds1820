#define _FILE_OFFSET_BITS 64
#include "action/action.h"
#include "map/map_lib.h"
#include "sqlite/temp.h"
#include "util/util.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <json-c/json.h>
#include <microhttpd.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define POST_BUFFER_SIZE 512
#define MAX_NAME_SIZE 1024
#define PAYLOAD_SIZE (64 * 1024 * 1024)

#define GET 0
#define POST 1

static struct json_object* cfg;
static sqlite3* db;
static const char* errorpage =
    "<html><body>This doesn't seem to be right.</body></html>";

typedef struct connection_info_struct {
    int conn_type;
    struct map_t* data;
    struct MHD_PostProcessor* postProcessor;
} connection_info_struct_t;

int handle_POST(const char* url, struct MHD_Connection* connection,
                map_t* post_args) {
    // route request
    int free_jobj = 1;
    struct json_object* jobj;

    int scale = config_lookup_int(cfg, "temperature_scale");

    if (strcmp(url, "/get_temp") == 0) {
        jobj = get_temp_action(db, scale, post_args);
    } else if (strcmp(url, "/get_settings") == 0) {
        jobj = cfg;
        free_jobj = 0;
    } else if (strcmp(url, "/set_settings") == 0) {
        jobj = update_settings_action(&cfg, post_args);
    } else {
        fprintf(stderr, "invalid action: %s\n", url);
        return -1;
    }

    // json -> string conversion
    const char* response_text = json_object_to_json_string(jobj);

    // send response
    struct MHD_Response* response = MHD_create_response_from_buffer(
        strlen(response_text), (void*)response_text, MHD_RESPMEM_MUST_COPY);

    if (!response) {
        if (free_jobj)
            json_object_put(jobj);
        return MHD_NO;
    }

    MHD_add_response_header(response, "Content-Type", mime_types[4]);
    int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);

    // cleanup
    MHD_destroy_response(response);
    if (free_jobj)
        json_object_put(jobj);

    return ret;
}

int handle_GET(const char* url, struct MHD_Connection* connection) {
    struct MHD_Response* response;
    struct stat sbuf;
    int fd, ret;
    char path[255];

    const char* mimetype = get_mime_type(url);
    sprintf(path, "../static%s", url);

    if ((-1 == (fd = open(path, O_RDONLY))) || (0 != fstat(fd, &sbuf))) {
        if (fd != -1) {
            close(fd);
        }

        response = MHD_create_response_from_buffer(
            strlen(errorpage), (void*)errorpage, MHD_RESPMEM_PERSISTENT);

        if (response) {
            ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR,
                                     response);
            MHD_destroy_response(response);

            return MHD_YES;
        }

        return MHD_NO;
    }

    response = MHD_create_response_from_fd_at_offset64(sbuf.st_size, fd, 0);
    ret = MHD_queue_response(connection, MHD_HTTP_OK, response);

    MHD_add_response_header(response, "Content-Type", mimetype);
    MHD_destroy_response(response);

    return ret;
}

int handle_error(const char* url, struct MHD_Connection* connection) {
    int ret;
    struct MHD_Response* response;

    response = MHD_create_response_from_buffer(strlen("ERROR"), (void*)"ERROR",
                                               MHD_RESPMEM_PERSISTENT);
    if (!response) {
        return MHD_NO;
    }

    ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);

    return ret;
}

static int iterate_post(void* coninfo_cls, enum MHD_ValueKind kind,
                        const char* key, const char* filename,
                        const char* content_type, const char* transfer_encoding,
                        const char* data, uint64_t off, size_t size) {
    connection_info_struct_t* con_info = coninfo_cls;

    if (size > 0 && size <= MAX_NAME_SIZE) {
        map_set(con_info->data, (char*)key, (char*)data);
    }

    return MHD_YES;
}

static void request_completed(void* cls, struct MHD_Connection* connection,
                              void** con_cls,
                              enum MHD_RequestTerminationCode toe) {
    connection_info_struct_t* con_info = *con_cls;

    if (con_info == NULL)
        return;

    if (con_info->conn_type == POST) {
        MHD_destroy_post_processor(con_info->postProcessor);

        if (con_info->data) {
            free(con_info->data);
        }
    }

    free(con_info);
    *con_cls = NULL;
}

static int answer_to_connection(void* cls, struct MHD_Connection* connection,
                                const char* url, const char* method,
                                const char* version, const char* upload_data,
                                size_t* upload_data_size, void** con_cls) {
    if (NULL == *con_cls) {
        connection_info_struct_t* con_info;

        con_info = malloc(sizeof(connection_info_struct_t));
        if (NULL == con_info) {
            return MHD_NO;
        }

        con_info->data = map_create();

        if (0 == strcmp(method, "POST")) {
            con_info->postProcessor = MHD_create_post_processor(
                connection, POST_BUFFER_SIZE, iterate_post, (void*)con_info);

            if (NULL == con_info->postProcessor) {
                free(con_info);
                return MHD_NO;
            }

            con_info->conn_type = POST;
        } else {
            con_info->conn_type = GET;
        }

        *con_cls = (void*)con_info;

        return MHD_YES;
    }

    if (strcmp(method, "GET") == 0) {
        return handle_GET(url, connection);
    }

    if (strcmp(method, "POST") == 0) {
        connection_info_struct_t* con_info = *con_cls;

        if (*upload_data_size != 0) {
            MHD_post_process(con_info->postProcessor, upload_data,
                             *upload_data_size);
            *upload_data_size = 0;

            return MHD_YES;
        } else if (NULL != con_info->data) {
            return handle_POST(url, connection, con_info->data);
        }
    }

    return handle_error(url, connection);
}

float read_temp_from_sensor(const char* sensor_path) {
    int d = open(sensor_path, O_RDONLY);
    char dataT1[512];
    char* token;

    read(d, dataT1, 512);
    token = strtok(dataT1, "t=");
    while (token != NULL) {
        if (strtok(NULL, "t=") != NULL) {
            token = strtok(NULL, "t=");
        } else {
            break;
        }
    }

    return (float)atoi(token) / 1000;
}

void* collect_temperature_thread(void* param) {
    sqlite3* db = (sqlite3*)param;
    int current_time = (int)time(NULL);
    int last_time_sent = current_time;
    float sensor_temp;

    const char* email = config_lookup_string(cfg, "email_recipient");
    int db_update_rate_secs = config_lookup_int(cfg, "db_update_rate_secs");
    int email_send_rate_secs = config_lookup_int(cfg, "email_send_rate_secs");

    struct json_object* sensors_range;
    json_object_object_get_ex(cfg, "sensors_range", &sensors_range);

    int temp_min = config_lookup_int(sensors_range, "min");
    int temp_max = config_lookup_int(sensors_range, "max");

    const char* sensor_path;
    struct json_object* sensors;
    struct json_object* sensor_obj;

    json_object_object_get_ex(cfg, "sensors", &sensors);
    size_t n_sensors = json_object_array_length(sensors);

    while (1) {
        sleep(db_update_rate_secs);
        current_time = (int)time(NULL);

        for (size_t i = 0; i < n_sensors; i++) {
            sensor_obj = json_object_array_get_idx(sensors, i);
            sensor_path = json_object_get_string(sensor_obj);

            sensor_temp = read_temp_from_sensor(sensor_path);

            int res = db_store_temp(db, current_time, sensor_temp,
                                    (char*)sensor_path);
            if (res) {
                fprintf(stderr, "failed to record temperature in database\n");
                continue;
            } else {
                fprintf(stderr, "collected data from sensor: %s\n",
                        sensor_path);
            }

            if (sensor_temp >= temp_max || sensor_temp <= temp_min) {
                if (current_time - last_time_sent >= email_send_rate_secs) {
                    char* mailText;
                    asprintf(&mailText,
                             "echo -e 'Subject: Temperature "
                             "notification\r\n\r\nDear "
                             "Sir,\nOne of The sensors detected the "
                             "temperature out of "
                             "the bound!\n\n Please take a look!\n Sensor: "
                             "%.02f\n'| "
                             "msmtp --from=default -t %s &",
                             sensor_temp, email);

                    system(mailText);
                    free(mailText);

                    last_time_sent = current_time;
                }
            }
        }
    }

    return NULL;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "missing config path\n");
        return EXIT_FAILURE;
    }

    /*** SETUP CONFIG ***/
    int fd = open(argv[1], O_RDONLY);
    int len = lseek(fd, 0, SEEK_END);
    void* data = mmap(0, len, PROT_READ, MAP_PRIVATE, fd, 0);

    cfg = json_tokener_parse(data);

    /*** SETUP SERVER ***/
    const char* host = config_lookup_string(cfg, "host");

    int port = config_lookup_int(cfg, "port");

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, host, &(addr.sin_addr)) != 1) {
        fprintf(stderr, "v4 inet_pton fail for %s\n", host);
        return EXIT_FAILURE;
    }

    struct MHD_Daemon* daemon = MHD_start_daemon(
        MHD_USE_SELECT_INTERNALLY | MHD_USE_DEBUG, htons(port), NULL, NULL,
        &answer_to_connection, NULL, MHD_OPTION_NOTIFY_COMPLETED,
        request_completed, NULL, MHD_OPTION_SOCK_ADDR, &addr, MHD_OPTION_END);

    if (daemon == NULL) {
        fprintf(stderr, "failed to setup http server\n");
        return EXIT_FAILURE;
    }

    /*** SETUP SQLITE ***/
    pthread_t loop_thread;

    const char* db_path = config_lookup_string(cfg, "db_path");

    int rc = setup_database(db_path, &db);
    if (rc) {
        fprintf(stderr, "can't open database\n");
        return EXIT_FAILURE;
    }

    rc = pthread_create(&loop_thread, NULL, collect_temperature_thread,
                        (void*)db);
    if (rc) {
        fprintf(stderr, "can't create new thread (reading from device)");
        return EXIT_FAILURE;
    }

    /*** WAIT FOR TERMINATION ***/
    fprintf(stderr, "server successfully started\n");
    getchar();

    /*** CLEANUP ***/
    sqlite3_close(db);
    MHD_stop_daemon(daemon);

    return 0;
}
