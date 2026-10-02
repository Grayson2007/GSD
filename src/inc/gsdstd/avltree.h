#include "stdtype.h"

typedef ulong avl_key;
typedef struct s_avl_tree {
        struct s_avl_tree* parent;
        handle ref;
        avl_key key;
        long height;
        union {
                struct {
                        struct s_avl_tree* left;
                        struct s_avl_tree* right;
                };
                struct s_avltree* children[2];
        };
        
} avltree;


