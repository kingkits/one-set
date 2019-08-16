# 2019/8/13: 

## 1 减小blower在压力偏差大时调整的幅度 
## 2 增加peep-valve-control-data 和 blower-speed 的计算函数（由新的校准数据导出） 
----
# 2019/8/16: 

## 1 recoding the method of PEEP-valve control data & speed control data 
----
'''
void get_cpap_speed_and_peep_valve_control_data(uint16_t press, int32_t flow)
{ 
  int32_t speed,peep;  
  double speed_add; 
  double speed_tmp; 
  double peep_tmp; 
  // 初步设想取5-10cmH2O和10L/min的数据，后面每增加一个厘米水PEEP增加30，speed增加800

  speed_add = (double)press * 53.57 + 3197;
  speed_tmp = flow * 0.8 + speed_add;
  speed = (uint16_t) speed_tmp;

  peep_tmp = 0.03 * flow + 1133 - 2 * press;
  peep = (uint16_t) peep_tmp;
  ...
}
```
