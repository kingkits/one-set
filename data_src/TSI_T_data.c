typedef unsigned int uint32_t;
// index 为温度通道读取的电压（mV）(91个数据)
const uint32_t TSI_T_idx[] =
{
    2292,
    2280,
    2268,
    2255,
    2242,
    2228,
    2213,
    2198,
    2182,
    2166,
    2149,
    2131,
    2113,
    2094,
    2075,
    2055,
    2034,
    2013,
    1991,
    1969,
    1946,
    1922,
    1898,
    1873,
    1848,
    1823,
    1793,
    1770,
    1743,
    1716,
    1688,
    1660,
    1632,
    1603,
    1574,
    1545,
    1516,
    1486,
    1457,
    1427,
    1397,
    1368,
    1338,
    1309,
    1279,
    1250,
    1221,
    1192,
    1163,
    1135,
    1107,
    1079,
    1051,
    1024,
    997,
    971,
    945,
    919,
    894,
    869,
    845,
    821,
    798,
    775,
    753,
    731,
    709,
    688,
    668,
    648,
    629,
    610,
    591,
    573,
    556,
    539,
    522,
    506,
    491,
    475,
    461,
    446,
    433,
    419,
    406,
    393,
    381,
    369,
    358,
    346,
    336
};

// Tdata 为温度数据，其 转换公式位 真实温度 Treal = Tdata/1000.0 - 20.0;
const uint32_t TSI_T_data[] =
{
    0,
    1000,
    2000,
    3000,
    4000,
    5000,
    6000,
    7000,
    8000,
    9000,
    10000,
    11000,
    12000,
    13000,
    14000,
    15000,
    16000,
    17000,
    18000,
    19000,
    20000,
    21000,
    22000,
    23000,
    24000,
    25000,
    26000,
    27000,
    28000,
    29000,
    30000,
    31000,
    32000,
    33000,
    34000,
    35000,
    36000,
    37000,
    38000,
    39000,
    40000,
    41000,
    42000,
    43000,
    44000,
    45000,
    46000,
    47000,
    48000,
    49000,
    50000,
    51000,
    52000,
    53000,
    54000,
    55000,
    56000,
    57000,
    58000,
    59000,
    60000,
    61000,
    62000,
    63000,
    64000,
    65000,
    66000,
    67000,
    68000,
    69000,
    70000,
    71000,
    72000,
    73000,
    74000,
    75000,
    76000,
    77000,
    78000,
    79000,
    80000,
    81000,
    82000,
    83000,
    84000,
    85000,
    86000,
    87000,
    88000,
    89000,
    90000
};

