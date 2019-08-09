// File: ventLIB.c

// if tm1 > tm2
unsigned char system_direct_compare_tm1_bg_tm2(unsigned int tm1, unsigned int tm2)
{
    unsigned int time1, time2;
    if((tm2 > 0xffff159f && tm1 < 60000) || (tm1 > 0xffff159f && tm2 < 60000))
    {
        time1 = tm1 + 60000;
        time2 = tm2 + 60000;
        return time1 > time2;
    }
    return tm1 > tm2;
}

// if tm1 >= tm2
unsigned char system_direct_compare_tm1_be_tm2(unsigned int tm1, unsigned int tm2)
{
    unsigned int time1, time2;
    if((tm2 > 0xffff159f && tm1 < 60000) || (tm1 > 0xffff159f && tm2 < 60000))
    {
        time1 = tm1 + 60000;
        time2 = tm2 + 60000;
        return time1 >= time2;
    }
    return tm1 >= tm2;
}


