
/* various register operations */

#ifndef _ROP_H
#define _ROP_H

#include <stdint.h>

#define	MSR_MTRR_CAP		(0xFE)
#define	MSR_MTRR_DEF_TYPE	(0x2FF)
#define MSR_MTRR_PHY_BASE	(0x200)
#define MSR_MTRR_PHY_MASK	(0x201)
#define MSR_EFER		(0xC0000080)
#define MSR_PAT			(0x277)

typedef struct {
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
} cpuid_r;

extern uint8_t inb(uint16_t port);
extern uint16_t inw(uint16_t port);
extern uint32_t inl(uint16_t port);

extern void outb(uint16_t port, uint8_t data);
extern void outw(uint16_t port, uint16_t data);
extern void outl(uint16_t port, uint32_t data);

extern uint64_t rdmsr(uint32_t r);
extern void wrmsr(uint32_t r, uint64_t v);

extern uint64_t rdcr0();
extern uint64_t rdcr1();
extern uint64_t rdcr2();
extern uint64_t rdcr3();
extern uint64_t rdcr4();
extern uint64_t rdcr8();

extern cpuid_r cpuid(uint32_t eax, uint32_t ecx);

extern void invtlb();

extern uint64_t pre_mtrr_change();

extern void post_mtrr_change(uint64_t cr4);

#endif
