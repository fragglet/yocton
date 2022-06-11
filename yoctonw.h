
typedef int (*yoctonw_write)(void *buf, size_t nbytes, void *handle);

struct yoctonw_object;

struct yoctonw_object *yoctonw_write_with(yoctonw_write callback, void *handle);
struct yoctonw_object *yoctonw_write_to(FILE *fstream);
void yoctonw_free(struct yoctonw_object *obj);

void yoctonw_write_value(struct yoctonw_object *obj, const char *name,
                         const char *value);
struct yoctonw_object *yoctonw_write_inner(struct yoctonw_object *obj,
                                           const char *name);

