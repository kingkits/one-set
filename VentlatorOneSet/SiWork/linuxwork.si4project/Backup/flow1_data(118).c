/* *******************************************************
 * 这组数据是呼气流量传感器的临时标校数据，目前用于检测
 * 主气道流速
 */
typedef unsigned int uint32_t;
const uint32_t expire_sensor_flow_data[] =
{
    0x7cd,
    0xf31

};

// 增加4%的斜率 20180817
const uint32_t expire_flow_val_data[] =
{
    0,
    132080
};

const uint32_t expire_flow_high_idx_data[] =
{
    2048,
    3000
};


const uint32_t expire_flow_high_data[] =
{
    0,
    300000
};



