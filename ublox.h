// ublox.h
#ifndef UBLOX_H
#define UBLOX_H

#include "py/obj.h"
#include "tools.h"

// Экспортируем тип класса для регистрации в moduleugps.c
extern const mp_obj_type_t mp_type_ublox;

// Полноценное описание структуры объекта класса ublox
typedef struct {
    mp_obj_base_t base; // Обязательный базовый тип MicroPython
    mp_obj_t uart;      // Ссылка на объект machine.UART из Python
    mp_obj_t readline;  // Кэшированный Си-указатель на метод .readline
    gps_data_t data;    // Внутренние навигационные данные из tools.h
} mp_obj_ublox_t;

#endif // UBLOX_H