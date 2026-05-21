#include "one_wire_bus.h"
#include <string.h>

void OneWire_InitBus(one_wire_bus_t *bus) {
    bus->discovered_count = 0;
    bus->last_discrepancy_bit = 0;
    bus->bus_mutex = xSemaphoreCreateMutex();
    memset(bus->nodes, 0, sizeof(bus->nodes));
}

void OneWire_InjectVirtualNode(one_wire_bus_t *bus, uint8_t *mock_rom) {
    if (bus->discovered_count < MAX_BUS_NODES) {
        memcpy(bus->nodes[bus->discovered_count].rom_id, mock_rom, 8);
        bus->nodes[bus->discovered_count].is_virtual = 1;
        bus->discovered_count++;
    }
}

/* 
 * Industry Standard 1-Wire Search ROM Algorithm 
 * Returns 1 if another node is found, 0 if search cycle is complete.
 */
uint8_t OneWire_SearchROM(one_wire_bus_t *bus) {
    uint8_t id_bit_number = 1;
    uint8_t last_zero = 0;
    uint8_t rom_byte_number = 0;
    uint8_t rom_byte_mask = 1;
    uint8_t search_result = 0;
    
    uint8_t id_bit, cmp_id_bit;
    uint8_t search_direction = 0;
    
    unsigned char current_rom[8] = {0};

    // Acquire bus hardware lock to freeze context switches during bit-banging
    if (xSemaphoreTake(bus->bus_mutex, portMAX_DELAY) != pdTRUE) return 0;

    // Phase 4 Defensive Window Expansion ---
    // [Track A Defense Trigger]: Preemption is locked out here because
    // a single tick interrupt would break the microsecond bit-read timing.
    taskENTER_CRITICAL();

    // If the last discrepancy bit has converged, we have scanned the entire tree
    if (bus->last_discrepancy_bit >= 0) {
        
        while (id_bit_number <= ROM_BIT_LENGTH) {
            // Read next bit and its complement from physical line / virtual matrix
            // In a real chip, this triggers the GPIO pin read sequences.
            id_bit     = 0; // Mocked line state
            cmp_id_bit = 0; // Mocked line state (00 implies collision)

            if ((id_bit == 1) && (cmp_id_bit == 1)) {
                // No device responding on the line
                break;
            } else {
                if ((id_bit != cmp_id_bit)) {
                    // Normal state: No collision, direction is deterministic
                    search_direction = id_bit;
                } else {
                    // Collision Detected (00)! Traversal decision point
                    if (id_bit_number < bus->last_discrepancy_bit) {
                        // Take the same path as last search cycle
                        search_direction = ((current_rom[rom_byte_number] & rom_byte_mask) > 0);
                    } else {
                        // If equal to last discrepancy, force path to '1'
                        search_direction = (id_bit_number == bus->last_discrepancy_bit);
                    }

                    if (search_direction == 0) {
                        last_zero = id_bit_number;
                    }
                }

                // Write the chosen direction bit to register the path
                if (search_direction == 1) {
                    current_rom[rom_byte_number] |= rom_byte_mask;
                } else {
                    current_rom[rom_byte_number] &= ~rom_byte_mask;
                }

                // Move forward to the next bit in the 64-bit space
                id_bit_number++;
                rom_byte_mask <<= 1;
                if (rom_byte_mask == 0) {
                    rom_byte_number++;
                    rom_byte_mask = 1;
                }
            }
        } // End of 64-bit loop

        // Save trace metrics for the next recursive invocation
        bus->last_discrepancy_bit = last_zero;
        if (bus->last_discrepancy_bit == 0) {
            bus->last_discrepancy_bit = -1; // Tree fully resolved
        }
        search_result = 1;
    }

    taskEXIT_CRITICAL();
    xSemaphoreGive(bus->bus_mutex);
    
    return search_result;
}
