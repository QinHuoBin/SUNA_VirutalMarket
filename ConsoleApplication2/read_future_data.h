// 功能：读取在
//http://www.pythonpai.com/topic/4206/%E9%87%8F%E5%8C%96%E7%88%B1%E5%A5%BD%E8%80%85%E7%A6%8F%E5%88%A9%E8%B4%B4-%E9%87%8F%E5%8C%96%E4%BA%A4%E6%98%93%E4%BB%A3%E7%A0%81-%E5%B7%A5%E5%85%B7-2012-2020%E5%B9%B4%E6%9C%9F%E8%B4%A7%E5%85%A8%E5%93%81%E7%A7%8Dtick%E6%95%B0%E6%8D%AE%E5%85%B1%E4%BA%AB
// 下载的期货数据，其中每个csv的字段为：
/*
CSV数据文件字段顺序：
localtime (本机写入TICK的时间),
InstrumentID (合约名),
TradingDay (交易日),
ActionDay (业务日期),
UpdateTime （时间）,
UpdateMillisec（时间毫秒）,
LastPrice （最新价）,
Volume（成交量） ,
HighestPrice （最高价）,
LowestPrice（最低价） ,
OpenPrice（开盘价） ,
ClosePrice（收盘价）,
AveragePrice（均价）,
AskPrice1（申卖价一）,
AskVolume1（申卖量一）,
BidPrice1（申买价一）,
BidVolume1（申买量一）,
UpperLimitPrice（涨停板价）
LowerLimitPrice（跌停板价）
OpenInterest（持仓量）,
Turnover（成交金额）,
PreClosePrice (昨收盘),
PreOpenInterest (昨持仓),
PreSettlementPrice (上次结算价),

*/
// 2021年4月4日 15点54分

/*
编译方法：g++ read_future_data.cpp -o read_future_data -lpthread
记得加上-lpthread

c++的csv解析器：
    https://github.com/ben-strasser/fast-cpp-csv-parser

c++的date/time处理工具
    https://github.com/HowardHinnant/date
*/
#ifndef READ_FUTURE_DATA_H
#define READ_FUTURE_DATA_H

// 关闭csv的多线程读取
#define CSV_IO_NO_THREAD
#include "csv.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>
#include <cassert>

#include <filesystem>

namespace rfd
{
    typedef struct
    {
        time_t datetime;
        float LastPrice;
        float Volume;
    } MarketPrice;

    // 读取，并保存tick为二进制文件
    void save_binary_tick_file(const std::wstring &path, const std::string &code);
    // 直接读取二进制文件
    size_t read_binary_tick_file(std::vector<MarketPrice> &MarketPrice_vector, const std::wstring &path, const std::string &code);

    // 重新读取文本文件
    // 这个函数用于收集指定目录（递归）下所有的code.csv的数据
    size_t collect_all_ticks(std::vector<MarketPrice> &MarketPrice_vector, const std::wstring &path, const std::string &code);

    void fix_time_problem(std::vector<MarketPrice> &MarketPrice_vector);
    int resample(std::vector<MarketPrice> &MarketPrice_vector_original, std::vector<MarketPrice> &MarketPrice_vector_sample, size_t seconds);

    size_t getMarketPrice(std::vector<MarketPrice> &MarketPrice_vector, const std::wstring & csv_path);
    size_t get_satisfied_paths(std::vector<std::filesystem::path> &path_vector,
                               const std::wstring &path, const std::string &code);
    bool sort_by_date(std::filesystem::path p1, std::filesystem::path p2);
    time_t get_seconds_by_date( std::string date);

}

#endif
