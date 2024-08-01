#ifndef CLASS_H
#define CLASS_H

// #include "../../parser.h"

typedef enum {
    MEMBER_FIELD,
    MEMBER_METHOD
} MemberType;

typedef struct {
    MemberType type;
    char* name;
    ASTNode* value;
} ClassMember;

typedef struct {
    char* name;
    ClassMember* members;
    size_t member_count;
} Class;

typedef struct {
    Class* cls;
    ClassMember* fields;
    size_t field_count;
} ClassInstance;

Class* create_class(const char* name, ClassMember* members);
void add_member(Class* cls, MemberType type, const char* name, Literal* value);
ClassMember* get_member(Class* cls, const char* name);
ClassInstance* create_instance(Class* cls);

#endif /* CLASS_H */
