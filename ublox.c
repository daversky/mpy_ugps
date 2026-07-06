// ublox.c
#include "py/runtime.h"
#include "py/obj.h"
#include "ublox.h"
#include "tools.h"
#include <math.h>

// Конструктор: gps = ugps.ublox(uart)
//static mp_obj_t ublox_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
//    mp_arg_check_num(n_args, n_kw, 1, 1, false); // Ждем ровно 1 аргумент — UART
//
//    mp_obj_ublox_t *self = m_new_obj(mp_obj_ublox_t);
//    self->base.type = &mp_type_ublox;
//    self->uart = args[0]; // Сохраняем ссылку на объект UART
//
//    gps_data_reset(&self->data);
//    return MP_OBJ_FROM_PTR(self);
//}
// ublox.c
static mp_obj_t ublox_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 1, false); // Ждем ровно 1 аргумент — UART

    mp_obj_t uart_obj = args[0];

    // 1. Проверяем, что нам вообще передали объект
    if (uart_obj == mp_const_none || uart_obj == MP_OBJ_NULL) {
        mp_raise_TypeError(MP_ERROR_TEXT("uart argument required"));
    }

    // 2. Проверяем наличие метода readline у переданного объекта
    mp_obj_t readline_method = mp_load_attr(uart_obj, MP_QSTR_readline);
    if (readline_method == MP_OBJ_NULL) {
        mp_raise_TypeError(MP_ERROR_TEXT("UART has no readline method"));
    }

    // 3. Если проверки прошли — выделяем память под объект класса ublox
    mp_obj_ublox_t *self = m_new_obj(mp_obj_ublox_t);
    self->base.type = &mp_type_ublox;
    self->uart = uart_obj;
    self->readline = readline_method; // Кэшируем метод для мгновенного вызова

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

    mp_obj_ublox_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    if (!self->data.is_valid || isnan(self->data.lat) || isnan(self->data.lon)) {
        // Исправлено: добавлен макрос MP_ERROR_TEXT
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

    mp_obj_ublox_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    if (!self->data.is_valid || isnan(self->data.lat) || isnan(self->data.lon)) {
        // Исправлено: добавлен макрос MP_ERROR_TEXT
        mp_raise_ValueError(MP_ERROR_TEXT("Current GPS fix is invalid. Cannot calculate bearing."));
    }

    double target_lon, target_lat;
    unpack_coord(args[ARG_coord].u_obj, &target_lon, &target_lat);

    double bearing = calculate_bearing(self->data.lat, self->data.lon, target_lat, target_lon);
    return mp_obj_new_int((int)round(bearing));
}
static MP_DEFINE_CONST_FUN_OBJ_KW(ublox_calc_bearing_obj, 1, ublox_calc_bearing);

// get_fix(epochs=120)
// ublox_get_fix(max_count=120, ...) внутри ublox.c
static mp_obj_t ublox_get_fix(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_max_count, ARG_start_msg, ARG_end_msg, ARG_debug };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_max_count, MP_ARG_INT, {.u_int = 120} }, // Дефолт 120 эпох
        { MP_QSTR_start_msg, MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_end_msg,   MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_debug,     MP_ARG_INT, {.u_int = 0} },
    };

    // Парсим аргументы. Внимание: n_args - 1 и pos_args + 1, так как первый аргумент — это self!
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // Вытаскиваем сам объект класса ublox
    mp_obj_ublox_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    // ДОСТАЕМ UART НАПРЯМУЮ ИЗ SELF! Передавать его из Python больше не нужно
    mp_obj_t uart = self->uart;

    int max_count = args[ARG_max_count].u_int;
    int debug     = args[ARG_debug].u_int;

    const char* start_msg = get_msg_type(args[ARG_start_msg].u_obj, "RMC");
    const char* end_msg   = get_msg_type(args[ARG_end_msg].u_obj, "ZDA");

    // Загружаем метод readline из сохраненного внутри self UART
    mp_obj_t readline_method = mp_load_attr(uart, MP_QSTR_readline);

    // ... дальнейший цикл while (epoch_count <= max_count) полностью идентичен,
    // но вместо глобальной current_gps_data мы пишем в данные конкретного объекта: &self->data ...

    // И возвращаем словарь, собранный из внутренних данных объекта
    return create_data_dict(&self->data);
}
// 1 означает, что как минимум self обязателен при вызове
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