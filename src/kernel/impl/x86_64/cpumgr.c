#include <gsd-common.h>
#include <impl/x86_64/cpux64.h>
static gsd_registry_key* CPUS_KEY; // @r:/cs/cpus/0000
static gsd_procid(*cpu_get_self)();
static gsd_registry_key*(_get_cpu_node)(gsd_procid id); // Vector for either legacy LAPIC or X2 
// And look like [Cluster]/[Core]
// X2 would look like
// [0x0000-0xFFFF]/[0x0-0xF] in hex 
// Examples: 
/// 0000/A Processor 10 in Cluster 0
/// FFA0/1 Processor 1 in Cluster 0xFFA0
// etc... 
// Legacy would look like 
// [0-9]|[A-E]/[0-3] 
// Example:
// A/1 -> Cpu 1 in Cluster 10
// E/3 -> Cpu 3 in cluster 14
gsd_registry_key* _legacy_GetCpuNode(gsd_procid id) {

}

gsd_registry_key* x2_GetCpuNode(gsd_procid id) {

}

gsd_registry_key* GetCpuNode(gsd_procid id) {
        return _get_cpu_node(id);
}

void CpuGetSelf() {

}

void Cpumgr_Initalise() {
        // Check if x2 is supported if so use that 
        // Otherwise We will use Legacy 

}
