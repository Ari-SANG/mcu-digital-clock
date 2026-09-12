/* Power-loss-safe clock and alarm settings stored in the internal EEPROM. */

#include <STC15.H>
#include <intrins.h>
#include "board.h"
#include "config.h"
#include "music.h"
#include "settings.h"

#define IAP_CMD_IDLE           0U
#define IAP_CMD_READ           1U
#define IAP_CMD_PROGRAM        2U
#define IAP_CMD_ERASE          3U
#define IAP_ENABLE_11MHZ       0x83U

#define SETTINGS_SECTOR_SIZE   512U
#define SETTINGS_SECTOR0       0x0000U
#define SETTINGS_SECTOR1       (SETTINGS_SECTOR0 + SETTINGS_SECTOR_SIZE)
#define SETTINGS_SECTOR_COUNT  2U

#define SETTINGS_RECORD_SIZE   20U
#define SETTINGS_SLOT_COUNT    25U
#define SETTINGS_INVALID_ADDR  0xFFFFU
#define SETTINGS_SAVE_RETRIES  3U

#define RECORD_MAGIC0          0x43U
#define RECORD_MAGIC1          0x4BU
#define RECORD_VERSION         1U
#define RECORD_VALID_MARKER    0xA5U
#define RECORD_ENABLED_TAG     0xA0U

#define OFFSET_MAGIC0          0U
#define OFFSET_MAGIC1          1U
#define OFFSET_VERSION         2U
#define OFFSET_RESERVED        3U
#define OFFSET_SEQUENCE_LOW    4U
#define OFFSET_SEQUENCE_HIGH   5U
#define OFFSET_CLOCK_HOUR      6U
#define OFFSET_CLOCK_MINUTE    7U
#define OFFSET_CLOCK_SECOND    8U
#define OFFSET_ALARMS          9U
#define OFFSET_CRC             18U
#define OFFSET_VALID           19U

static unsigned char xdata RecordBuffer[SETTINGS_RECORD_SIZE];
static unsigned char data ActiveSector;
static unsigned char data SaveAttempts;
static unsigned int data NextSequence;
static bit SavePending;

static void Settings_IapIdle(void)
{
    IAP_CONTR = 0;
    IAP_CMD = IAP_CMD_IDLE;
    IAP_TRIG = 0;
    IAP_ADDRH = 0x80U;
    IAP_ADDRL = 0;
}

static unsigned char Settings_IapRead(unsigned int address)
{
    unsigned char data value;
    unsigned char data interrupt_state;

    interrupt_state = EA;
    EA = 0;
    IAP_CONTR = IAP_ENABLE_11MHZ;
    IAP_CMD = IAP_CMD_READ;
    IAP_ADDRL = (unsigned char)address;
    IAP_ADDRH = (unsigned char)(address >> 8);
    IAP_TRIG = 0x5AU;
    IAP_TRIG = 0xA5U;
    _nop_();
    value = IAP_DATA;
    Settings_IapIdle();
    EA = interrupt_state;
    return value;
}

static void Settings_IapProgram(unsigned int address, unsigned char value)
{
    unsigned char data interrupt_state;

    interrupt_state = EA;
    EA = 0;
    IAP_CONTR = IAP_ENABLE_11MHZ;
    IAP_CMD = IAP_CMD_PROGRAM;
    IAP_ADDRL = (unsigned char)address;
    IAP_ADDRH = (unsigned char)(address >> 8);
    IAP_DATA = value;
    IAP_TRIG = 0x5AU;
    IAP_TRIG = 0xA5U;
    _nop_();
    Settings_IapIdle();
    EA = interrupt_state;
}

static void Settings_IapErase(unsigned int address)
{
    unsigned char data interrupt_state;

    interrupt_state = EA;
    EA = 0;
    IAP_CONTR = IAP_ENABLE_11MHZ;
    IAP_CMD = IAP_CMD_ERASE;
    IAP_ADDRL = (unsigned char)address;
    IAP_ADDRH = (unsigned char)(address >> 8);
    IAP_TRIG = 0x5AU;
    IAP_TRIG = 0xA5U;
    _nop_();
    Settings_IapIdle();
    EA = interrupt_state;
}

static unsigned char Settings_Crc8(unsigned char length)
{
    unsigned char data crc;
    unsigned char data value;
    unsigned char data byte_index;
    unsigned char data bit_index;

    crc = 0;
    for (byte_index = 0; byte_index < length; byte_index++)
    {
        value = RecordBuffer[byte_index];
        crc ^= value;
        for (bit_index = 0; bit_index < 8U; bit_index++)
        {
            if (crc & 0x80U) crc = (crc << 1) ^ 0x07U;
            else crc <<= 1;
        }
    }
    return crc;
}

static bit Settings_RecordFieldsValid(void)
{
    unsigned char data index;
    unsigned char data offset;

    if ((RecordBuffer[OFFSET_CLOCK_HOUR] >= 24U) ||
        (RecordBuffer[OFFSET_CLOCK_MINUTE] >= 60U) ||
        (RecordBuffer[OFFSET_CLOCK_SECOND] >= 60U))
        return 0;

    /* Version 1 records used zero here and had all alarms enabled. */
    if ((RecordBuffer[OFFSET_RESERVED] != 0U) &&
        ((RecordBuffer[OFFSET_RESERVED] & ~ALARM_ENABLED_ALL) !=
         RECORD_ENABLED_TAG))
        return 0;

    for (index = 0; index < ALARM_COUNT; index++)
    {
        offset = OFFSET_ALARMS + index * 3U;
        if ((RecordBuffer[offset] >= 24U) ||
            (RecordBuffer[offset + 1U] >= 60U) ||
            (RecordBuffer[offset + 2U] >= MUSIC_COUNT))
            return 0;
    }
    return 1;
}

static bit Settings_ReadRecord(unsigned int address,
                               unsigned int data *sequence)
{
    unsigned char data index;

    if (Settings_IapRead(address + OFFSET_VALID) != RECORD_VALID_MARKER)
        return 0;

    for (index = 0; index < SETTINGS_RECORD_SIZE; index++)
        RecordBuffer[index] = Settings_IapRead(address + index);

    if ((RecordBuffer[OFFSET_MAGIC0] != RECORD_MAGIC0) ||
        (RecordBuffer[OFFSET_MAGIC1] != RECORD_MAGIC1) ||
        (RecordBuffer[OFFSET_VERSION] != RECORD_VERSION) ||
        (RecordBuffer[OFFSET_VALID] != RECORD_VALID_MARKER) ||
        (RecordBuffer[OFFSET_CRC] != Settings_Crc8(OFFSET_CRC)) ||
        !Settings_RecordFieldsValid())
        return 0;

    *sequence = (unsigned int)RecordBuffer[OFFSET_SEQUENCE_LOW] |
                ((unsigned int)RecordBuffer[OFFSET_SEQUENCE_HIGH] << 8);
    return 1;
}

static bit Settings_SequenceNewer(unsigned int candidate,
                                  unsigned int current)
{
    unsigned int data difference;

    difference = candidate - current;
    return (difference != 0U) && (difference < 0x8000U);
}

static bit Settings_RecordErased(unsigned int address)
{
    unsigned char data index;

    for (index = 0; index < SETTINGS_RECORD_SIZE; index++)
    {
        if (Settings_IapRead(address + index) != 0xFFU) return 0;
    }
    return 1;
}

static unsigned int Settings_SectorAddress(unsigned char sector)
{
    return (sector == 0U) ? SETTINGS_SECTOR0 : SETTINGS_SECTOR1;
}

static unsigned int Settings_FindBlankSlot(unsigned char sector)
{
    unsigned char data slot;
    unsigned int data address;

    address = Settings_SectorAddress(sector);
    for (slot = 0; slot < SETTINGS_SLOT_COUNT; slot++)
    {
        if (Settings_RecordErased(address)) return address;
        address += SETTINGS_RECORD_SIZE;
    }
    return SETTINGS_INVALID_ADDR;
}

static void Settings_ApplyRecord(void)
{
    unsigned char data index;
    unsigned char data offset;

    ClockHour = RecordBuffer[OFFSET_CLOCK_HOUR];
    ClockMinute = RecordBuffer[OFFSET_CLOCK_MINUTE];
    ClockSecond = RecordBuffer[OFFSET_CLOCK_SECOND];
    AlarmEnabledMask = (RecordBuffer[OFFSET_RESERVED] == 0U) ?
                       ALARM_ENABLED_ALL :
                       (RecordBuffer[OFFSET_RESERVED] & ALARM_ENABLED_ALL);
    for (index = 0; index < ALARM_COUNT; index++)
    {
        offset = OFFSET_ALARMS + index * 3U;
        AlarmHours[index] = RecordBuffer[offset];
        AlarmMinutes[index] = RecordBuffer[offset + 1U];
        AlarmMelodies[index] = RecordBuffer[offset + 2U];
    }
}

static void Settings_BuildRecord(unsigned int sequence)
{
    unsigned char data index;
    unsigned char data offset;
    unsigned char data interrupt_state;

    RecordBuffer[OFFSET_MAGIC0] = RECORD_MAGIC0;
    RecordBuffer[OFFSET_MAGIC1] = RECORD_MAGIC1;
    RecordBuffer[OFFSET_VERSION] = RECORD_VERSION;
    RecordBuffer[OFFSET_RESERVED] = RECORD_ENABLED_TAG |
                                    AlarmEnabledMask;
    RecordBuffer[OFFSET_SEQUENCE_LOW] = (unsigned char)sequence;
    RecordBuffer[OFFSET_SEQUENCE_HIGH] = (unsigned char)(sequence >> 8);

    interrupt_state = EA;
    EA = 0;
    RecordBuffer[OFFSET_CLOCK_HOUR] = ClockHour;
    RecordBuffer[OFFSET_CLOCK_MINUTE] = ClockMinute;
    RecordBuffer[OFFSET_CLOCK_SECOND] = ClockSecond;
    for (index = 0; index < ALARM_COUNT; index++)
    {
        offset = OFFSET_ALARMS + index * 3U;
        RecordBuffer[offset] = AlarmHours[index];
        RecordBuffer[offset + 1U] = AlarmMinutes[index];
        RecordBuffer[offset + 2U] = AlarmMelodies[index];
    }
    EA = interrupt_state;

    RecordBuffer[OFFSET_CRC] = Settings_Crc8(OFFSET_CRC);
    RecordBuffer[OFFSET_VALID] = RECORD_VALID_MARKER;
}

static bit Settings_WriteRecord(unsigned int address,
                                unsigned int sequence)
{
    unsigned char data index;
    unsigned int data stored_sequence;

    for (index = 0; index < OFFSET_VALID; index++)
        Settings_IapProgram(address + index, RecordBuffer[index]);

    /* Commit last so an interrupted write cannot replace the old record. */
    Settings_IapProgram(address + OFFSET_VALID, RECORD_VALID_MARKER);
    return Settings_ReadRecord(address, &stored_sequence) &&
           (stored_sequence == sequence);
}

static void Settings_SaveFailed(void)
{
    if (++SaveAttempts >= SETTINGS_SAVE_RETRIES)
    {
        SaveAttempts = 0;
        SavePending = 0;
    }
}

void Settings_Init(void)
{
    unsigned char data sector;
    unsigned char data slot;
    unsigned char data found;
    unsigned char data best_sector;
    unsigned int data address;
    unsigned int data sequence;
    unsigned int data best_address;
    unsigned int data best_sequence;

    Settings_IapIdle();
    SavePending = 0;
    SaveAttempts = 0;
    ActiveSector = 0xFFU;
    NextSequence = 0;
    found = 0;
    best_sector = 0;
    best_address = 0;
    best_sequence = 0;

    for (sector = 0; sector < SETTINGS_SECTOR_COUNT; sector++)
    {
        address = Settings_SectorAddress(sector);
        for (slot = 0; slot < SETTINGS_SLOT_COUNT; slot++)
        {
            if (Settings_ReadRecord(address, &sequence))
            {
                if ((!found) || Settings_SequenceNewer(sequence, best_sequence))
                {
                    found = 1;
                    best_sector = sector;
                    best_address = address;
                    best_sequence = sequence;
                }
            }
            address += SETTINGS_RECORD_SIZE;
        }
    }

    if (found && Settings_ReadRecord(best_address, &sequence))
    {
        Settings_ApplyRecord();
        ActiveSector = best_sector;
        NextSequence = best_sequence + 1U;
    }
}

void Settings_RequestSave(void)
{
    SaveAttempts = 0;
    SavePending = 1;
}

void Settings_Service(void)
{
    unsigned char data target_sector;
    unsigned int data address;

    if (!SavePending) return;

    if (ActiveSector < SETTINGS_SECTOR_COUNT)
        address = Settings_FindBlankSlot(ActiveSector);
    else
        address = SETTINGS_INVALID_ADDR;

    if (address == SETTINGS_INVALID_ADDR)
    {
        target_sector = (ActiveSector == 0U) ? 1U : 0U;
        Settings_IapErase(Settings_SectorAddress(target_sector));
        address = Settings_FindBlankSlot(target_sector);
    }
    else
        target_sector = ActiveSector;

    if (address == SETTINGS_INVALID_ADDR)
    {
        Settings_SaveFailed();
        return;
    }

    Settings_BuildRecord(NextSequence);
    if (Settings_WriteRecord(address, NextSequence))
    {
        ActiveSector = target_sector;
        NextSequence++;
        SaveAttempts = 0;
        SavePending = 0;
    }
    else
        Settings_SaveFailed();
}
