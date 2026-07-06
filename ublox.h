#ifndef UBLOX_H
#define UBLOX_H
#include "py/obj.h"
#include "tools.h"



typedef gnss_data_t ublox_data_t;

typedef struct {
    mp_obj_base_t base;
    mp_obj_t uart;
    ublox_data_t data;
    bool debug;
} ublox_obj_t;

extern const mp_obj_type_t mp_type_ublox;

#endif // UBLOX_H