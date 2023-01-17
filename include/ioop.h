
#ifndef _IOOP_H
#define _IOOP_H

#include <stdint.h>

extern uint8_t inb(uint16_t port);
extern uint16_t inw(uint16_t port);
extern uint32_t inl(uint16_t port);

extern void outb(uint16_t port, uint8_t data);
extern void outw(uint16_t port, uint16_t data);
extern void outl(uint16_t port, uint32_t data);

#endif
