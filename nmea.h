// nmea.h
#ifndef NMEA_H
#define NMEA_H
#include "py/obj.h"
#include "tools.h"

typedef gnss_data_t nmea_data_t;
extern const mp_obj_type_t mp_type_nmea;

typedef struct {
    mp_obj_base_t base;
    nmea_data_t data;
} nmea_obj_t;

int nmea_parse_msg(const char *msg, nmea_data_t *data);
double nmea_calc_distance(double lat1, double lon1, double lat2, double lon2);
double nmea_calc_bearing(double lat1, double lon1, double lat2, double lon2);

#endif // NMEA_H