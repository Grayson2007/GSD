#ifndef amlinterp_h
#define amlinterp_h
#include "acpi.h"
enum aml_opcodes {
        aml_zero = 0x00,
        aml_one = 0x01,
        aml_alias = 0x06,
        aml_name = 0x08,
        aml_byte_pref = 0x0A,
        aml_word_pref = 0x0B,
        aml_dword_pref = 0x0C,
        aml_str_pref = 0x0D,
        aml_qword_pref = 0x0E,
        aml_scope = 0x10,
        aml_buffer = 0x11,
        aml_package = 0x12,
        aml_var_package = 0x13,
        aml_method = 0x14,
        aml_extern = 0x15,
        aml_dual_name = 0x2E,
        aml_multi_name = 0x2F,
        aml_ext = 0x5B,
        aml_local_min = 0x60,
        aml_local_max = 0x67,
        aml_arg_min,
        aml_arg_max = 0x6E,
        aml_store = 0x70,
        aml_refof = 0x71,
        aml_add = 0x72,
        aml_concat,
        aml_sub,
        aml_inc,
        aml_dec,
        aml_mul,
        aml_div,
        aml_shl,
        aml_shr,
        aml_bitand,
        aml_bitnand,
        aml_bitor,
        aml_bitnor,
        aml_bitxor,
        aml_bitnot,
        aml_bscnl,
        aml_bscnr,
        aml_deref,
        aml_concat_res,
        aml_mod,
        aml_notify,
        aml_sizeof,
        aml_index,
        aml_match,
        aml_dfeild,
        aml_wfeild,
        aml_bfeild,
        aml_bitfeild,
        aml_object_type,
        aml_qfeild,
        aml_and,
        aml_or,
        aml_not,
        aml_equal,
        aml_greater,
        aml_less,
        aml_to_buffer,
        aml_to_dec_str,
        aml_to_hex_str,
        aml_to_int,
        aml_to_str,
        aml_copy_obj,
        aml_mid,
        aml_continue,
        aml_if,
        aml_else,
        aml_while,
        aml_noop,
        aml_return,
        aml_break,
        aml_breakpoint = 0xCC,
        aml_prefix_char = 0x5E,
        aml_root_char = 0x5C, 
        aml_ones = 0xFF
};

enum aml_ext_opcodes {
        aml_ext_mutex = 0x01,
        aml_ext_event = 0x02,
        aml_ext_cond_ref_of = 0x12,
        aml_ext_create_feild = 0x13,
        aml_ext_load_table = 0x1F,
        aml_ext_load = 0x20,
        aml_ext_stall,
        aml_ext_sleep,
        aml_ext_acquire,
        aml_ext_signal,
        aml_ext_wait,
        aml_ext_reset,
        aml_ext_frombcd,
        aml_ext_tobcd,
        aml_ext_revision = 0x30,
        aml_ext_debug,
        aml_ext_fatal,
        aml_ext_timer,
        aml_ext_opregion = 0x80,
        aml_ext_feild,
        aml_ext_device,
        aml_ext_power_res,
        aml_ext_thermal_zone,
        aml_ext_bank_feild,
        aml_ext_data_region,

};





enum aml_types {
        amlt_unitalized,
        amlt_integer,
        amlt_str,
        amlt_buff,
        amlt_package,
        amlt_feild_unit,
        amlt_device,
        amlt_event,
        amlt_mutex,
        amlt_operation_region,
        amlt_power_resource,
        amlt_thermal_zone = 13,
        amlt_buff_feild,
        amlt_debug = 16
};



typedef struct s_aml_object_data {
        enum aml_types type;
        union {
                uintptr_t ptr;
                struct s_aml_object* ref;
                char* str;
                u64 integer;
                mutex mut;

                struct {
                        index bit_idx;
                        size bit_length;
                        struct s_aml_object* buffer_object;
                } bitfeild;

                struct {
                        u8* base;
                        size buffersz;
                        bool is_static;
                } buffer;

                struct {
                        char* str;
                        size len;
                        bool is_static; // if true this string is a ptr in the raw table itself otherwise It's a page allocation  
                } string;
                struct {
                        index bit_index;
                        u8 acc_rules;
                } feild;
                struct {
                        offset byte_offset;
                        index bit_start;
                        size bitlength;
                } feild_unit;

                struct {
                        size len;
                } package;

                struct {
                        offset byte_index;
                        struct s_aml_object* buff;
                } sized_feild;

                struct { 
                        struct s_aml_object* refer;
                } alias;

                struct {
                        index num_args;
                        u8* base_address;
                        size method_size;
                } method;

                
              
        };
} aml_object_data;




typedef struct s_aml_object{

        char name[4];
       
        aml_object_data data;

        // if this is a package member 
        struct {        
                struct s_aml_object* prev;
                struct s_aml_object* next;
        } pkg_links;

        // directory links 
        struct s_aml_object* parent;
        struct s_aml_object* par_next_child;
        struct s_aml_object* par_prev_child;
        struct s_aml_object* this_first_child;
} aml_object;


// execution context 



#endif 
