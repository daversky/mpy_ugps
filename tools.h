// tools.h
#ifndef NMEA_TOOLS_H
#define NMEA_TOOLS_H
//#include <stdbool.h>
#include <stdint.h>
//#include <stddef.h>
#include "py/obj.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

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
    char utc_time[9];  // "hh:mm:ss" + \0
    char utc_date[11]; // "DD.MM.YYYY" + \0
} gnss_data_t;

const char* get_msg_type(mp_obj_t obj, const char* default_type);
void gps_data_reset(gnss_data_t* gps_data);
mp_obj_t create_data_dict(gnss_data_t* gps_data);
void unpack_coord(mp_obj_t coord_obj, double* lon, double* lat);
void parse_nmea(mp_obj_t string_obj, gnss_data_t* gps_data);

int checksum_valid(const char* nmea_msg);
int parse_fields(const char* nmea_msg, char fields[][16], int max_fields);
double parse_coordinate(const char* coord, char direction);
double calculate_distance_haversine(double lat1, double lon1, double lat2, double lon2);
double calculate_bearing(double lat1, double lon1, double lat2, double lon2);

// Внутренние функции парсинга конкретных сообщений
void parse_rmc(const char* nmea_msg, gnss_data_t* gps_data);
void parse_gga(const char* nmea_msg, gnss_data_t* gps_data);
void parse_gsa(const char* nmea_msg, gnss_data_t* gps_data);
void parse_gsv(const char* nmea_msg, gnss_data_t* gps_data);
void parse_vtg(const char* nmea_msg, gnss_data_t* gps_data);

#endif // NMEA_TOOLS_H