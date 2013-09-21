### Introduction

Yes, there are buckets of JSON(http://json.org/) parser. But I found that some of them is so complicated and obscure, though they MAY be perform well. If you wreite a project like Nginx, do you intend to use a JSON parser which has a heap of code?

json4c is a pretty lightweighted and graceful json parser.

### Usage
 in json4c, a JSON is represented as a `json_t`. 
 If JSON is built on a collection of name/value pairs, `json_t->object` works. 
 If JSON is built on an ordered list of values, `json-t->array` works.
 
 Steps to using json4c:
 
 - assign JSON value to `str`, which usually is a `char *`
 - declare json_t: `json_t *json_root`
 - use `json_parse(str, &json_root)`, then json_root stores the JSON value.
 - for a JSON name/value pairs, use `json_object_get_*` functions to get a value from object file; for a JSON ordered list values, use `arr->value.*` to get value directly.
 - use that value
 - use `json_free(json_root)` to free your json_root parser.
 

usage example: see test_api.c
