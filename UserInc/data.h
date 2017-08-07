#ifndef _DATA_H
#define _DATA_H
#include "userinc.h"
#include "mpu6050_process.h"
#include "PIDController.h"

typedef struct 
{
  int32_t raw;
  int32_t acc_roll;
  uint16_t lines;
  float ang_v;
  float line_v;
}DECODER_STRUCT;

typedef struct 
{
  MPU6050_DATA_STRUCT mpu6050;
  MPU6050_EULER_STRUCT gy25_euler;
  MPU6050_EULER_STRUCT gy25_euler_last;
  MPU6050_EULER_STRUCT global_euler;
  MPU6050_EULER_STRUCT global_euler_last;
  DECODER_STRUCT decoder1;
  DECODER_STRUCT decoder2;
  int16_t adc10;
}DATA_IN_STRUCT;

typedef struct 
{
  uint32_t channel1;
  uint32_t channel2;
  uint32_t channel3;
  uint32_t channel4;
}CHANNEL_STRUCT;

typedef struct 
{
  CHANNEL_STRUCT tim2;
  CHANNEL_STRUCT tim3;
  MPU6050_EULER_STRUCT euler;
  float speed;
  int motor_hz;
  int pwm_x;
  int pwm_y;
}DATA_OUT_STRUCT;


extern DATA_IN_STRUCT indata;
extern DATA_OUT_STRUCT outdata;
extern MPU6050_PHYSICAL_STRUCT mpu6050_offset;
extern PID axis_x;
extern PID axis_y;
extern PID axis_x_error;
extern PID axis_y_error;

void DataInput();
void DataSave();
void DataOutput();
void DataNoPut();
void DataProcess();


#endif

