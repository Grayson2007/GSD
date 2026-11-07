#include <impl/x86_64/acpi/acpi.h>
#include <impl/x86_64/acpi/amlinterp.h>
#include <gsd-memory.h>

// todo 

/*

struct s_aml_interp_state {
        u8* cptr;
        u8* endptr;
        struct s_aml_object* scope_object;
        struct s_aml_object* root_object;
        struct s_aml_xctx* exec_ctx;
};
typedef struct s_aml_xctx {
        aml_object* arg[7];
        aml_object local[7];
        struct s_aml_xctx* last_ctx;
        struct s_aml_interp_state* state;
        u8* aml_ip;
} aml_xctx;


static inline int namcmp(char* a,char* b) {
        for(int i = 0; i < 4; i++) {
                if(*a++ > *b++) { return 1;}
                if(*a++ < *b++) { return -1;}
        }
        return 0;
}

static struct s_aml_object* find_child(struct s_aml_object* par,char* child_name) {
        struct s_aml_object* curr = par->this_first_child;
        while(curr) {
                if(namcmp(&curr->name,child_name) != 0) { 
                        curr = curr->par_next_child;
                        continue;
                }
                return curr;
        }
        return NULL;
}


static struct s_aml_object* resolve_name(struct s_aml_interp_state* state) {
        struct s_aml_object* curr_obj = state->scope_object;
        size scope_depth = 0;
        if(*state->cptr >= aml_local_min && *state->cptr <= aml_local_max) {
                if(!state->exec_ctx) { return NULL;}
                int idx = *state->cptr-aml_local_min;
                state->cptr++;
                return &state->exec_ctx->local[idx];
        }

        if(*state->cptr >= aml_arg_min && *state->cptr <= aml_arg_max) {
                 if(!state->exec_ctx) { return NULL;}
                int idx = *state->cptr-aml_arg_min;
                state->cptr++;
                return state->exec_ctx->arg[idx];
        }


        // check if this is actually an object to be resolved 
        if(!((*state->cptr >= 'A' && *state->cptr <= 'F') || *state->cptr == '_' || *state->cptr == aml_prefix_char || *state->cptr != aml_root_char)) 
        {
                return NULL;
        }

        while(*state->cptr == aml_prefix_char) {
                curr_obj = curr_obj->parent;
                state->cptr++;
                if(!curr_obj) { return NULL;}
        }
        if(*state->cptr == aml_root_char) {
                curr_obj = state->root_object;
        }
        if(*state->cptr == aml_dual_name) { scope_depth = 2; state->cptr++;}
        else if(*state->cptr == aml_multi_name) {
                state->cptr++;
                scope_depth = (size)*state->cptr;
                state->cptr++;
        }
        for(index i = 0; i < scope_depth; i++)
        {
                struct s_aml_object* next = find_child(curr_obj,(char*)state->cptr);
                if(!next) { return NULL;}
                curr_obj = next;
        }
        return curr_obj;
}

size get_pkglen(struct s_aml_interp_state* state) {
        u8 lead_byte = *state->cptr;
        size bytes_ahead = (size)(lead_byte >> 6);
        if(!bytes_ahead) { 
                state->cptr++;
                return (size)lead_byte;
        }
        size outsz = (size)(lead_byte & 0xF);
        state->cptr++;
        for(int i = 0; i < bytes_ahead; i++) {
                outsz |= ((size)(*state->cptr) << (4+(i << 3)));
                state->cptr++;
        }
        return outsz;
}

static void aml_to_string(struct s_aml_object_data* dat0,struct s_aml_object_data* result)
{

}

static status aml_eval(struct s_aml_interp_state* state) {
        u8 operation = *state->cptr;
        state->cptr++;
        status s;
        struct s_aml_object_data dat0;
        struct s_aml_object_data dat1;
        struct s_aml_object* dest;
        switch(operation)
        {
                
                        

        }

        
}


static status aml_create_buffer(struct s_aml_interp_state* state) {
        u8* baseptr  = state->cptr;
        size pkg_size = get_pkglen(state);
        u8* endptr = baseptr+pkg_size;
}

#define aml_itos_hex 1
#define aml_itos_dec 0
static status aml_itos(struct s_aml_object_data* input,struct s_aml_object_data* output,int type)
{

}

static status aml_create_object(struct s_aml_interp_state* state)
{
        status s;
        if(*state->cptr == aml_ext) 
        {
                state->cptr++;
                switch(*state->cptr)
                {
                        case aml_ext_bank_feild:


                                break;
                        case aml_ext_create_feild:

                                break;
                        case aml_ext_device:

                                break;

                        default:
                                return gsd_invalid_param;
                }

        }
        else 
        {
                switch(*state->cptr)
                {
                        case aml_buffer:
                                

                        case aml_bfeild:

                        case aml_name:

                        case aml_wfeild:

                        case aml_dfeild:

                        case aml_qfeild:


                        default:
                                return gsd_invalid_param;
                }
        }
        status_assert_ok(s) { return s;}
        return gsd_ok;
}

static status aml_next_object_ext(struct s_aml_interp_state* state) {

}

static status aml_eval_expr() {

}


static status aml_eval_integer(struct s_aml_interp_state* state,u64* integer) {
        switch(*state->cptr) // constant checking
        {
                case aml_byte_pref: 
                        state->cptr++;
                        *integer = (u64)*state->cptr;
                        return gsd_ok;
                case aml_word_pref:
                        state->cptr++;
                        *integer = (u64)*(u16*)state->cptr;
                        return gsd_ok;
                case aml_dword_pref:
                        state->cptr++;
                        *integer = (u64)*(u32*)state->cptr;
                        return gsd_ok;
                case aml_qword_pref:
                        state->cptr++;
                        *integer = *(u64*)state->cptr;
                        return gsd_ok;        
        }


}


static status aml_eval_buffer(struct s_aml_interp_state* state) {
        size pkglen = get_pkglen(state);
        u64 buffsz;
        status s = aml_eval_integer(state,&buffsz);
        
}

static status aml_next_object(struct s_aml_interp_state* state) {
        status s;
        u8 opcode = *state->cptr;
        if(opcode == aml_ext) { 
                state->cptr++;
                return aml_next_object_ext(state);
        }
        switch(opcode) {
                case aml_buffer:
                        s = aml_eval_buffer(state);
                        break;
                case aml_package:
                        break;
                case aml_var_package:

                case aml_extern:

                        break;
        }

        return s;
}

status load_aml_namespace(acpi_table_common_hdr* hdr,struct s_aml_interp_state* state)
{

        size tablesize = hdr->length;
        state->cptr = (u8*)hdr+sizeof(acpi_table_common_hdr);
        state->endptr = (u8*)hdr+tablesize;
        status s;
        while(state->cptr < state->endptr)
        {
                s = aml_next_object(state);
                status_assert_ok(s) { return s;}
        }

        return gsd_ok;
}





status aml_execute(handle ctx) {

}

*/