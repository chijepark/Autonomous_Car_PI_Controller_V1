#include <unistd.h>
#include "ros/ros.h"
#include "control_esc_v3/MsgControl.h"
#include <stdio.h>
#include <cstdio>
#include <cstdlib>
#include <pigpio.h>
#include <wiringPi.h>
#include <pthread.h>


int receive_degree=-90;

void m_stop();
void m_forward(int speed);
void m_backward();
int PID_function();
void *t_function();
void *u_function();

void m_backward()
{

gpioPWM(17, 240); // 6% 듀티비 인가 (240/4000)
time_sleep(0.05); //필수 시간딜레이
gpioPWM(17, 230); // 후진 신호 인가
time_sleep(0.05); // 필수 타임 딜레이
gpioPWM(17,0); // 신호 끄기
time_sleep(0.05);

///이전 코드에서 0V 인가 뒤 똑같은 후진동작 실행
gpioPWM(17,240);
time_sleep(0.05);
gpioPWM(17,230);
time_sleep(0.05);

return;

}


void msgCallback(const control_esc_v3::MsgControl::ConstPtr& msg);


void *t_function(void *args)
{
	unsigned int rateTimer;
	unsigned int now;
	unsigned int controlPeriod = 1000;//1ms

	while (1)
	{

	if(receive_degree==-90)
	  {
		while(1)
		   {
	         	if(receive_degree!=-90)
	               	{
				m_stop();
				time_sleep(2);
				m_forward(260);
				time_sleep(0.4);
			 	m_forward(251);
				time_sleep(0.1);
			break;
			}
               	   }
    	  }

		rateTimer = now = micros();
		while ((now - rateTimer) < controlPeriod)//loop untill 1ms
			now = micros();

		gpioPWM(18, PID_function());//PWM injection
   
   if(receive_degree==9999)//9999 = Stop Signal
   {
   m_backward();
   time_sleep(0.8);
   while(1)
     {
     m_stop();
    	gpioPWM(18,270);
     }
   }
   
	}

}


void msgCallback(const control_esc_v3::MsgControl::ConstPtr& msg)
{
	ROS_INFO("recieve msg = %d", msg->data);
	receive_degree = msg->data;

}

void m_stop()
{
	gpioPWM(17, 240); //natural
	return;
}
void m_forward(int speed)
{
	gpioPWM(17, 240);
	time_sleep(0.05);
	gpioPWM(17, speed);
	time_sleep(0.05);
	return;
}

int PID_function()
{

	int desired_angle = 90;
	int current_angle = receive_degree;//first degree from TX2
	int current_angle_modified = 180 - current_angle;// second degree fited with servo (180-current_angle)

  int error = desired_angle - current_angle_modified; //error angle
  // int pre_error = 0;
  int P_control;
  //int I_control, D_control;
  int PID_control; // contorl angle value
	int AbsU;
	int PWMvalue;
	//	int dT = 0.001; //PID calculation interval = 1ms
	
  	int Kp = 1;
	//	int Ki = 1;
	//	int Kd = 1;

  P_control = Kp*error;
  //I_control = I_control + Ki*error*dT;
	//D_control = Kd*(error - pre_error) / dT;
	//pre_error = error;
	PID_control = P_control;
 
	AbsU = 90 - PID_control; // Absolute angle for control angle value
	
  if(AbsU<40||AbsU>140) // if servo control angle is over the critical point
  {
    if(AbsU<60)
    {
    AbsU=40;
    }
    else
    {
    AbsU=140;
    }
  }
  PWMvalue = 180 + AbsU; // PWMvalue for moving servo motor
  
 
	return PWMvalue;

}



int main(int argc, char** argv)
{



	/////////////////////////
	ros::init(argc, argv, "esc_out_v3");
	ros::NodeHandle nh;
	ros::Subscriber ros_tutorial_sub = nh.subscribe("ros_msg", 100, msgCallback);

	////////////////////////////////


	if (wiringPiSetup() == -1) return 1;
	if (gpioInitialise()<0) return -1;

	//SERVO setting
	gpioSetMode(18, PI_OUTPUT);
	gpioSetPWMfrequency(18, 50);
	gpioSetPWMrange(18, 3600);
	
	//DC setting
	gpioSetMode(17, PI_OUTPUT); //use GPIO 17pin
	gpioSetPWMfrequency(17, 40); // set frequency 40Hz
	gpioSetPWMrange(17, 4000); // speed range transfer 255->4000
 
 


	/**************thread setting**************************/
	pthread_t p_thread;

	int pid;
  pid = pthread_create(&p_thread, NULL, t_function, NULL);
	if(pid<0)
  {
	perror("thread create error");
	exit(1);
}



	
	/******************************************************/



	ros::spin();
   
	return 0;
}
