
typedef size_t (*yocton_read)(void *buf, size_t buf_size, void *handle);

enum yocton_field_type {
	YOCTON_FIELD_STRING,
	YOCTON_FIELD_OBJECT,
};

struct yocton_object;
struct yocton_field;

struct yocton_object *yocton_read_with(yocton_read callback, void *handle);
struct yocton_object *yocton_read_from(FILE *fstream);
int yocton_have_error(struct yocton_object *obj, int *lineno,
                      const char **error_msg);
void yocton_free(struct yocton_object *obj);
void yocton_check(struct yocton_object *obj, const char *error_msg,
                  int normally_true);

struct yocton_field *yocton_next_field(struct yocton_object *obj);

enum yocton_field_type yocton_field_type(struct yocton_field *f);
const char *yocton_field_name(struct yocton_field *f);
const char *yocton_field_value(struct yocton_field *f);
struct yocton_object *yocton_field_inner(struct yocton_field *f);

