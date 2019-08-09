
/*
 * pid.c
 */

#include "pid.h"

/**
 *
 * @param pid
 * @param kp
 * @param ki
 * @param kd
 * @param intmax set to 0 to disable integral windup prevention
 */
void pid_init(PID_t           *pid,
              float         kp,
              float         ki,
              float         kd,
              float         intmax,
              unsigned char mode)
{
    pid->kp             = kp;
    pid->ki             = ki;
    pid->kd             = kd;
    pid->intmax         = intmax;   // 最大单次可调整偏差，每次调整不会超过这个限制
    pid->mode           = mode;
    pid->saturated      = 0;  // 饱和调整
    pid->sp             = 0;
    pid->error_previous = 0;
    pid->integral       = 0;
}


/* **************************************************************
 *
 * @param pid
 * (suggest value)
 * @param kp  0.1
 * @param ki  0
 * @param kd  0.1
 * @param intmax set to 0 to disable integral windup prevention
 * *************************************************************
 */
void pid_set_parameters(PID_t *pid,
                        float  kp,
                        float  ki,
                        float  kd,
                        float  intmax)
{
    pid->kp     = kp;
    pid->ki     = ki;
    pid->kd     = kd;
    pid->intmax = intmax;
}


/* ***********
 *
 * @param pid // pointer of PID - structure
 * @param val // current detected value
 * @param sp  // set point
 * @param dt  // adjust error for devided (误差修正系数 /dev)
 * @param val_dot // kd 修正系数，只在（pid->mode = PID_MODE_DERIVATIV_SET）时有效 （- kd*val_dot）
 * @return // 返回修正的PID值
 * ***********
 */
double pid_calculate(PID_t *pid,
                     double sp,
                     double val,
                     double val_dot,
                     double dt)
{
    double i, d;
    double error;

    pid->sp = sp;
    error = pid->sp - val;

    if (pid->saturated && (pid->integral * error > 0))
    {
        //Output is saturated and the integral would get bigger (positive or negative)
        i = pid->integral;

        //Reset saturation. If we are still saturated this will be set again at output limit check.
        pid->saturated = 0;
    }
    else
    {
        i = pid->integral + (error * dt);
    }
    // Anti-Windup. Needed if we don't use the saturation above.
    if (pid->intmax != 0.0)
    {
        if (i > pid->intmax)
        {
            pid->integral = pid->intmax;
        }
        else if (i < -pid->intmax)
        {

            pid->integral = -pid->intmax;
        }
        else
        {
            pid->integral = i;
        }
    }

    if (pid->mode == PID_MODE_DERIVATIV_CALC)
    {
        d = (error - pid->error_previous) / dt;
    }
    else if (pid->mode == PID_MODE_DERIVATIV_SET)
    {
        d = -val_dot;
    }
    else
    {
        d = 0;
    }

    pid->error_previous = error;

    return (error * pid->kp) + (i * pid->ki) + (d * pid->kd);
}

#if 0
void Compute(float input, float *output, float setPoint, PIDstructure *pidStruct)
{
    systime_t now = chTimeNow();	// system ticks
    systime_t timeChange = now - pidStruct->lastTime;	// system ticks

    if (timeChange >= S2ST(SAMPLE_TIME_S))
    {
        /*Compute all the working error variables*/
        float error = setPoint - input;
        pidStruct->ITerm += (pidStruct->ki * error);

        if (pidStruct->ITerm > pidStruct->outMax)
            pidStruct->ITerm = pidStruct->outMax;
        else if (pidStruct->ITerm < pidStruct->outMin)
            pidStruct->ITerm = pidStruct->outMin;
        float dInput = (input - pidStruct->lastInput);

        /*Compute PID Output*/
        *output = pidStruct->kp * error + pidStruct->ITerm - pidStruct->kd * dInput;
        if (*output > pidStruct->outMax)
            *output = pidStruct->outMax;
        else if (*output < pidStruct->outMin)
            *output = pidStruct->outMin;

        /*Remember some variables for next time*/
        pidStruct->lastInput = input;
        pidStruct->lastTime = now;
    }
}
#endif


