// moduleugps.c
#include "py/runtime.h"
#include "py/obj.h"
#include "nmea.h"
#include "ublox.h"

extern const mp_obj_type_t mp_type_ublox;
extern const mp_obj_type_t mp_type_nmea;

static const mp_rom_map_elem_t ugps_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_ugps)},
    {MP_ROM_QSTR(MP_QSTR_ublox), MP_ROM_PTR(&mp_type_ublox)},
    {MP_ROM_QSTR(MP_QSTR_nmea), MP_ROM_PTR(&mp_type_nmea)},
};

static MP_DEFINE_CONST_DICT(ugps_globals, ugps_globals_table);

const mp_obj_module_t ugps_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t*)&ugps_globals,
};
MP_REGISTER_MODULE(MP_QSTR_ugps, ugps_module);