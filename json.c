#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "json.h"

/* Buffer */
typedef struct{
    char *base;
    unsigned int len;
    unsigned int cur;
    unsigned int whisker;
}json_buffer;
#define BUFFER_CUR(b) ((b)->base+(b)->cur)
#define BUFFER_STEP(b, n)\
    if((b)->cur <= (b)->len){(b)->cur += (n);}
#define BUFFER_PEEP(b, n) ((b)->base + n)
#define BUFFER_SKIP(b)\
    while(((b)->base+(b)->cur) && (unsigned char)*((b)->base+(b)->cur) <= 32){ \
        (b)->cur++; \
    }
#define BUFFER_SNIFF(b, c) \
        (b)->whisker = (b)->cur;\
        while(((b)->base+(b)->whisker) && (unsigned char)*((b)->base+(b)->whisker) != (c)\
              && (unsigned char)*((b)->base+(b)->whisker-1) != ('\\')){  \
            (b)->whisker++; }
#define BUFFER_DISTANCE(b) ((b)->whisker - (b)->cur)
#define BUFFER_ADVANCE(b) (b)->cur += (b)->whisker - (b)->cur + 1;
/* end of Buffer */

/* parser Footprint */
typedef enum{
    IDLE,
    IN_OBJECT,
    IN_STRING,
    IN_ARRAY
}json_state;

#define MAXDEPTH 128
struct footprint{
    json_state state[MAXDEPTH];
    unsigned int top;
};

static struct footprint cookie = {{0}, 0};
/* end of Footprint */

/* we use glibc's maolloc and free currently */
#define json_malloc(a) malloc(a)
#define json_realloc(a, b) realloc(a, b)
#define json_release(a) free(a)

/* malloc object & free object functions */
static json_result json_object_init(json_object **object);
static json_result json_array_init(json_array **array);
static json_result json_object_free(json_object **object);
static json_result json_array_free(json_array **arr);

/* parser functions */
static json_result do_parse(json_buffer *buf, json_t *root);
static json_result json_parse_object(json_buffer *buf, json_object **object);
static json_result json_parse_array(json_buffer *buf, json_array **array);
static json_result json_parse_number(json_buffer *buf, json_number *num);
static json_result json_parse_string(json_buffer *buf, char **str);

/* dump functions */
static inline json_result string_dump(FILE *out, char *str);
static json_result json_object_dump(json_object *object, FILE *out, unsigned int depth);
static json_result json_array_dump(json_array *arr, FILE *out, unsigned int depth);

/* json apis */
static inline json_result json_object_find(json_object *object, char *key, json_object **obj);

/* ================================================== */
json_result json_init(json_t **root)
{
    json_result ret;
    json_t *obj;

    obj = json_malloc(sizeof(json_t));
    if(!obj){
        ret = JSON_NO_MEMORY;
        return ret;
    }

    obj->object = NULL;
    obj->array = NULL;

    *root = obj;

    ret = JSON_SUCCESS;
}

static json_result json_object_init(json_object **object)
{
    json_object *obj;
    
    obj = json_malloc(sizeof(json_object));
    if(!obj){
        return JSON_NO_MEMORY;
    }

    obj->key = NULL;
    obj->type = JSON_OBJECT;
    obj->value.obj = NULL;
    obj->next = NULL;

    *object = obj;
    
    return JSON_SUCCESS;
}

static json_result json_array_init(json_array **array)
{
    json_array *arr;
    
    arr = json_malloc(sizeof(json_array));
    if(!arr){
        return JSON_NO_MEMORY;
    }

    arr->type = JSON_ARRAY;
    arr->value.obj = NULL;
    arr->next = NULL;

    *array = arr;
    
    return JSON_SUCCESS;
}

json_result json_free(json_t *root)
{
    if(!root)
        return JSON_SUCCESS;
    if(root->object)
        json_object_free(&root->object);
    else
        json_array_free(&root->array);

    json_release(root);
    root = NULL;
    return JSON_SUCCESS;
}

static json_result json_object_free(json_object **object)
{
    json_object *obj = *object;

    if(!obj)
        return JSON_SUCCESS;
    
    if(obj->key)
        json_release(obj->key);
    switch(obj->type){
    case JSON_OBJECT:
        json_object_free(&obj->value.obj);
        break;
    case JSON_ARRAY:
        json_array_free(&obj->value.arr);
        break;
    case JSON_STRING:
        json_release(obj->value.string);
        break;
    }
    if(obj->next)
        json_object_free(&obj->next);

    json_release(obj);
    *object = NULL;

    return JSON_SUCCESS;
}

static json_result json_array_free(json_array **array)
{
    json_array *arr = *array;
    
    if(!arr)
        return JSON_SUCCESS;

    switch(arr->type){
    case JSON_OBJECT:
        json_object_free(&arr->value.obj);
        break;
    case JSON_ARRAY:
        json_array_free(&arr->value.arr);
        break;
    case JSON_STRING:
        json_release(arr->value.string);
        break;
    }
    if(arr->next)
        json_array_free(&arr->next);

    json_release(arr);
    *array = NULL;

    return JSON_SUCCESS;

}

/* ================================================= */
json_result json_parser(char *str, json_t **object){
    json_result ret;
    json_buffer *buf;
    json_t *root;

    buf = json_malloc(sizeof(json_buffer));
    buf->base = str;
    buf->len  = strlen(str);
    buf->cur = 0;

    json_init(&root);
    ret = do_parse(buf, root);
    if(ret != JSON_SUCCESS)
        json_free(root);

    *object = root;

    json_release(buf);
    return ret;
}

static json_result do_parse(json_buffer *buf, json_t *root)
{
    int ch;
    json_result ret;
    json_object *obj;
    json_array *arr;

    cookie.top = 0;
    
    BUFFER_SKIP(buf);
    ch = *(BUFFER_CUR(buf));
    switch(*(BUFFER_CUR(buf))){
    case '{':
        ret = json_parse_object(buf, &obj);
        if(ret == JSON_SUCCESS)
            root->object = obj;
        break;
    case '[':
        ret = json_parse_array(buf, &arr);
        if(ret == JSON_SUCCESS)
            root->array = arr;
        break;
    default:
        ret = JSON_WRONG_FORMAT;
        break;
    }

    return ret;
}

static json_result json_parse_object(json_buffer *buf, json_object **object)
{
    json_result ret;
    json_object *obj;
    json_object *first = NULL;

    BUFFER_SKIP(buf);
    if(cookie.top == 0 || cookie.state[cookie.top-1] != IN_OBJECT){
        if(*(BUFFER_CUR(buf)) != '{'){
            return JSON_WRONG_FORMAT;
        }
        cookie.state[cookie.top++] = IN_OBJECT;
    }
    
    BUFFER_STEP(buf, 1);
    BUFFER_SKIP(buf);
    while(*(BUFFER_CUR(buf)) != '}'){
        ret = json_object_init(&obj);
        if(ret != JSON_SUCCESS){
            return ret;
        }
        BUFFER_SKIP(buf);
        if(*(BUFFER_CUR(buf)) != '"'){
            ret = JSON_WRONG_FORMAT;
            goto failure;
        }

        json_parse_string(buf, &obj->key);

        BUFFER_SKIP(buf);
        if(*(BUFFER_CUR(buf)) != ':'){
            ret = JSON_WRONG_FORMAT;
            goto failure;
        }
    
        BUFFER_STEP(buf, 1);
        BUFFER_SKIP(buf);
        switch(*(BUFFER_CUR(buf))){
        case '{':
            ret = json_parse_object(buf, &obj->value.obj);
            if(ret != JSON_SUCCESS){
                goto failure;
            }
            obj->type = JSON_OBJECT;
            break;
        case '[':
            ret = json_parse_array(buf, &obj->value.arr);
            if(ret != JSON_SUCCESS){
                goto failure;
            }
            obj->type = JSON_ARRAY;
            break;
        case '"':
            ret = json_parse_string(buf, &obj->value.string);
            if(ret != JSON_SUCCESS){
                goto failure;
            }
            obj->type = JSON_STRING;
            break;
        case '0': case '1': case '2': case '3': case '4': case '5':
        case '6': case '7': case '8': case '9': case '-':
            ret = json_parse_number(buf, &obj->value.num);
            if(ret != JSON_SUCCESS){
                goto failure;
            }
            obj->type = obj->value.num.type;
            break;
        case 't':
            if(!strncmp("true", BUFFER_CUR(buf), 4)){
                obj->type = JSON_TRUE;
                BUFFER_STEP(buf, 4);
            }else{
                ret = JSON_WRONG_FORMAT;
                goto failure;
            }
            break;
        case 'f':
            if(!strncmp("false", BUFFER_CUR(buf), 5)){
                obj->type = JSON_FALSE;
                BUFFER_STEP(buf, 5);
            }else{
                ret = JSON_WRONG_FORMAT;
                goto failure;
            }
            break;
        case 'n':
            if(!strncmp("null", BUFFER_CUR(buf), 4)){
                obj->type = JSON_NULL;
                BUFFER_STEP(buf, 4);
            }else{
                ret = JSON_WRONG_FORMAT;
                goto failure;
            }
            break;
        default:
            ret = JSON_WRONG_FORMAT;
            goto failure;
        }
        
        if(first == NULL)
            first = obj;
        else {
            obj->next = first->next;
            first->next = obj;
        }
        
        BUFFER_SKIP(buf);
        if(*(BUFFER_CUR(buf)) == ','){
            BUFFER_STEP(buf, 1);
            BUFFER_SKIP(buf);
        }
    }

    if(*(BUFFER_CUR(buf)) == '}'){
        BUFFER_STEP(buf, 1);
        cookie.top--;
        *object = first;
        return JSON_SUCCESS;
    }else{
        ret = JSON_WRONG_FORMAT;
        goto failure;
    }
        
failure:
    json_object_free(&obj);
    if(first)
        json_object_free(&first);
    return ret;
}

static json_result json_parse_array(json_buffer *buf, json_array **array)
{
    json_array *arr;
    json_array *first = NULL;
    json_result ret;

    BUFFER_SKIP(buf);
    if(cookie.top == 0 || cookie.state[cookie.top-1] != IN_ARRAY){
        if(*(BUFFER_CUR(buf)) != '[')
            return JSON_WRONG_FORMAT;
        cookie.state[cookie.top++] = IN_ARRAY;
    }


    BUFFER_STEP(buf, 1);
    BUFFER_SKIP(buf);
    while(*(BUFFER_CUR(buf)) != ']'){
        ret = json_array_init(&arr);
        if(ret != JSON_SUCCESS){
            return ret;
        }

        BUFFER_SKIP(buf);
        switch(*(BUFFER_CUR(buf))){
        case '{':
            ret = json_parse_object(buf, &arr->value.obj);
            if(ret != JSON_SUCCESS){
                goto failure;
            }
            arr->type = JSON_OBJECT;
            break;
        case '[':
            ret = json_parse_array(buf, &arr->value.arr);
            if(ret != JSON_SUCCESS){
                goto failure;
            }
            arr->type = JSON_ARRAY;
            break;
        case '"':
            ret = json_parse_string(buf, &arr->value.string);
            if(ret != JSON_SUCCESS){
                goto failure;
            }
            arr->type = JSON_STRING;
            break;
        case '0': case '1': case '2': case '3': case '4': case '5':
        case '6': case '7': case '8': case '9': case '-':
            ret = json_parse_number(buf, &arr->value.num);
            if(ret != JSON_SUCCESS){
                goto failure;
            }
            arr->type = arr->value.num.type;
            break;
        case 't':
            if(!strncmp("true", BUFFER_CUR(buf), 4)){
                arr->type = JSON_TRUE;
                BUFFER_STEP(buf, 4);
            }else{
                ret = JSON_WRONG_FORMAT;
                goto failure;
            }
            break;
        case 'f':
            if(!strncmp("false", BUFFER_CUR(buf), 5)){
                arr->type = JSON_FALSE;
                BUFFER_STEP(buf, 5);
            }else{
                ret = JSON_WRONG_FORMAT;
                goto failure;
            }
            break;
        case 'n':
            if(!strncmp("null", BUFFER_CUR(buf), 4)){
                arr->type = JSON_NULL;
                BUFFER_STEP(buf, 4);
            }else{
                ret = JSON_WRONG_FORMAT;
                goto failure;
            }
            break;
        default:
            ret = JSON_WRONG_FORMAT;
            goto failure;
        }

        if(first == NULL)
            first = arr;
        else{
            arr->next = first->next;
            first->next = arr;
        }
        
        BUFFER_SKIP(buf);
        if(*(BUFFER_CUR(buf)) == ','){
            BUFFER_STEP(buf, 1);
            BUFFER_SKIP(buf);
        }
    }

    if(*(BUFFER_CUR(buf)) == ']'){
        BUFFER_STEP(buf, 1);
        cookie.top--;
        *array = first;
        return JSON_SUCCESS;
    }else{
        ret = JSON_WRONG_FORMAT;
        goto failure;
    }

failure:
    json_array_free(&arr);
    if(first)
        json_array_free(&first);
    return ret;
}

static json_result json_parse_number(json_buffer *buf, json_number *num)
{
    unsigned int whisker;
    unsigned int n;
    char *tmp;

    if(*(BUFFER_CUR(buf)) == '-')
        whisker = buf->cur + 1;
    else 
        whisker = buf->cur;

    while(isdigit(*(BUFFER_PEEP(buf, whisker))) || *(BUFFER_PEEP(buf, whisker)) == '.' \
          || *(BUFFER_PEEP(buf, whisker)) == 'E' || *(BUFFER_PEEP(buf, whisker)) == 'e')
        whisker++;

    n = whisker - buf->cur;
    tmp = json_malloc(sizeof(char) * (n + 1));
    if(!tmp){
        return JSON_NO_MEMORY;
    }
    memset(tmp, 0, (n + 1));
    memcpy(tmp, BUFFER_CUR(buf), n);
    
    if(strchr(tmp, '.') || strchr(tmp, 'e') || strchr(tmp, 'E')){   /* double */
        num->type = JSON_DOUBLE;
        num->dnum = atof(tmp);
    }else{
        num->type = JSON_INT;
        num->num = atol(tmp);
    }

    BUFFER_STEP(buf, n);

    json_release(tmp);
    return JSON_SUCCESS;
}

static json_result json_parse_string(json_buffer *buf, char **str)
{
    int n;
    char *ch;
        
    BUFFER_STEP(buf, 1);
    BUFFER_SNIFF(buf, '"');
    n = BUFFER_DISTANCE(buf);
    
    ch = json_malloc(sizeof(char) * (n + 1));
    memset(ch, 0, (n + 1));
    memcpy(ch, BUFFER_CUR(buf), n);
    BUFFER_ADVANCE(buf);

    *str = ch;
    
    return JSON_SUCCESS;
}

static inline json_result json_string_dump(FILE *out, char *str)
{
    fprintf(out, "\"");
    fprintf(out, "%s", str);
    fprintf(out, "\"");

    return JSON_SUCCESS;
}

static json_result json_object_dump(json_object *object, FILE *out, unsigned int depth)
{
    int i;

    if(cookie.top == 0 || cookie.state[cookie.top-1] != IN_OBJECT){
        for(i = 0; i < depth; i++){
            fprintf(out, "\t");
        }

        fprintf(out, "{\n");
        cookie.state[cookie.top++] = IN_OBJECT;
    }
    for(i = 0; i< depth + 1; i++){
        fprintf(out, "\t");        
    }

    if(object->key)
        json_string_dump(out, object->key);

    fprintf(out, ":");
    
    switch(object->type){
    case JSON_OBJECT:
        fprintf(out, "\n");
        cookie.state[cookie.top++] = IN_STRING;
        json_object_dump(object->value.obj, out, depth + 1);
        cookie.top--;
        break;
    case JSON_ARRAY:
        fprintf(out, "\n");
        json_array_dump(object->value.arr, out, depth + 1);
        break;
    case JSON_STRING:
        json_string_dump(out, object->value.string);
        break;
    case JSON_DOUBLE:
        fprintf(out, "%f", object->value.num.dnum);
        break;
    case JSON_INT:
        fprintf(out, "%d", object->value.num.num);
        break;
    case JSON_TRUE:
        fprintf(out, "true");
        break;
    case JSON_FALSE:
        fprintf(out, "false");
        break;
    case JSON_NULL:
        fprintf(out, "null");
        break;
    }

    if(object->next){
        fprintf(out, ",\n");
        json_object_dump(object->next, out, depth);
    }else{
        fprintf(out, "\n");
        for(i = 0; i < depth; i++)
            fprintf(out, "\t");
        fprintf(out, "}");
        cookie.top--;
    }
}

static json_result json_array_dump(json_array *arr, FILE *out, unsigned int depth)
{
    int i;

    if(cookie.top == 0 || cookie.state[cookie.top-1] != IN_ARRAY){
        for(i = 0; i < depth; i++){
            fprintf(out, "\t");
        }

        fprintf(out, "[\n");
        cookie.state[cookie.top++] = IN_ARRAY;
    }
    for(i = 0; i< depth + 1; i++){
        fprintf(out, "\t");
    }

    switch(arr->type){
    case JSON_OBJECT:
        fprintf(out, "\n");
        json_object_dump(arr->value.obj, out, depth + 1);
        break;
    case JSON_ARRAY:
        fprintf(out, "\n");
        cookie.state[cookie.top++] = IN_STRING;
        json_array_dump(arr->value.arr, out, depth + 1);
        cookie.top--;
        break;
    case JSON_STRING:
        json_string_dump(out, arr->value.string);
        break;
    case JSON_DOUBLE:
        fprintf(out, "%f", arr->value.num.dnum);
        break;
    case JSON_INT:
        fprintf(out, "%d", arr->value.num.num);
        break;
    case JSON_TRUE:
        fprintf(out, "true");
        break;
    case JSON_FALSE:
        fprintf(out, "false");
        break;
    case JSON_NULL:
        fprintf(out, "null");
        break;
    }

    if(arr->next){
        fprintf(out, ",\n");
        json_array_dump(arr->next, out, depth);
    }else{
        fprintf(out, "\n");
        for(i = 0; i < depth; i++)
            fprintf(out, "\t");
        fprintf(out, "]");
        cookie.top--;
    }
}

json_result json_dump(json_t *root, FILE *out)
{
    if(!root)
        return JSON_SUCCESS;

    cookie.top = 0;
    if(root->object){
        json_object_dump(root->object, out, 0);
    }
    else if(root->array){
        json_array_dump(root->array, out, 0);
    }
    else 
        return JSON_EMPTY;
}

/* json API */
static inline json_result json_object_find(json_object *object, char *key, json_object **obj)
{
    json_result ret;
    json_object *oo;
    
    if(object == NULL)
        return JSON_EMPTY;

    oo = object;
    while(strncmp(oo->key, key, strlen(key)))
        oo = oo->next;
    if(!oo)
        return JSON_KEY_NOT_EXISTS;

    *obj = oo;
    return JSON_SUCCESS;
}

json_result json_object_get_number(json_object *object, char *key, long int *value)
{
    long int val;
    json_object *obj;
    json_result ret;

    ret = json_object_find(object, key, &obj);
    if(ret != JSON_SUCCESS)
        return ret;
    if(obj->type != JSON_INT)
        return JSON_WRONG_TYPE;

    val = obj->value.num.num;
    *value = val;
    
    return JSON_SUCCESS;
}

json_result json_object_get_double(json_object *object, char *key, double *value)
{
    double val;
    json_object *obj;
    json_result ret;
    
    ret = json_object_find(object, key, &obj);
    if(ret != JSON_SUCCESS)
        return ret;
    if(obj->type != JSON_DOUBLE)
        return JSON_WRONG_TYPE;

    val = obj->value.num.dnum;
    *value = val;
    
    return JSON_SUCCESS;
}

json_result json_object_get_string(json_object *object, char *key, char **value)
{
    json_object *obj;
    json_result ret;

    ret = json_object_find(object, key, &obj);
    if(ret != JSON_SUCCESS)
        return ret;
    if(obj->type != JSON_STRING)
        return JSON_WRONG_TYPE;

    *value = obj->value.string;
    
    return JSON_SUCCESS;
}

json_result json_object_get_object(json_object *object, char *key, json_object **value)
{
    json_object *obj;
    json_result ret;
    
    ret = json_object_find(object, key, &obj);
    if(ret != JSON_SUCCESS)
        return ret;
    if(obj->type != JSON_OBJECT)
        return JSON_WRONG_TYPE;

    *value = obj->value.obj;
    
    return JSON_SUCCESS;
}

json_result json_object_get_array(json_object *object, char *key, json_array **value)
{
    json_object *obj;
    json_result ret;

    ret = json_object_find(object, key, &obj);
    if(ret != JSON_SUCCESS)
        return ret;
    if(obj->type != JSON_ARRAY)
        return JSON_WRONG_TYPE;

    *value = obj->value.arr;
    
    return JSON_SUCCESS;
}

char *json_error_str(json_result ret)
{
    switch(ret){
    case JSON_SUCCESS:
        return "success";
    case JSON_WRONG_FORMAT:
        return "wrong json format";
    case JSON_NO_MEMORY:
        return "no enough memory";
    case JSON_EMPTY:
        return "empty value";
    case JSON_WRONG_TYPE:
        return "wrong object type";
    case JSON_KEY_NOT_EXISTS:
        return "key not exists";
    default:
        return "unknown error";
    }
}
