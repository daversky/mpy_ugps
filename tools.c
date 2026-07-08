// tools.c
#include "tools.h"
#include "py/runtime.h"
#include "py/obj.h"
//#include <math.h>
//#include <string.h>
//#include <stdlib.h>

#define EARTH_RADIUS_M 6371000.0
#define DEG_TO_RAD (M_PI / 180.0)

const char* get_msg_type(mp_obj_t obj, const char* default_val) {
    if (obj == mp_const_none || obj == MP_OBJ_NULL) {
        return default_val;
    }
    return mp_obj_str_get_str(obj);
}

static mp_obj_t float_or_none(double value) {
    if (isnan(value)) {
        return mp_const_none;
    }
    return mp_obj_new_float(value);
}

mp_obj_t create_data_dict(gnss_data_t* gps_data) {
    mp_obj_t dict = mp_obj_new_dict(15);
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_valid),        mp_obj_new_bool(gps_data->is_valid));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_sat_used),     mp_obj_new_int(gps_data->sat_used));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_sat_visible),  mp_obj_new_int(gps_data->sat_visible));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_lat),          float_or_none(gps_data->lat));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_lon),          float_or_none(gps_data->lon));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_alt),          float_or_none(gps_data->alt));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_speed_kmh),    float_or_none(gps_data->speed_kmh));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_speed_knots),  float_or_none(gps_data->speed_knots));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_course),       float_or_none(gps_data->course));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_hdop),         float_or_none(gps_data->hdop));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_vdop),         float_or_none(gps_data->vdop));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_pdop),         float_or_none(gps_data->pdop));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_utc_time),     mp_obj_new_str(gps_data->utc_time, strlen(gps_data->utc_time)));
    mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR(MP_QSTR_utc_date),     mp_obj_new_str(gps_data->utc_date, strlen(gps_data->utc_date)));
    return dict;
}

void unpack_coord(mp_obj_t coord_obj, double* lon, double* lat) {
    mp_obj_t *items;
    size_t len;
    mp_obj_get_array(coord_obj, &len, &items);
    *lon = mp_obj_get_float(items[0]);
    *lat = mp_obj_get_float(items[1]);
}

void gps_data_reset(gnss_data_t* gps_data) {
    gps_data->is_valid = 0;
    gps_data->lat = NAN;
    gps_data->lon = NAN;
    gps_data->alt = NAN;
    gps_data->speed_knots = NAN;
    gps_data->speed_kmh = NAN;
    gps_data->course = NAN;
    gps_data->sat_used = 0;
    gps_data->sat_visible = 0;
    gps_data->fix_type = 0;
    gps_data->hdop = NAN;
    gps_data->vdop = NAN;
    gps_data->pdop = NAN;
    gps_data->utc_time[0] = '\0';
    gps_data->utc_date[0] = '\0';
}

int parse_fields(const char* msg, char fields[][16], int max_fields) {
    int field_count = 0;
    const char* start = msg;
    while (*msg && field_count < max_fields) {
        if (*msg == ',') {
            size_t len = msg - start;
            if (len < sizeof(fields[0]) - 1) {
                memcpy(fields[field_count], start, len);
                fields[field_count][len] = '\0';
            } else {
                fields[field_count][0] = '\0';
            }
            field_count++;
            start = msg + 1;
        }
        msg++;
    }
    if (*start && field_count < max_fields) {
        const char* end = strchr(start, '*');
        if (!end) end = msg;
        size_t len = end - start;
        if (len < sizeof(fields[0]) - 1) {
            memcpy(fields[field_count], start, len);
            fields[field_count][len] = '\0';
        } else {
            fields[field_count][0] = '\0';
        }
        field_count++;
    }
    return field_count;
}

double parse_coordinate(const char* coord, char direction) {
    if (strlen(coord) < 7) return NAN;
    const char* dot_pos = strchr(coord, '.');
    if (!dot_pos) return NAN;
    int dot_index = dot_pos - coord;
    int degree_digits;
    if (dot_index == 2) {
        degree_digits = 1;  // DDM.MMMMM
    } else if (dot_index == 3) {
        degree_digits = 2;  // DDM.MMMMM (редкий)
    } else if (dot_index == 4) {
        degree_digits = 2;  // DDMM.MMMMM (широта)
    } else if (dot_index == 5) {
        degree_digits = 3;  // DDDMM.MMMMM (долгота)
    } else if (dot_index == 6) {
        degree_digits = 4;  // DDDDMM.MMMMM (высокая точность)
    } else {
        return NAN;
    }
    char degrees_str[5] = {0};
    strncpy(degrees_str, coord, degree_digits);
    degrees_str[degree_digits] = '\0';
    char minutes_str[16] = {0};
    strncpy(minutes_str, coord + degree_digits, sizeof(minutes_str) - 1);
    double degrees = atof(degrees_str);
    double minutes = atof(minutes_str);
    double result = degrees + minutes / 60.0;
    if (direction == 'S' || direction == 'W') {
        result = -result;
    }
    return result;
}

void parse_nmea(mp_obj_t string_obj, gnss_data_t* gps_data) {
    const char* raw_string = mp_obj_str_get_str(string_obj);
    const char* nmea_string = strchr(raw_string, '$');
    int nmea_string_is_valid = 0;

    if (nmea_string != NULL) {
        if (strlen(nmea_string) >= 10 && checksum_valid(nmea_string)) {
            nmea_string_is_valid = 1;
        }
    }
    if (nmea_string_is_valid) {
        char msg_type[4] = {0};
        memcpy(msg_type, nmea_string + 3, 3);
        msg_type[3] = '\0';
        if (strcmp(msg_type, "RMC") == 0) {
            parse_rmc(nmea_string, gps_data);
        } else if (strcmp(msg_type, "GGA") == 0) {
            parse_gga(nmea_string, gps_data);
        } else if (strcmp(msg_type, "GSA") == 0) {
            parse_gsa(nmea_string, gps_data);
        } else if (strcmp(msg_type, "GSV") == 0) {
            parse_gsv(nmea_string, gps_data);
        } else if (strcmp(msg_type, "VTG") == 0) {
            parse_vtg(nmea_string, gps_data);
        }
    }
}

int checksum_valid(const char* msg) {
    if (msg[0] != '$') return 0;
    const char* checksum_start = strchr(msg, '*');
    if (!checksum_start) return 0;
    uint8_t calculated_checksum = 0;
    for (const char* p = msg + 1; p < checksum_start; p++) {
        calculated_checksum ^= *p;
    }
    uint8_t received_checksum = (uint8_t)strtol(checksum_start + 1, NULL, 16);
    return calculated_checksum == received_checksum;
}

void parse_rmc(const char* msg, gnss_data_t* gps_data) {
    char fields[20][16] = {0};
    int field_count = parse_fields(msg, fields, 20);
    if (field_count < 12) return;
    if (strlen(fields[1]) >= 6) {
        snprintf(gps_data->utc_time, sizeof(gps_data->utc_time), "%.2s:%.2s:%.2s", fields[1], fields[1] + 2, fields[1] + 4);
    }
    gps_data->is_valid = (fields[2][0] == 'A') ? 1 : 0;
    if (strlen(fields[3]) > 0 && strlen(fields[4]) > 0 && isnan(gps_data->lat)) {
        gps_data->lat = parse_coordinate(fields[3], fields[4][0]);
    }
    if (strlen(fields[5]) > 0 && strlen(fields[6]) > 0 && isnan(gps_data->lon)) {
        gps_data->lon = parse_coordinate(fields[5], fields[6][0]);
    }
    if (strlen(fields[7]) > 0) {
        double speed_knots = atof(fields[7]);
        gps_data->speed_knots = round(speed_knots * 10.0) / 10.0;
    }
    if (strlen(fields[8]) > 0) {
        gps_data->course = round(atof(fields[8]) * 10.0) / 10.0;
    }
    if (strlen(fields[9]) >= 6) {
        snprintf(gps_data->utc_date, sizeof(gps_data->utc_date), "%.2s.%.2s.20%.2s", fields[9], fields[9] + 2, fields[9] + 4);
    }
}

void parse_gga(const char* msg, gnss_data_t* gps_data) {
    char fields[20][16] = {0};
    int field_count = parse_fields(msg, fields, 20);
    if (field_count < 14) return;
    if (strlen(fields[2]) > 0 && strlen(fields[3]) > 0) {
        gps_data->lat = parse_coordinate(fields[2], fields[3][0]);
    }
    if (strlen(fields[4]) > 0 && strlen(fields[5]) > 0) {
        gps_data->lon = parse_coordinate(fields[4], fields[5][0]);
    }
    if (strlen(fields[6]) > 0) {
        gps_data->fix_type = atoi(fields[6]);
    }
    if (strlen(fields[8]) > 0) {
        gps_data->hdop = atof(fields[8]);
    }
    if (strlen(fields[9]) > 0) {
        gps_data->alt = round(atof(fields[9]) * 10.0) / 10.0;
    }
}

void parse_gsa(const char* msg, gnss_data_t* gps_data) {
    char fields[20][16] = {0};
    int field_count = parse_fields(msg, fields, 20);
    if (field_count < 18) return;
    int sats_in_this_msg = 0;
    for (int i = 3; i <= 14; i++) {
        if (strlen(fields[i]) > 0) {
            sats_in_this_msg++;
        }
    }
    gps_data->sat_used += sats_in_this_msg;
    if (strlen(fields[15]) > 0) {
        gps_data->pdop = round(atof(fields[15]) * 10.0) / 10.0;
    }
    if (strlen(fields[16]) > 0) {
        gps_data->hdop = round(atof(fields[16]) * 10.0) / 10.0;
    }
    if (strlen(fields[17]) > 0) {
        gps_data->vdop = round(atof(fields[17]) * 10.0) / 10.0;
    }
}

void parse_gsv(const char* msg, gnss_data_t* gps_data) {
    char fields[20][16] = {0};
    int field_count = parse_fields(msg, fields, 20);
    if (field_count < 4) return;
    if (strcmp(fields[2], "1") == 0) {
        if (strlen(fields[3]) > 0) {
            gps_data->sat_visible += atoi(fields[3]);
        }
    }
}

void parse_vtg(const char* msg, gnss_data_t* gps_data) {
    char fields[20][16] = {0};
    int field_count = parse_fields(msg, fields, 20);
    if (field_count < 8) return;
    if (strlen(fields[1]) > 0 && isnan(gps_data->course)) {
        gps_data->course = round(atof(fields[1]) * 10.0) / 10.0;
    }
    if (strlen(fields[7]) > 0 && (isnan(gps_data->speed_kmh) || gps_data->speed_kmh < 0.1)) {
        double speed_kmh = atof(fields[7]);
        gps_data->speed_kmh = round(speed_kmh * 10.0) / 10.0;
    }
}

double calculate_distance_haversine(double lat1, double lon1, double lat2, double lon2) {
    double lat1_rad = lat1 * DEG_TO_RAD;
    double lon1_rad = lon1 * DEG_TO_RAD;
    double lat2_rad = lat2 * DEG_TO_RAD;
    double lon2_rad = lon2 * DEG_TO_RAD;
    double dlat = lat2_rad - lat1_rad;
    double dlon = lon2_rad - lon1_rad;
    double a = sin(dlat/2) * sin(dlat/2) + cos(lat1_rad) * cos(lat2_rad) * sin(dlon/2) * sin(dlon/2);
    double c = 2 * atan2(sqrt(a), sqrt(1-a));
    return EARTH_RADIUS_M * c;
}

double calculate_bearing(double lat1, double lon1, double lat2, double lon2) {
    double r_lat1 = lat1 * DEG_TO_RAD;
    double r_lat2 = lat2 * DEG_TO_RAD;
    double dLon = (lon2 - lon1) * DEG_TO_RAD;
    double y = sin(dLon) * cos(r_lat2);
    double x = cos(r_lat1) * sin(r_lat2) - sin(r_lat1) * cos(r_lat2) * cos(dLon);
    double bearing = atan2(y, x) / DEG_TO_RAD;
    return bearing;
}