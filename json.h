#ifndef JSON_H
#define JSON_H

#include <stdio.h>

typedef enum{
    JSON_OBJECT = 0,
    JSON_ARRAY, 
    JSON_STRING,
    JSON_INT,
    JSON_DOUBLE,
    JSON_TRUE,    /* string "true" */
    JSON_FALSE,   /* string "false" */
    JSON_NULL     /* string "null" */
}json_value_type;

typedef enum{
    JSON_SUCCESS,
    JSON_WRONG_FORMAT,
    JSON_NO_MEMORY,
    JSON_EMPTY,
    JSON_WRONG_TYPE,
    JSON_KEY_NOT_EXISTS,
    JSON_UNKONWN
}json_result;

typedef struct json_number{
    long int num;
    double dnum;
    json_value_type type;
}json_number;

typedef struct json_object{
    char *key;
    
    /* json value */
    json_value_type type;
    union{
        struct json_object *obj;
        struct json_array  *arr;
        char        *string;
        json_number num;
    }value;

    struct json_object *next;
}json_object;

typedef struct json_array{
    /* json value */
    json_value_type type;
    union{
        struct json_object *obj;
        struct json_array  *arr;
        char        *string;
        json_number num;
    }value;

    struct json_array *next;
}json_array;

typedef struct json{
    json_object *object;
    json_array  *array;
}json_t;

/* parser */
json_result json_parser(char *string, json_t **root);
json_result json_dump(json_t *root, FILE *out);

/* object management */
json_result json_init(json_t **root);
json_result json_free(json_t *root);
json_result json_object_get_number(json_object *object, char *key, long int *value);
json_result json_object_get_string(json_object *object, char *key, char **value);
json_result json_object_get_double(json_object *object, char *key, double *value);
json_result json_object_get_object(json_object *object, char *key, json_object **value);
json_result json_object_get_array (json_object *object, char *key, json_array **value);

char *json_error_str(json_result ret);
#endif
