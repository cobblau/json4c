#include "json.h"

int main()
{
    char *str = "{\"id\": \"Open\", \"id2\":\"close\"}";
    char *str1 = "[\"Sunday\", \"Monday\", \"Tuesday\", \"Wednesday\", \"Thursday\", \"Friday\", \"Saturday\"]";
    char *str2 = "[ \
     {\"precision\": \"zip\",\
         \"Latitude\":  37,\
         \"Country\":   null\
      },\
      {\
         \"precision\": \"zip\",\
         \"Latitude\":  37.371991,\
         \"Country\":   \"Beijing\"\
      }\
   ]";

    char *str3 = "{\"widget\": {\
    \"debug\": \"on\",\
    \"window\": {\
        \"title\": \"Sample Konfabulator Widget\",\
        \"name\": \"main_window\",\
        \"width\": 500,\
        \"height\": 500\
    }}}";

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
    FILE *fp;

    json_parser(str1, &json);
    fp = fopen("str1.out", "w");
    json_dump(json, fp);
    json_free(json);
    close(fp);

    json_parser(str2, &json);
    fp = fopen("str2.out", "w");
    json_dump(json, fp);
    json_free(json);
    close(fp);

    json_parser(str3, &json);
    fp = fopen("str3.out", "w");
    json_dump(json, fp);
    json_free(json);
    close(fp);

    json_parser(str4, &json);
    fp = fopen("str4.out", "w");
    json_dump(json, fp);
    json_free(json);
    close(fp);

    return 0;
}

