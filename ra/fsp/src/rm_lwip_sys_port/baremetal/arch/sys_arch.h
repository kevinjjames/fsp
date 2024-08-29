#ifndef LWIP_ARCH_SYS_ARCH_H
#define LWIP_ARCH_SYS_ARCH_H

#include "bsp_api.h"

#define SYS_ARCH_DECL_PROTECT(lev)    FSP_CRITICAL_SECTION_DEFINE;
#define SYS_ARCH_PROTECT(lev)         FSP_CRITICAL_SECTION_ENTER;
#define SYS_ARCH_UNPROTECT(lev)       FSP_CRITICAL_SECTION_EXIT;

#endif                                 /* LWIP_ARCH_SYS_ARCH_H */
