// File: Gas_compensation.c
float Atp_2_BTPS(float atp_val)
{
    return (float)atp_val * basic_control_parameters.btps_fact;
}


