#include "py/runtime.h"
#include "py/obj.h"
#include "ublox.h"
#include "tools.h"
#include <math.h>
#include <string.h>
#include "py/mphal.h"

static int read_nmea_line(mp_obj_t readline_method, char* nmea_msg, size_t nmea_msg_size) {
    mp_obj_t line = mp_call_function_n_kw(readline_method, 0, 0, NULL);
    if (line == mp_const_none) return 1;

    const char* raw = mp_obj_str_get_str(line);
    if (!raw || strlen(raw) < 6) return 1;

    const char* dollar = strchr(raw, '$');
    if (!dollar) return 1;

    const char* end = dollar;
    while (*end && *end != '\n' && *end != '\r') end++;

    size_t len = end - dollar;
    if (len >= nmea_msg_size) len = nmea_msg_size - 1;

    memcpy(nmea_msg, dollar, len);
    nmea_msg[len] = '\0';

    if (strlen(nmea_msg) < 6) return 1;
    if (!checksum_valid(nmea_msg)) return 1;
    return 0;
}

static mp_obj_t ublox_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_uart, ARG_debug };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_uart,  MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_debug, MP_ARG_BOOL, {.u_bool = false} },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_obj_t uart_obj = args[ARG_uart].u_obj;
    bool debug = args[ARG_debug].u_bool;
    if (debug == true) {
        mp_printf(&mp_plat_print, "[ugps]: enabled debug\n");
    }
    if (uart_obj == mp_const_none || uart_obj == MP_OBJ_NULL) {
        mp_raise_TypeError(MP_ERROR_TEXT("uart argument required"));
    }
    mp_obj_t readline_method = mp_load_attr(uart_obj, MP_QSTR_readline);
    if (readline_method == MP_OBJ_NULL) {
        mp_raise_TypeError(MP_ERROR_TEXT("UART has no readline method"));
    }
    ublox_obj_t *self = m_new_obj(ublox_obj_t);
    self->base.type = &mp_type_ublox;
    self->uart = uart_obj;
    self->debug = debug;
    gps_data_reset(&self->data);
    return MP_OBJ_FROM_PTR(self);
}

// calc_distance(coord)
static mp_obj_t ublox_calc_distance(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_coord };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_coord, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    ublox_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    if (!self->data.is_valid || isnan(self->data.lat) || isnan(self->data.lon)) {
        mp_raise_ValueError(MP_ERROR_TEXT("Current GPS fix is invalid. Cannot calculate distance."));
    }

    double target_lon, target_lat;
    unpack_coord(args[ARG_coord].u_obj, &target_lon, &target_lat);

    double distance = calculate_distance_haversine(self->data.lat, self->data.lon, target_lat, target_lon);
    return mp_obj_new_int((int)round(distance));
}
static MP_DEFINE_CONST_FUN_OBJ_KW(ublox_calc_distance_obj, 1, ublox_calc_distance);

// calc_bearing(coord)
static mp_obj_t ublox_calc_bearing(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_coord };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_coord, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    ublox_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    if (!self->data.is_valid || isnan(self->data.lat) || isnan(self->data.lon)) {
        mp_raise_ValueError(MP_ERROR_TEXT("Current GPS fix is invalid. Cannot calculate bearing."));
    }

    double target_lon, target_lat;
    unpack_coord(args[ARG_coord].u_obj, &target_lon, &target_lat);

    double bearing = calculate_bearing(self->data.lat, self->data.lon, target_lat, target_lon);
    return mp_obj_new_int((int)round(bearing));
}
static MP_DEFINE_CONST_FUN_OBJ_KW(ublox_calc_bearing_obj, 1, ublox_calc_bearing);


static mp_obj_t ublox_get_fix(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_max_count, ARG_start_msg, ARG_end_msg };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_max_count, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 5 } },
        { MP_QSTR_start_msg, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none } },
        { MP_QSTR_end_msg,   MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none } },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    ublox_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    int max_count = (args[ARG_max_count].u_int < 1) ? 1 : args[ARG_max_count].u_int;
    const char* start_msg_type = get_msg_type(args[ARG_start_msg].u_obj, "RMC");
    const char* end_msg_type =   get_msg_type(args[ARG_end_msg].u_obj, "ZDA");
    char current_msg_type[4] = {0};
    if (self->debug == true) {
        mp_printf(&mp_plat_print, "[ugps] Get Fix\n");
        mp_printf(&mp_plat_print, "[ugps] - max epoch:                '%u'\n", (unsigned int)max_count);
        mp_printf(&mp_plat_print, "[ugps] - epoch start message type: '%s'\n", start_msg_type);
        mp_printf(&mp_plat_print, "[ugps] - epoch end message type:   '%s'\n", end_msg_type);
    }
    int epoch_count = 0;
    int data_is_valid = self->data.is_valid;
    char nmea_msg[128];
    gps_data_reset(&self->data);
    mp_obj_t readline_method = mp_load_attr(self->uart, MP_QSTR_readline);
    if (readline_method == MP_OBJ_NULL) {
        mp_raise_TypeError(MP_ERROR_TEXT("UART has no readline method"));
    }
    while (epoch_count < max_count) {
        data_is_valid = self->data.is_valid;

        if (read_nmea_line(readline_method, nmea_msg, sizeof(nmea_msg)) != 0) {
            if (self->debug == true) {
                mp_printf(&mp_plat_print, "[ugps] .");
            }
            gps_data_reset(&self->data);
            mp_hal_delay_ms(10);
            continue;
        }
        current_msg_type[0] = nmea_msg[3];
        current_msg_type[1] = nmea_msg[4];
        current_msg_type[2] = nmea_msg[5];

        if (self->debug == true) {
            mp_printf(&mp_plat_print, "[ugps] - [%d/%d] [%d] | msg:type=%s | msg=%s\n", epoch_count, max_count, data_is_valid, current_msg_type, nmea_msg);
        }
        if (strcmp(current_msg_type, end_msg_type) == 0) {
            //
            if (data_is_valid && epoch_count > 0) {
                return create_data_dict(&self->data);
            }
        } else if (strcmp(current_msg_type, start_msg_type) == 0) {
            if (data_is_valid && epoch_count > 0) {
                return create_data_dict(&self->data);
            }
            gps_data_reset(&self->data);
            epoch_count++;
        }
        parse_nmea(mp_obj_new_str(nmea_msg, strlen(nmea_msg)), &self->data);
    }
    return create_data_dict(&self->data);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(ublox_get_fix_obj, 1, ublox_get_fix);

static const mp_rom_map_elem_t ublox_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_get_fix),        MP_ROM_PTR(&ublox_get_fix_obj) },
    { MP_ROM_QSTR(MP_QSTR_calc_distance),  MP_ROM_PTR(&ublox_calc_distance_obj) },
    { MP_ROM_QSTR(MP_QSTR_calc_bearing),   MP_ROM_PTR(&ublox_calc_bearing_obj) },
};
static MP_DEFINE_CONST_DICT(ublox_locals_dict, ublox_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ublox,
    MP_QSTR_ublox,
    MP_TYPE_FLAG_NONE,
    make_new, ublox_make_new,
    locals_dict, &ublox_locals_dict
);