#include "data.h"
#include "OLEDUI.h"
#include "SDFatfs.h"
#include "usmart.h"
#include "interrupt.h"
#include "PIDController.h"
#include "init.h"
#include "PIDController.h"

//upto 1000lines，10ms,30000rmp
//#define DECODER_LINES   256
//#define DECODER_COUNT 10000
//#define MAX_PWM 1000
//#define DECODER1_RAW_GRY 27.27272727273f
//#define DECODER2_RAW_GRY 0.3515625f
//#define CheckData(data_in,data_th)  ((data_in) < (data_th/2.0f))?\
//                                    (data_in):((data_in)-(data_th))
#define OCS_PIN PEin(12) 
#define SD_PIN (!PEin(11))                                  

#define STEP_DIV        8
#define MM_1204 4
#define MM_PER_PULSE    0.005*MM_1204/STEP_DIV
#define MIN_STEP_FREQUENCY      800
#define MAX_STEP_FREQUENCY      9600  
#define MAX_STEP_MM     25

DATA_IN_STRUCT indata;
DATA_OUT_STRUCT outdata;
AXIS_STRUCT camera_axis;
AXIS_STRUCT board_axis;

//static void GetGY_25(MPU6050_EULER_STRUCT*);

void DataInput()
{
  //获得两个编码器参数
  //  indata.decoder1.raw = CheckData(TIM4->CNT,DECODER_COUNT);
  //  indata.decoder2.raw = CheckData(TIM8->CNT,DECODER_COUNT);
  //  indata.decoder1.acc_roll += indata.decoder1.raw;
  //  indata.decoder2.acc_roll += indata.decoder2.raw;
  //  
  //  indata.decoder1.ang_v = indata.decoder1.raw * DECODER1_RAW_GRY;
  //  indata.decoder2.ang_v = indata.decoder2.raw * DECODER2_RAW_GRY;
  //  
  //  TIM4->CNT = 0;
  
  //获得摄像头数据
  HAL_UART_Receive_IT(&huart1, (uint8_t*)uart1_rx_buff, 4);
  indata.speed = sqrt(pow((indata.last_position.y - indata.ball_position.y),2)+pow((indata.last_position.x - indata.ball_position.x),2));
//  if(indata.speed > 2)
//    indata.ball_position = indata.last_position;
  
  indata.last_position = indata.ball_position;
}


KALMAN_FILTER kfx, kfy;
void KalmanFilter(KALMAN_FILTER* kf)
{
        kf->x_mid = kf->x_last; //x_last=x(k-1|k-1),x_mid=x(k|k-1)
        kf->p_mid = kf->p_last + kf->Q; //p_mid=p(k|k-1),p_last=p(k-1|k-1),Q=噪声
        kf->kg = kf->p_mid / (kf->p_mid + kf->R); //为kalman filter，R为噪声
        kf->x_now = kf->x_mid + kf->kg*(kf->resrc_data - kf->x_mid);//估计出的最优值
        kf->p_now = (1 - kf->kg)*kf->p_mid;//最优值对应的协方差
        kf->p_last = kf->p_now;   //更新covariance值
        kf->x_last = kf->x_now;   //更新系统状态值
}

void DataProcess()
{  
  if(sys.status == BLOCKED)
  {
    //    outdata.step_motor1.set_lenth_mm = outdata.step_motor2.set_lenth_mm = 0;
  }
  
  else if(sys.status == RUNNING)
  { 
    setpara.pid_x.bound = setpara.pid_y.bound = MAX_STEP_MM;
    setpara.pid_x.death = setpara.pid_y.death = 0;
    
    setpara.pid_y = setpara.pid_x;
    
    GetPIDPara(&pid_x,&setpara.pid_x);
    GetPIDPara(&pid_y,&setpara.pid_y);
    
    switch(setpara.task_num)
    {
    case 1:
      //      if(outdata.step_motor1.speed > 0)
      //        outdata.step_motor1.direction = 0;
      //      else
      //        outdata.step_motor1.direction = 1;
      //      
      //      if(outdata.step_motor2.speed > 0)
      //        outdata.step_motor2.direction = 0;
      //      else
      //        outdata.step_motor2.direction = 1;
      //      
      //      outdata.step_motor1.frequency = abs(outdata.step_motor1.speed);
      //      outdata.step_motor2.frequency = abs(outdata.step_motor2.speed);
      
      outdata.step_motor1.set_lenth_mm = setpara.test_1/10.0;
      outdata.step_motor2.set_lenth_mm = setpara.test_2/10.0;
      
      break;
    case 2:      
      if((sys.T_RUN%(setpara.test_1+setpara.test_2) < setpara.test_1) || indata.speed > 0.2)
      {
        kfx.Q = 0.23;
        kfx.R = 50;
        kfx.resrc_data = indata.ball_position.x;
        kfy.Q = 0.23;
        kfy.R = 50;
        kfy.resrc_data = indata.ball_position.y;
        KalmanFilter(&kfx);
        KalmanFilter(&kfy);
        
        
        kfy.resrc_data = indata.ball_position.x;
        pid_x.current_point = kfx.x_now;
        pid_x.set_point = setpara.test_3;
        IncPIDCalc(&pid_x);
        outdata.step_motor2.set_lenth_mm = -pid_x.sum_con;
        
        pid_y.current_point = kfy.x_now;
        pid_y.set_point = setpara.test_4;
        IncPIDCalc(&pid_y);
        outdata.step_motor1.set_lenth_mm = -pid_y.sum_con;
      }
      else
      {
        outdata.step_motor1.set_lenth_mm = 0;
        outdata.step_motor2.set_lenth_mm = 0;
      }
      
      
      
      //        outdata.step_motor1.set_lenth_mm += sin(setpara.test_1*sys.T_RUN/1000);
      //        outdata.step_motor2.set_lenth_mm += sin(setpara.test_1*sys.T_RUN/1000);
      
      outdata.step_motor1.set_lenth_mm = limit(outdata.step_motor1.set_lenth_mm, -MAX_STEP_MM, MAX_STEP_MM);
      outdata.step_motor2.set_lenth_mm = limit(outdata.step_motor2.set_lenth_mm, -MAX_STEP_MM, MAX_STEP_MM);
      
      break;
      
    default:
      
      break;
    }
    
    //计算当前距离
    if(outdata.step_motor1.direction == 0)
      outdata.step_motor1.lenth_mm += (outdata.step_motor1.frequency*T_PERIOD_MS*MM_PER_PULSE/1000);
    else
      outdata.step_motor1.lenth_mm -= (outdata.step_motor1.frequency*T_PERIOD_MS*MM_PER_PULSE/1000);
    if(outdata.step_motor2.direction == 0)
      outdata.step_motor2.lenth_mm += (outdata.step_motor2.frequency*T_PERIOD_MS*MM_PER_PULSE/1000);
    else
      outdata.step_motor2.lenth_mm -= (outdata.step_motor2.frequency*T_PERIOD_MS*MM_PER_PULSE/1000);
    
    //控制到设定距离
    outdata.step_motor1.diff_lenth_mm = outdata.step_motor1.set_lenth_mm - outdata.step_motor1.lenth_mm;
    outdata.step_motor2.diff_lenth_mm = outdata.step_motor2.set_lenth_mm - outdata.step_motor2.lenth_mm;
    
    if(fabs(outdata.step_motor1.diff_lenth_mm) < 0.2)
    {
      outdata.step_motor1.frequency = 0;
    }
    else
    {
      if(outdata.step_motor1.diff_lenth_mm > 0.2)
      {
        outdata.step_motor1.direction = 0;
      }
      else if(outdata.step_motor1.diff_lenth_mm < -0.2)
      {
        outdata.step_motor1.direction = 1;
      }
      
      switch(outdata.step_motor1.frequency)
      {
      case 0:outdata.step_motor1.frequency = MIN_STEP_FREQUENCY;break;
      case MIN_STEP_FREQUENCY: outdata.step_motor1.frequency = MAX_STEP_FREQUENCY/5;break;
      case MAX_STEP_FREQUENCY/5: outdata.step_motor1.frequency = MAX_STEP_FREQUENCY/4;break;
      case MAX_STEP_FREQUENCY/4: outdata.step_motor1.frequency = MAX_STEP_FREQUENCY/2;break;
      case MAX_STEP_FREQUENCY/2: outdata.step_motor1.frequency = MAX_STEP_FREQUENCY;break;
      default: break;
      }
      
      if((outdata.step_motor1.pre_direction + outdata.step_motor1.direction) == 1)
        outdata.step_motor1.frequency = 0;
    }
    
    if(fabs(outdata.step_motor2.diff_lenth_mm) < 0.2)
    {
      outdata.step_motor2.frequency = 0;
    }
    else
    {
      if(outdata.step_motor2.diff_lenth_mm > 0.2)
      {
        outdata.step_motor2.direction = 0;
      }
      else if(outdata.step_motor2.diff_lenth_mm < -0.2)
      {
        outdata.step_motor2.direction = 1;
      }
      switch(outdata.step_motor2.frequency)
      {
      case 0:outdata.step_motor2.frequency = MIN_STEP_FREQUENCY;break;
      case MIN_STEP_FREQUENCY: outdata.step_motor2.frequency = MAX_STEP_FREQUENCY/5;break;
      case MAX_STEP_FREQUENCY/5: outdata.step_motor2.frequency = MAX_STEP_FREQUENCY/4;break;
      case MAX_STEP_FREQUENCY/4: outdata.step_motor2.frequency = MAX_STEP_FREQUENCY/2;break;
      case MAX_STEP_FREQUENCY/2: outdata.step_motor2.frequency = MAX_STEP_FREQUENCY;break;
      default: break;
      }
      if((outdata.step_motor2.pre_direction + outdata.step_motor2.direction) == 1)
        outdata.step_motor2.frequency = 0;
    }
    
    outdata.step_motor1.pre_direction = outdata.step_motor1.direction;
    outdata.step_motor2.pre_direction = outdata.step_motor2.direction;
  }
  
}

void DataOutput()
{    
  if(sys.status == RUNNING)
  {
    PAout(2) = outdata.step_motor1.enable = 1;
    PBout(0) = outdata.step_motor1.enable = 1;
  }
  else
  {
    PAout(2) = outdata.step_motor1.enable = 0;
    PBout(0) = outdata.step_motor1.enable = 0;
  }
  
  PAout(1) = outdata.step_motor1.direction;
  PAout(7) = outdata.step_motor2.direction;
  
  //开启pwm中断
  HAL_NVIC_EnableIRQ(TIM2_IRQn);
  HAL_NVIC_EnableIRQ(TIM3_IRQn);
}

void DataNoPut()
{
  PWMStop();
}

void DataSave()
{
  if(!(sys.osc_suspend || OCS_PIN))
    SendOscilloscope();
  
#if SD_ENABLE
  if(sys.sd_write && SD_PIN)
    DataWriteFatfs();
#endif
}

/*
        Q:过程噪声，Q增大，动态响应变快，收敛稳定性变坏
        R:测量噪声，R增大，动态响应变慢，收敛稳定性变好
*/



//
//void GetGY_25(MPU6050_EULER_STRUCT *gy25)
//{
//  int a;
//  a= 0x51a5;//通信规则
//  HAL_UART_Transmit(&huart1,(uint8_t*)&(a),2,0xFFFF);
//  HAL_UART_Receive(&huart1, (uint8_t*)uart1_rx_buff, 8, 0x01);
//  
//  gy25->pitch = ((int16_t)( (uart1_rx_buff[3] << 8) | uart1_rx_buff[4] )) / 100.0f; 
//  gy25->roll = ((int16_t)( (uart1_rx_buff[5] << 8) | uart1_rx_buff[6] )) / 100.0f; 
//  gy25->yaw = ((int16_t)( (uart1_rx_buff[1] << 8) | uart1_rx_buff[2] )) / 100.0f; 
//}

POINT_STRUCT Camera2BoardAxisChange(POINT_STRUCT camera)
{
  POINT_STRUCT board;
  board.x = camera.x + camera_axis.transfer.x;
  board.y = camera.y + camera_axis.transfer.y;
  return board;
}


