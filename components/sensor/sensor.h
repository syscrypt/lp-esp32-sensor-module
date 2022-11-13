#pragma once

void init_sensor(void* sensor);

void read_all_values(void* sensor, float* temperature, float* humidity, float* pressure);
