#include "pico_diag.h"
#include "main.h"
#include "i2c.h"
#include "cmsis_os.h"
#include "usbd_cdc_if.h"
#include <stddef.h>
#include <string.h>

#define RECORD_MAGIC 0x31444350U
#define PACKET_MAGIC 0x31445050U
#define RING_SIZE 32U
#define EEPROM_ADDR (0x50U << 1)
#define SLOT0 0x80U
#define SLOT1 0xA0U

typedef struct {
    uint32_t magic, sequence, stage, arg, tick, reset, fault, checksum;
} Record;
typedef struct { uint32_t tick, stage, arg; } Event;
typedef struct {
    uint32_t magic, version, size, reset, storage;
    Record saved, recovered, current;
    uint32_t count;
    Event events[RING_SIZE];
    uint32_t checksum;
} Packet;

_Static_assert(sizeof(Record) == 32, "Persistent record layout");
_Static_assert(sizeof(Packet) <= APP_TX_DATA_SIZE, "USB diagnostic packet size");
static Record saved, recovered, current;
static Event ring[RING_SIZE];
static Record boot_record;
static Event boot_ring[RING_SIZE];
static uint32_t boot_count;
static bool boot_frozen, requested_boot;
static Packet packet;
static uint32_t ring_total, reset_flags, storage_status;
static uint8_t saved_slot = SLOT1;
static bool backup_ready, armed;
static volatile bool requested;
static uint32_t requested_at;

static uint32_t checksum(const void *data, uint32_t size)
{
    const uint8_t *p = data;
    uint32_t value = 2166136261U;
    while (size--) value = (value ^ *p++) * 16777619U;
    return value;
}

static bool valid(const Record *r)
{
    return r->magic == RECORD_MAGIC &&
        r->checksum == checksum(r, offsetof(Record, checksum));
}

static volatile uint32_t *backup(unsigned index)
{
    return (volatile uint32_t *)((uintptr_t)&RTC->BKP0R + index * 4U);
}

/* Backup registers survive reset, but are not guaranteed across power loss. */
static void backup_save(void)
{
    if (!backup_ready) return;
    *backup(0) = 0;
    for (unsigned i = 1; i < sizeof(current) / 4U; ++i) {
        uint32_t word;
        memcpy(&word, (const uint8_t *)&current + i * 4U, 4);
        *backup(i) = word;
    }
    __DMB();
    *backup(0) = current.magic;
}

void pico_diag_early_init(void)
{
    reset_flags = RCC->CSR;
    __HAL_RCC_CLEAR_RESET_FLAGS();
    __HAL_RCC_PWR_CLK_ENABLE();
    PWR->CR |= PWR_CR_DBP;
    for (unsigned i = 0; i < 10000U; ++i) {
        if (PWR->CR & PWR_CR_DBP) { backup_ready = true; break; }
    }
    if (backup_ready) {
        for (unsigned i = 0; i < sizeof(recovered) / 4U; ++i) {
            uint32_t word = *backup(i);
            memcpy((uint8_t *)&recovered + i * 4U, &word, 4);
        }
        if (!valid(&recovered)) memset(&recovered, 0, sizeof(recovered));
    }
}

static bool read_record(uint8_t address, Record *r)
{
    return HAL_I2C_Mem_Read(&hi2c1, EEPROM_ADDR, address, I2C_MEMADD_SIZE_8BIT,
        (uint8_t *)r, sizeof(*r), 10) == HAL_OK;
}

static bool write_bytes(uint8_t address, const void *data, unsigned length)
{
    if (HAL_I2C_Mem_Write(&hi2c1, EEPROM_ADDR, address, I2C_MEMADD_SIZE_8BIT,
        (uint8_t *)data, length, 10) != HAL_OK) return false;
    /* Also safe for the older 8-byte-page BL24C02 variant. */
    HAL_Delay(6);
    return true;
}

/* Only before a trial or during boot, never inside PD packet handling.
 * Alternate slots and commit magic last; the previous slot remains valid. */
static bool persist(Record record)
{
    uint8_t target = saved_slot == SLOT0 ? SLOT1 : SLOT0;
    uint32_t zero = 0;
    record.magic = RECORD_MAGIC;
    record.sequence = saved.sequence + 1U;
    record.checksum = checksum(&record, offsetof(Record, checksum));
    int32_t lock = osKernelLock();
    bool ok = write_bytes(target, &zero, 4);
    if (ok) ok = write_bytes(target + 4, (uint8_t *)&record + 4, 4);
    for (unsigned offset = 8; ok && offset < sizeof(record); offset += 8)
        ok = write_bytes(target + offset, (uint8_t *)&record + offset, 8);
    if (ok) ok = write_bytes(target, &record.magic, 4);
    Record verify;
    if (ok) ok = read_record(target, &verify) &&
        memcmp(&record, &verify, sizeof(record)) == 0;
    if (ok) { saved = record; saved_slot = target; }
    /* Boot may already hold a scheduler suspension; do not add another. */
    if (lock == 0) osKernelRestoreLock(0);
    storage_status = ok ? 1U : 2U;
    return ok;
}

void pico_diag_storage_init(void)
{
    Record a = {0}, b = {0};
    bool ra = read_record(SLOT0, &a), rb = read_record(SLOT1, &b);
    bool va = ra && valid(&a), vb = rb && valid(&b);
    storage_status = ra && rb ? 1U : 2U;
    if (va) { saved = a; saved_slot = SLOT0; }
    if (vb && (!va || (int32_t)(b.sequence - a.sequence) > 0)) {
        saved = b;
        saved_slot = SLOT1;
    }
    if (valid(&recovered)) {
        Record record = recovered;
        record.reset = reset_flags;
        if (persist(record)) *backup(0) = 0;
    } else if (valid(&saved) && saved.stage == DIAG_ARM) {
        /* Power may have erased backup RAM. Keep the pre-trial checkpoint
         * explicitly labelled ARM, not as the last instruction executed. */
        Record record = saved;
        record.reset = reset_flags;
        record.stage = DIAG_ARM | 0x100U;
        persist(record);
    }
}

void pico_diag_log(uint32_t stage, uint32_t arg)
{
    if (!armed) return;
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    current.stage = stage;
    current.arg = arg;
    current.tick = HAL_GetTick();
    current.checksum = checksum(&current, offsetof(Record, checksum));
    ring[ring_total % RING_SIZE] = (Event){current.tick, stage, arg};
    ++ring_total;
    backup_save();
    __set_PRIMASK(mask);
}

void pico_diag_arm_ram(void)
{
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    /* Preserve the startup trace once, before manual trials reset the ring. */
    if (armed && !boot_frozen) {
        boot_record = current;
        boot_count = ring_total < RING_SIZE ? ring_total : RING_SIZE;
        uint32_t start = ring_total - boot_count;
        for (uint32_t i = 0; i < boot_count; ++i)
            boot_ring[i] = ring[(start + i) % RING_SIZE];
        boot_frozen = true;
    }
    current = (Record){.magic = RECORD_MAGIC, .sequence = saved.sequence + 1U,
        .reset = reset_flags};
    ring_total = 0;
    armed = true;
    pico_diag_log(DIAG_ARM, 0);
    __set_PRIMASK(mask);
}

void pico_diag_arm(void)
{
    pico_diag_arm_ram();
    if (!persist(current)) pico_diag_log(DIAG_STORAGE_ERROR, 0);
}

void pico_diag_fault(uint32_t exception)
{
    if (!armed) return;
    current.fault = SCB->CFSR;
    pico_diag_log(DIAG_FAULT, (exception & 0xFFU) | (SCB->HFSR & 0xFFFFFF00U));
}

void pico_diag_request(void)
{
    requested_boot = false;
    requested_at = HAL_GetTick();
    requested = true;
}

void pico_diag_boot_request(void)
{
    requested_boot = true;
    requested_at = HAL_GetTick();
    requested = true;
}

bool pico_diag_usb_service(void)
{
    if (!requested) return false;
    if (HAL_GetTick() - requested_at > 1000U) { requested = false; return false; }
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    memset(&packet, 0, sizeof(packet));
    packet.magic = PACKET_MAGIC;
    /* 1 = live trial, 2 = frozen startup, 3 = startup not frozen yet. */
    bool use_boot = requested_boot && boot_frozen;
    packet.version = requested_boot ? (use_boot ? 2U : 3U) : 1U;
    packet.size = sizeof(packet);
    packet.reset = reset_flags;
    packet.storage = storage_status;
    packet.saved = saved;
    packet.recovered = recovered;
    packet.current = use_boot ? boot_record : current;
    packet.count = use_boot ? boot_count :
        (ring_total < RING_SIZE ? ring_total : RING_SIZE);
    uint32_t start = ring_total - packet.count;
    for (uint32_t i = 0; i < packet.count; ++i)
        packet.events[i] = use_boot ? boot_ring[i] : ring[(start + i) % RING_SIZE];
    __set_PRIMASK(mask);
    packet.checksum = checksum(&packet, offsetof(Packet, checksum));
    if (CDC_Transmit_FS((uint8_t *)&packet, sizeof(packet)) == USBD_OK)
        requested = false;
    return true;
}
