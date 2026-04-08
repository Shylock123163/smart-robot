#include "pid.h"

void PID_Init(PID_t *pid, float kp, float ki, float kd,
              float max_out, float max_i)
{
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->max_output = max_out;
    pid->max_integral = max_i;
}

float PID_Calc(PID_t *pid, float target, float measure)
{
    pid->target = target;
    pid->measure = measure;

    pid->err = target - measure;

    pid->integral += pid->err;
    if(pid->integral > pid->max_integral)
        pid->integral = pid->max_integral;
    if(pid->integral < -pid->max_integral)
        pid->integral = -pid->max_integral;

    float derivative = pid->err - pid->last_err;

    pid->output = pid->kp * pid->err +
                  pid->ki * pid->integral +
                  pid->kd * derivative;

    if(pid->output > pid->max_output)
        pid->output = pid->max_output;
    if(pid->output < -pid->max_output)
        pid->output = -pid->max_output;

    pid->last_err = pid->err;

    return pid->output;
}
