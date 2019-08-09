// File: rap_control.c
// inline method defines
#define reset_rap_actions()  (pcv_control_dat.rap_action_flag = EM_RAP_ACTION_IDLE)
/**
 * [do_rap_actions description]
 * @method do_rap_actions
 * @param  type           [description]
 */
void do_rap_actions(int type)
{
    if(!is_rap_enable())
    {
        set_breath_rap_stop(); // force stop!!!
        reset_rap_actions();
        return;
    }


    switch(pcv_control_dat.rap_action_flag)
    {
    default:
    case EM_RAP_ACTION_IDLE:
        // start rap
        set_breath_rap_start();
        pcv_control_dat.rap_action_flag = EM_RAP_ACTION_ACTIVE;
        break;
    case EM_RAP_ACTION_ACTIVE:
        // check when it need stop
        // 如果在吸气相则检测吸气时间
        if(is_rap_time_over(type))
        {
            set_breath_rap_stop();
            pcv_control_dat.rap_action_flag = EM_RAP_ACTION_STOP;
        }

        break;
    case EM_RAP_ACTION_STOP:
        // just waiting
        break;
    }
}
