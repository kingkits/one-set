说明：
Mode_control/breath_control.c 用于呼吸控制
Mode_control/breath_monitor.c 用于呼吸状态监控
Mode_control/Display_data_count.c 用于计算呼吸参数 用于显示/控制/报警
PCV
    EM_PATIENT_NOT_WORK,       // 待机 standby
    	monitor: 强制通气
    EM_PATIENT_PIPE_OUT,       // 管路处于脱落状态
    	monitor：检测管路恢复
    EM_PATIENT_T_INSPIRE_START,
    	control: 阀控制，状态控制，数据计算
    case EM_PATIENT_EXPIRE_START:	  // 呼气开始	
        control: 阀控制，状态控制
    EM_PATIENT_T_INSPIRE_PERIOD,
    	control: 压力控制，叩击
    EM_PATIENT_T_EXPIRE_DETECT,
    	control: 状态检测及转换
    	monitor: PEEP
    EM_PATIENT_T_EXPIRE_PERIOD,
        control: 状态检测及转换
                 叩击
    EM_PATIENT_T_INSPIRE_DETECT,