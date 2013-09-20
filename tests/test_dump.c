#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "json.h"

int main()
{
    json_t *root;
    json_object *object;
    json_object *object1;
    json_object *object2;
    json_array *arr;
    json_array *arr1;
    FILE *fp;
    
    json_root_init(&root);

    json_object_init(&object);
    object->key = json_malloc(sizeof(char) * 4);
    memset(object->key, 0, 4);
    memcpy(object->key, "key", 3);
    
    object->type = JSON_STRING;
    object->value.string = (char *)json_malloc(sizeof(char) * 6);
    memset(object->value.string, 0, 6);
    memcpy(object->value.string, "value", 5);

    root->object = object;

    json_object_init(&object1);
    object1->key = json_malloc(sizeof(char) * 4);
    memset(object1->key, 0, 4);
    memcpy(object1->key, "ids", 3);
    
    object1->type = JSON_STRING;
    object1->value.string = (char *)json_malloc(sizeof(char) * 6);
    memset(object1->value.string, 0, 6);
    memcpy(object1->value.string, "cobin", 5);

    object->next = object1;

    json_object_init(&object2);
    object2->key = json_malloc(sizeof(char) * 4);
    memset(object2->key, 0, 4);
    memcpy(object2->key, "arr", 4);
    
    object2->type = JSON_ARRAY;
    json_array_init(&arr);
    arr->type = JSON_STRING;
    arr->value.string = (char*)json_malloc(sizeof(char) * 6);
    memset(arr->value.string, 0, 6);
    memcpy(arr->value.string, "value", 5);
    object2->value.arr = arr;

    json_array_init(&arr1);
    arr1->type = JSON_STRING;
    arr1->value.string = (char*)json_malloc(sizeof(char) * 6);
    memset(arr1->value.string, 0, 6);
    memcpy(arr1->value.string, "valua", 5);
    arr->next = arr1;

    object1->next = object2;

    fp = fopen("a.out", "w");
    json_dump(root, fp);
    fclose(fp);

    json_free(root);

    return;
}
