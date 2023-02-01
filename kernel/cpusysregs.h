//----------------------------------------------------------------------------
//
// Arm64 CPU system registers tools
// Copyright (c) 2023, Thierry Lelegard
// BSD-2-Clause license, see the LICENSE file.
//
// Kernel module interface.
// Can be included in kernel and userland, C or C++.
//
//----------------------------------------------------------------------------

#if !defined(CPUSYSREGS_H)
#define CPUSYSREGS_H 1

// Define an unsigned 64-bit int which is valid in userland and kernel, Linux and macOS.
#if defined(__linux__)

    #include <linux/types.h>
    #include <linux/ioctl.h>
    typedef __u64 csr_u64_t;

#elif defined(__APPLE__)

    #include <mach/mach_types.h>
    typedef u_int64_t csr_u64_t;

#endif

#if defined(__cplusplus)
extern "C" {
#endif


//----------------------------------------------------------------------------
// Definitions of types used to communicate with the kernel.
//----------------------------------------------------------------------------

// Linux kernel module or macOS kernel extension name.
#define CSR_MODULE_NAME "cpusysregs"

// Description of a pair of hi/lo registers for PAC authentication keys.
// Most registers are individually read/written. Some registers, such as
// PAC keys, can be accessed only in pairs.
typedef struct {
    csr_u64_t high;
    csr_u64_t low;
} csr_pair_t;

// This macro checks if PACI and PACD are supported, based on the values of the
// ID_AA64ISAR1_EL1 (API or APA) and ID_AA64ISAR2_EL1 (APA3) system registers.
#define CSR_HAS_PAC(isar1,isar2) (((isar1) & 0x00000FF0) || ((isar2) & 0x0000F000))

// This macro checks if PACGA is supported, based on the values of the
// ID_AA64ISAR1_EL1 (GPI or GPA) and ID_AA64ISAR2_EL1 (GPA3) system registers.
#define CSR_HAS_PACGA(isar1,isar2) (((isar1) & 0xFF000000) || ((isar2) & 0x00000F00))

// This macro checks if BTI (Branch Target Identification) is supported,
// based on the values of the ID_AA64PFR1_EL1 system register.
#define CSR_HAS_BTI(pfr1) ((pfr1) & 0x0F)

// This macro checks if RME (Realm Management Extension) is supported,
// based on the values of the ID_AA64PFR0_EL1 system register.
#define CSR_HAS_RME(pfr0) ((pfr0) & 0x00F0000000000000llu)

// This macro gets the RME version (Realm Management Extension),
// based on the values of the ID_AA64PFR0_EL1 system register.
#define CSR_RME_VERSION(pfr0) (((pfr0) >> 52) & 0x0F)


//----------------------------------------------------------------------------
// Kernel module commands.
// Linux: Use ioctl() on /dev/cpusysregs.
// macOS: Use getsockopt() and setsockopt() on system control cpusysregs.
//----------------------------------------------------------------------------

#if defined(__linux__)

    // Special device for this module.
    #define CSR_DEVICE_PATH "/dev/" CSR_MODULE_NAME

    // IOCTL codes for /dev/cpusysregs
    #define _CSR_IOC_MAGIC         '\xF0'
    #define _CSR_GET(index, type)  _IOR(_CSR_IOC_MAGIC, (index), type)
    #define _CSR_SET(index, type)  _IOW(_CSR_IOC_MAGIC, (index), type)

#elif defined(__APPLE__)

    // Use CSR_MODULE_NAME as system control name.
    // Socket options for system control cpusysregs
    #define _CSR_SO_BASE           0x00AC0000
    #define _CSR_GET(index, type)  (_CSR_SO_BASE + (index))
    #define _CSR_SET(index, type)  (_CSR_SO_BASE + (index))

#endif

// Common system commands.
// Most system registers have get access only.
// Some system registers have get/set access.
#define CSR_CMD_GET_AA64PFR0   _CSR_GET(0,  csr_u64_t)   // Get AArch64 Processor Feature registers 0
#define CSR_CMD_GET_AA64PFR1   _CSR_GET(1,  csr_u64_t)   // Get AArch64 Processor Feature registers 1
#define CSR_CMD_GET_AA64ISAR0  _CSR_GET(2,  csr_u64_t)   // Get AArch64 Instruction Set Attribute Register 0
#define CSR_CMD_GET_AA64ISAR1  _CSR_GET(3,  csr_u64_t)   // Get AArch64 Instruction Set Attribute Register 1
#define CSR_CMD_GET_AA64ISAR2  _CSR_GET(4,  csr_u64_t)   // Get AArch64 Instruction Set Attribute Register 2
#define CSR_CMD_GET_TCR        _CSR_GET(5,  csr_u64_t)   // Get Translation Control Register
#define CSR_CMD_GET_APIAKEY    _CSR_GET(6,  csr_pair_t)  // Get Pointer Authentication Key A for Instruction
#define CSR_CMD_SET_APIAKEY    _CSR_SET(7,  csr_pair_t)  // Set Pointer Authentication Key A for Instruction
#define CSR_CMD_GET_APIBKEY    _CSR_GET(8,  csr_pair_t)  // Get Pointer Authentication Key B for Instruction
#define CSR_CMD_SET_APIBKEY    _CSR_SET(9,  csr_pair_t)  // Set Pointer Authentication Key B for Instruction
#define CSR_CMD_GET_APDAKEY    _CSR_GET(10, csr_pair_t)  // Get Pointer Authentication Key A for Data
#define CSR_CMD_SET_APDAKEY    _CSR_SET(11, csr_pair_t)  // Set Pointer Authentication Key A for Data
#define CSR_CMD_GET_APDBKEY    _CSR_GET(12, csr_pair_t)  // Get Pointer Authentication Key B for Data
#define CSR_CMD_SET_APDBKEY    _CSR_SET(13, csr_pair_t)  // Set Pointer Authentication Key B for Data
#define CSR_CMD_GET_APGAKEY    _CSR_GET(14, csr_pair_t)  // Get Pointer Authentication Generic Key
#define CSR_CMD_SET_APGAKEY    _CSR_SET(15, csr_pair_t)  // Set Pointer Authentication Generic Key


//----------------------------------------------------------------------------
// Definitions of Arm64 system registers and instructions to access them.
// Useful in the kernel only with accessing EL1 registers.
//----------------------------------------------------------------------------
//
// The system registers are described in section D17 of the Arm Architecture Reference Manual.
//
// - To read a system register into a general-purpose register (GPR), use instruction MRS.
//   Example: asm("mrs %0, id_aa64pfr0_el1" : "=r" (result));
// - To write a system register from a general-purpose register (GPR), use instruction MSR.
//   Example: asm("msr id_aa64pfr0_el1, %0" : : "r" (value));
//
// The assembler recognizes the name of system registers which are defined for a given level
// of architecture. For instance, the register apiakeyhi_el1 is defined starting with Armv8.3.
// Using it in an asm directive works only if the compilation option -march=armv8.3-a or higher
// is used. Without an appropriate -march option, the compilation fails.
//
// However this does not work with the following standard use case: compile for a minimum
// architecture level, test at runtime the supported features, access the system register
// only if the corresponding feature is supported. Even though the corresponding MRS or MSR
// instructions are only executed at the required level of architecture, these instructions
// must be compiled for a lower level of architecture, for which the register names are not
// defined.
//
// To solve that case, we provide macros to forge MRS and MSR instructions using the
// numerical identification of the system register, without providing its name to the
// assembler. A system register is defined by 5 short identifiers: op0, op1, CRn, CRm, op2.
// In the MSR and MRS instructions, these identifiers are located at the following bits:
//   [20-19] op0, [18-16] op1, [15-12] CRn, [11-8] CRm, [7-5] op2
//
// The following macro build the encoding of a system register for a MSR or MRS instruction.
// See the description of each system register in section D17 to get its identifiers.
//
#define CSR_SREG(op0, op1, crn, crm, op2) (((op0) << 19) | ((op1) << 16) | ((crn) << 12) | ((crm) << 8) | ((op2) << 5))

//
// The following macros define the encoding of some system registers the names of which
// are not recognized with the default level of architecture.
//
#define CSR_APIAKEYLO_EL1 CSR_SREG(3, 0, 2, 1, 0)
#define CSR_APIAKEYHI_EL1 CSR_SREG(3, 0, 2, 1, 1)
#define CSR_APIBKEYLO_EL1 CSR_SREG(3, 0, 2, 1, 2)
#define CSR_APIBKEYHI_EL1 CSR_SREG(3, 0, 2, 1, 3)
#define CSR_APDAKEYLO_EL1 CSR_SREG(3, 0, 2, 2, 0)
#define CSR_APDAKEYHI_EL1 CSR_SREG(3, 0, 2, 2, 1)
#define CSR_APDBKEYLO_EL1 CSR_SREG(3, 0, 2, 2, 2)
#define CSR_APDBKEYHI_EL1 CSR_SREG(3, 0, 2, 2, 3)
#define CSR_APGAKEYLO_EL1 CSR_SREG(3, 0, 2, 3, 0)
#define CSR_APGAKEYHI_EL1 CSR_SREG(3, 0, 2, 3, 1)

//
// Standard stringification macro (not so standard since we must define it again and again).
//
#define _CSR_STRINGIFY_1(x) #x
#define CSR_STRINGIFY(x) _CSR_STRINGIFY_1(x)

//
// The following macro is a trick to forge instructions with general-purpose registers (GPR),
// In an asm directive, argument references such as "%0" or "%1" are replaced by the compiler
// backed with some GPR names such as "w12" or "x24". To build the instruction code, we need
// to transform "w12" as 12 or "x24" as 24. To do that, we define all possible identifiers for
// all GPR in the form .csr_gpr_wNN or .csr_gpr_xNN. In an asm directive, we use .csr_gpr_%0,
// which will be replaced by .csr_gpr_x24 (if GPR x24 has been allocated), which will be
// replaced by 24. We also define xzr and wzr as x31 (zero register).
//
#define _CSR_DEFINE_GPR             \
    ".irp num,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30\n" \
    ".equ .csr_gpr_x\\num, \\num\n" \
    ".equ .csr_gpr_w\\num, \\num\n" \
    ".endr\n"                       \
    ".equ .csr_gpr_xzr, 31\n"       \
    ".equ .csr_gpr_wzr, 31\n"

//
// Macros to read/write system registers by name.
// The corresponding register names must be known at all levels of architecture.
// Examples:
//    csr_u64_t r;
//    CSR_MRS_STR(r, "id_aa64pfr0_el1");
//    CSR_MSR_STR("id_aa64pfr0_el1", r);
//
#define CSR_MSR_STR(sreg,value)   asm("msr " sreg ", %0" : : "r" (value))
#define CSR_MRS_STR(result,sreg)  asm("mrs %0, " sreg : "=r" (result))

//
// Macros to read/write system registers by identifiers.
// This method works at all levels of architecture.
// Examples:
//    csr_u64_t r;
//    CSR_MSR_NUM(CSR_APIAKEYHI_EL1, r);
//    CSR_MRS_NUM(r, CSR_APIAKEYHI_EL1);
//
#define CSR_MSR_NUM(sreg,value) \
    asm(_CSR_DEFINE_GPR ".inst 0xd5000000|(" CSR_STRINGIFY(sreg) ")|(.csr_gpr_%0)" : : "r" (value))

#define CSR_MRS_NUM(result,sreg) \
    asm(_CSR_DEFINE_GPR ".inst 0xd5200000|(" CSR_STRINGIFY(sreg) ")|(.csr_gpr_%0)" : "=r" (result))

#if defined(__cplusplus)
}
#endif

#endif // CPUSYSREGS_H
