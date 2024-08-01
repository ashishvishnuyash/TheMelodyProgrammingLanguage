#include "class.h"
#include <stdlib.h>
#include <string.h>

Class* create_class(const char* name, ClassMember* members) {
    Class* cls = malloc(sizeof(Class));
    cls->name = strdup(name);
    cls->members = members;
    cls->member_count = 0;
    return cls;
}

void add_member(Class* cls, MemberType type, const char* name, Literal* value) {
    cls->members = realloc(cls->members, sizeof(ClassMember) * (cls->member_count + 1));
    cls->members[cls->member_count].type = type;
    cls->members[cls->member_count].name = strdup(name);
    cls->members[cls->member_count].value = value;
    cls->member_count++;
}

ClassMember* get_member(Class* cls, const char* name) {
    for (size_t i = 0; i < cls->member_count; i++) {
        if (strcmp(cls->members[i].name, name) == 0) {
            return &cls->members[i];
        }
    }
    return NULL;
}

ClassInstance* create_instance(Class* cls) {
    ClassInstance* instance = malloc(sizeof(ClassInstance));
    instance->cls = cls;
    instance->fields = malloc(sizeof(ClassMember) * cls->member_count);
    instance->field_count = cls->member_count;
    for (size_t i = 0; i < cls->member_count; i++) {
        instance->fields[i].type = MEMBER_FIELD;
        instance->fields[i].name = strdup(cls->members[i].name);
        instance->fields[i].value = NULL;
    }
    return instance;
}
