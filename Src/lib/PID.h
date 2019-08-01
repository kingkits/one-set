/*
 * pid.h
 */

#ifndef PID_H_
#define PID_H_

/* PID_MODE_DERIVATIV_CALC calculates discrete derivative from previous error
 * val_dot in pid_calculate() will be ignored */
#define PID_MODE_DERIVATIV_CALC	0
/* Use PID_MODE_DERIVATIV_SET if you have the derivative already (Gyros, Kalman) */
#define PID_MODE_DERIVATIV_SET	1

typedef struct
{
    double        kp;
    double        ki;
    double        kd;
    double        intmax;
    double        sp;
    double        integral;
    double        error_previous;
    unsigned char mode;
    unsigned char saturated;
} PID_t;

void pid_init(PID_t *pid, float kp, float ki, float kd, float intmax, unsigned char mode);//, unsigned char plot_i);
void pid_set_parameters(PID_t *pid, float kp, float ki, float kd, float intmax);
//void pid_set(PID_t *pid, float sp);
double pid_calculate(PID_t *pid, double sp, double val, double val_dot, double dt);

#endif /* PID_H_ */

