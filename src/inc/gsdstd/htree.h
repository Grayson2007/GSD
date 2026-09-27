#include "stdtype.h"


typedef struct s_htree {
        struct s_htree* parent;
        union {
                struct {
                        struct s_htree* left;
                        struct s_htree* right;
                };
                struct s_htree* children[2];
        };
        handle ref;
        ulong key;
        int height;
} htree;
