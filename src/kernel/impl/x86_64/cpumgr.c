#include <gsd-common.h>
#include <impl/x86_64/cpux64.h>
#include <impl/x86_64/asm.h>

static gsd_registry_key* CPUS_KEY; 
static gsd_procid(*cpu_get_self)();
static gsd_registry_key*(*_get_cpu_node)(gsd_procid id); // Vector for either legacy LAPIC or X2 
static volatile int _ready; 
gsd_registry_key* _legacy_GetCpuNode(gsd_procid id) {

}

gsd_registry_key* x2_GetCpuNode(gsd_procid id) {

}
static gsd_procid _legacy_getself() {

}

static gsd_procid _x2_getself() {

}

gsd_registry_key* GetCpuNode(gsd_procid id) {
        return _get_cpu_node(id);
}

void AddCpu() {

}

void Cpumgr_Initalise() {
        // Check if x2 is supported if so use that 
        // Otherwise We will use Legacy 
        u32 _cpuid_data[4];
        _cpuid(1,(u32*)&_cpuid_data);
        cpu_get_self = _cpuid_data[3] & (1 << 21) ? _x2_getself : _legacy_getself;
        _ready = 0;
        
}

