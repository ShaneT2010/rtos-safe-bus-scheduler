#ifndef ONE_WIRE_BUS_H
#define ONE_WIRE_BUS_H

#include "FreeRTOS.h"
#include "semphr.h"

#define MAX_BUS_NODES         4
#define ROM_BIT_LENGTH        64

typedef struct {
    uint8_t rom_id[8];         /* 8-bytes (64-bit) Unique ROM ID */
    uint8_t is_virtual;        /* 1 = Emulated in Track A, 0 = Physical Node */
    float   last_temperature;
    float   last_humidity;
} one_wire_node_t;

typedef struct {
    one_wire_node_t nodes[MAX_BUS_NODES];
    uint8_t         discovered_count;
    SemaphoreHandle_t bus_mutex;
    int16_t         last_discrepancy_bit; /* Used for Binary Tree Backtracking */
} one_wire_bus_t;

/* Core 1-Wire Driver API */
void    OneWire_InitBus(one_wire_bus_t *bus);
uint8_t OneWire_SearchROM(one_wire_bus_t *bus);
void    OneWire_InjectVirtualNode(one_wire_bus_t *bus, uint8_t *mock_rom);

#endif /* ONE_WIRE_BUS_H */
