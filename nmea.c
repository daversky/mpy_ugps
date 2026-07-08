// nmea.c
#include "py/runtime.h"
#include "py/obj.h"
#include "nmea.h"
#include "tools.h"
#include <math.h>

static mp_obj_t nmea_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_debug };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_debug,       MP_ARG_BOOL, {.u_bool = false} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    nmea_obj_t *self = mp_obj_malloc(nmea_obj_t, type);
    gps_data_reset(&self->data);  // из tools.h
    return MP_OBJ_FROM_PTR(self);
}

// parse(msg=...)
static mp_obj_t nmea_parse_wrapper(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_msg };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_msg, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    nmea_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    mp_obj_t string_obj = args[ARG_msg].u_obj;

    parse_nmea(string_obj, &self->data);  // из tools.h
    return create_data_dict(&self->data);  // из tools.h
}
static MP_DEFINE_CONST_FUN_OBJ_KW(nmea_parse_obj, 1, nmea_parse_wrapper);

// reset()
static mp_obj_t nmea_reset_wrapper(mp_obj_t self_in) {
    nmea_obj_t *self = MP_OBJ_TO_PTR(self_in);
    gps_data_reset(&self->data);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(nmea_reset_data_obj, nmea_reset_wrapper);


// calc_distance(coord1, coord2)
static mp_obj_t nmea_calc_distance_wrapper(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_coord1, ARG_coord2 };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_coord1, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_coord2, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    double lon1, lat1, lon2, lat2;
    unpack_coord(args[ARG_coord1].u_obj, &lon1, &lat1);
    unpack_coord(args[ARG_coord2].u_obj, &lon2, &lat2);

    double distance = calculate_distance_haversine(lat1, lon1, lat2, lon2);
    return mp_obj_new_int((int)round(distance));
}
static MP_DEFINE_CONST_FUN_OBJ_KW(nmea_calc_distance_obj, 1, nmea_calc_distance_wrapper);


// calc_bearing(coord1, coord2)
static mp_obj_t nmea_calc_bearing_wrapper(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_coord1, ARG_coord2 };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_coord1, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_coord2, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    double lon1, lat1, lon2, lat2;
    unpack_coord(args[ARG_coord1].u_obj, &lon1, &lat1);
    unpack_coord(args[ARG_coord2].u_obj, &lon2, &lat2);

    double bearing = calculate_bearing(lat1, lon1, lat2, lon2);
    return mp_obj_new_int((int)round(bearing));
}
static MP_DEFINE_CONST_FUN_OBJ_KW(nmea_calc_bearing_obj, 1, nmea_calc_bearing_wrapper);


static const mp_rom_map_elem_t nmea_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_parse),          MP_ROM_PTR(&nmea_parse_obj) },
    { MP_ROM_QSTR(MP_QSTR_reset),          MP_ROM_PTR(&nmea_parse_obj) },
    { MP_ROM_QSTR(MP_QSTR_calc_distance),  MP_ROM_PTR(&nmea_calc_distance_obj) },
    { MP_ROM_QSTR(MP_QSTR_calc_bearing),   MP_ROM_PTR(&nmea_calc_bearing_obj) },
};
static MP_DEFINE_CONST_DICT(nmea_locals_dict, nmea_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_nmea,
    MP_QSTR_NMEA,
    MP_TYPE_FLAG_NONE,
    make_new, nmea_make_new,
    locals_dict, &nmea_locals_dict
);