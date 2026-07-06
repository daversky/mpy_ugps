// nmea.h
#ifndef NMEA_H
#define NMEA_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "py/obj.h"

// Чистая Си-структура для хранения распарсенных данных
typedef struct {
    uint8_t is_valid;
    double lat;
    double lon;
    double alt;
    double speed_knots;
    double speed_kmh;
    double course;
    uint8_t sat_used;
    uint8_t sat_visible;
    uint8_t fix_type;
    double hdop;
    double vdop;
    double pdop;
    char utc_time[9];
    char utc_date[11];
} nmea_data_t;

// Экземпляр MicroPython-класса nmea для moduleugps.c
extern const mp_obj_type_t mp_type_nmea;

// Внутренние Си-функции парсинга и математики (пригодятся и для ublox)
int nmea_parse_msg(const char *msg, nmea_data_t *data);
double nmea_calc_distance(double lat1, double lon1, double lat2, double lon2);
double nmea_calc_bearing(double lat1, double lon1, double lat2, double lon2);

#endif // NMEA_H