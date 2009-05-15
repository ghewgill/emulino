#include "eeprom.h"

#include "cpu.h"

#define EEPROM_SIZE 1024

#define EEPROM_EECR     0x3f
#define EEPROM_EEDR     0x40
#define EEPROM_EEARL    0x41
#define EEPROM_EEARH    0x42

#define EEPROM_EECR_EERE    BIT(0)
#define EEPROM_EECR_EEPE    BIT(1)
#define EEPROM_EECR_EEMPE   BIT(2)
#define EEPROM_EECR_EERIE   BIT(3)
#define EEPROM_EECR_EEPM0   BIT(4)
#define EEPROM_EECR_EEPM1   BIT(5)

static u8 EEPROM[EEPROM_SIZE];
static u8 EECR;
static u8 EEDR;
static u16 EEAR;

u8 eeprom_read_eecr(u16 addr)
{
    return EECR;
}

void eeprom_write_eecr(u16 addr, u8 value)
{
    EECR = value;
}

u8 eeprom_read_eedr(u16 addr)
{
    return EEPROM[EEAR];
}

void eeprom_write_eedr(u16 addr, u8 value)
{
    EEDR = value;
}

u8 eeprom_read_eearl(u16 addr)
{
    return EEAR & 0xff;
}

void eeprom_write_eearl(u16 addr, u8 value)
{
    EEAR = (EEAR & ~0xff) | value;
}

u8 eeprom_read_eearh(u16 addr)
{
    return (EEAR & (EEPROM_SIZE-1)) >> 8;
}

void eeprom_write_eearh(u16 addr, u8 value)
{
    EEAR = (EEAR & ~0xff00) | ((value << 8) & (EEPROM_SIZE-1));
}

void eeprom_init()
{
    register_io(EEPROM_EECR, eeprom_read_eecr, eeprom_write_eecr);
    register_io(EEPROM_EEDR, eeprom_read_eedr, eeprom_write_eedr);
    register_io(EEPROM_EEARL, eeprom_read_eearl, eeprom_write_eearl);
    register_io(EEPROM_EEARH, eeprom_read_eearh, eeprom_write_eearh);

    LoadHex("emulino.eeprom", EEPROM, EEPROM_SIZE);
}
