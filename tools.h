// tools.h
#ifndef NMEA_TOOLS_H
#define NMEA_TOOLS_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "py/obj.h"

// Единая Си-структура для хранения навигационных данных
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
    char utc_time[11];  // "hh:mm:ss" + \0 (с запасом под миллисекунды)
    char utc_date[11];  // "DD.MM.YYYY" + \0
} gps_data_t;

// Глобальное состояние данных, используемое модулем nmea для отладки
extern gps_data_t current_gps_data;

const char* get_msg_type(mp_obj_t obj, const char* default_type);
// Сброс полей структуры в NAN / 0 / пустые строки
void gps_data_reset(gps_data_t* gps_data);

// Конвертация структуры данных в Python-словарь (dict)
mp_obj_t create_data_dict(gps_data_t* gps_data);

// Распаковка Python-координат (кортеж/список) в Си-переменные double (без валидации типов)
void unpack_coord(mp_obj_t coord_obj, double* lon, double* lat);

// Главная функция парсинга строки, принимающая внешнюю целевую структуру
void parse_nmea_string(mp_obj_t string_obj, gps_data_t* gps_data);

// Вспомогательные Си-функции парсера и геодезии
int checksum_valid(const char* msg);
int parse_fields(const char* msg, char fields[][16], int max_fields);
double parse_coordinate(const char* coord, char direction);
double calculate_distance_haversine(double lat1, double lon1, double lat2, double lon2);
double calculate_bearing(double lat1, double lon1, double lat2, double lon2);

// Внутренние функции парсинга конкретных сообщений
void parse_rmc(const char* msg, gps_data_t* gps_data);
void parse_gga(const char* msg, gps_data_t* gps_data);
void parse_gsa(const char* msg, gps_data_t* gps_data);
void parse_gsv(const char* msg, gps_data_t* gps_data);
void parse_vtg(const char* msg, gps_data_t* gps_data);

#endif // NMEA_TOOLS_H