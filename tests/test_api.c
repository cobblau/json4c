#include "json.h"

int main()
{
    char *str4 = "{\
      \"Image\": {\
          \"Width\":  800,\
          \"Height\": 600,\
          \"Title\":  \"View from 15th Floor\",\
          \"Thumbnail\": {\
              \"Url\":    \"http://www.example.com/image/481989943\",\
        \"Height\": 125,\
        \"Width\":  \"100\"\
        },\
    \"IDs\": [116, 943, 234, 38793]}}";

    json_t *json;
    json_object *obj;
    json_object *obj1;
    json_array  *arr;
    json_array  *arr_iter;
    json_result ret;
    long int val;
    FILE *fp;

    json_parser(str4, &json);

    /* example : how to get object element */
    obj = json->object;
    ret = json_object_get_object(obj, "Image", &obj1);
    if(ret != JSON_SUCCESS){
        printf("%s\n", json_error_str(ret));
        json_free(json);
        return 0;
    }

    ret = json_object_get_number(obj1, "Width", &val);
    if(ret != JSON_SUCCESS){
        printf("%s\n", json_error_str(ret));
        json_free(json);
        return 0;
    }
    printf("value:%d\n", val);

    /* example : how to get array elements */
    ret = json_object_get_array(obj1, "IDs", &arr);
    if(ret != JSON_SUCCESS){
        printf("%s\n", json_error_str(ret));
        json_free(json);
        return 0;
    }
    arr_iter = arr;
    while(arr_iter){
        printf("%d ", arr_iter->value.num.num);
        arr_iter = arr_iter->next;
    }

    json_free(json);

    close(fp);
    return 0;
}
