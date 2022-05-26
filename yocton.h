
enum yocton_field_type {
	YOCTON_FIELD_STRING,
	YOCTON_FIELD_OBJECT,
};

struct yocton_object;
struct yocton_field;

struct yocton_object *yocton_read_from(FILE *fstream);
void yocton_free(struct yocton_object *obj);

struct yocton_field *yocton_next_field(struct yocton_object *obj);

enum yocton_field_type yocton_field_type(struct yocton_field *f);
const char *yocton_field_name(struct yocton_field *f);
const char *yocton_field_value(struct yocton_field *f);
struct yocton_object *yocton_field_inner(struct yocton_field *f);

