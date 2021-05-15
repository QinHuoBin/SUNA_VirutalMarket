/*
编译方法：g++ read_future_data.cpp -o read_future_data -lpthread
记得加上-lpthread
*/

#include "read_future_data.h"

namespace fs = std::filesystem;
using std::cout;
using std::endl;
using std::string;
using std::wstring;
using std::time_t;
using std::vector;
#include<chrono>

#define P(EX) cout << #EX << ": " << EX << endl;

//wstring=>string
std::string WString2String(const std::wstring& ws)
{
    std::string strLocale = setlocale(LC_ALL, "");
    const wchar_t* wchSrc = ws.c_str();
    size_t nDestSize = wcstombs(NULL, wchSrc, 0) + 1;
    char* chDest = new char[nDestSize];
    memset(chDest, 0, nDestSize);
    wcstombs(chDest, wchSrc, nDestSize);
    std::string strResult = chDest;
    delete[]chDest;
    setlocale(LC_ALL, strLocale.c_str());
    return strResult;
}
// string => wstring
std::wstring String2WString(const std::string& s)
{
    std::string strLocale = setlocale(LC_ALL, "");
    const char* chSrc = s.c_str();
    size_t nDestSize = mbstowcs(NULL, chSrc, 0) + 1;
    wchar_t* wchDest = new wchar_t[nDestSize];
    wmemset(wchDest, 0, nDestSize);
    mbstowcs(wchDest, chSrc, nDestSize);
    std::wstring wstrResult = wchDest;
    delete[]wchDest;
    setlocale(LC_ALL, strLocale.c_str());
    return wstrResult;
}

// 20200518/c2009.csv有25203条记录，建议MarketPrice_vector开3w
// 注意，返回的数据中,Volume是当日的积累
// 重要提醒：原始数据中，第一天的夜盘时间，是算在第二天里的！也就是说，某一天中21点的数据，出现得比当天10点的早！
//    用fix_time_problem()
// 返回：读取到的条数
size_t rfd::getMarketPrice(vector<rfd::MarketPrice> &MarketPrice_vector, const wstring & csv_path)
{
  io::CSVReader<4> in(WString2String(csv_path));
  char header[] = "localtime,InstrumentID,TradingDay,ActionDay,UpdateTime,UpdateMillisec,LastPrice,Volume,HighestPrice,LowestPrice,OpenPrice,ClosePrice,AveragePrice,AskPrice1,AskVolume1,BidPrice1,BidVolume1,UpperLimitPrice,LowerLimitPrice,OpenInterest,Turnover,PreClosePrice,PreOpenInterest,PreSettlementPrice";

  in.locate_header_with_assumption(io::ignore_extra_column, header, "TradingDay", "UpdateTime", "LastPrice", "Volume");

  std::string TradingDay, UpdateTime;
  float LastPrice, Volume;

  while (in.read_row(TradingDay, UpdateTime, LastPrice, Volume))
  {
    
    // 先获取tick对应的秒数
    std::tm datetime_tm;
    std::istringstream ss(TradingDay + "-" + UpdateTime);
    ss >> std::get_time(&datetime_tm, "%Y%m%d-%H:%M:%S"); // 获得tm结构体(年月日时分秒分别表示)


    // tm结构体转time_t(初始时间以来的秒数)
    // 关系图：https://blog.csdn.net/ffcjjhv/article/details/83376767
    time_t datetime = mktime(&datetime_tm);

    // 若解析出的时间小于一定秒数，则认为解析失败
    if (datetime < 100000)
    {
      cout << TradingDay + "-" + UpdateTime << endl;
      cout << "tick的时间数据解析失败！" << endl;
      P(datetime);
      exit(-1);
    }

    // 输出转换得到的时间用来调试
    // printf("%ld\n",datetime);
    //std::cout << std::put_time(&datetime_tm, "%c") << '\n';
    // printf("%s %s %f %f\n", TradingDay.c_str(), UpdateTime.c_str(), LastPrice, Volume);

    MarketPrice price = {datetime, LastPrice, Volume};
    MarketPrice_vector.push_back(price);
  }

  printf("现有%ld条tick记录\n", MarketPrice_vector.size());
  return MarketPrice_vector.size();
}

time_t rfd::get_seconds_by_date(string date)
{
    std::tm datetime_tm = {};
//date.insert(date.begin() + 4, '-');
//date.insert(date.begin() + 7, '-');
  std::istringstream ss(date);
  ss >> std::get_time(&datetime_tm, "%Y%m%d");
  time_t datetime = mktime(&datetime_tm);

  return datetime;
}

// 解决第一天夜盘时间数据放在第二天甚至第四天（周末或节假日）数据里的问题
// 聊聊TradingDay和ActionDay
//    https://zhuanlan.zhihu.com/p/33553653
// 思路：遍历tick，如果这个tick的小时介于19-24,0-5之间，则认为这个tick处于夜盘.
//      此时计算 [此tick - 上一个tick（日盘）]/24h，得出这两tick差了多少天，再给这个tick减去相应的天数
// 注意：对于第一部分的tick，默认把时间提前一天，这不一定准确
void rfd::fix_time_problem(vector<rfd::MarketPrice> &MarketPrice_vector)
{

  for (size_t x = 0; x < MarketPrice_vector.size(); x++)
  {
    rfd::MarketPrice &tick = MarketPrice_vector[x];
    // 计算tick的小时时间
    // 零时刻是Thu Jan  1 08:00:00 1970，所以要加8小时
    int hour = (tick.datetime + 8 * 3600) / 3600 % 24;
    bool in_night = hour >= 19 || hour <= 5;

    //P(std::ctime(&tick.datetime));P(hour);P(in_night);

    if (in_night)
    {
      // 如果此tick是第一个，则默认把时间提前1天
      if (x == 0)
      {
        tick.datetime -= 24 * 3600;
        continue;
      }

      rfd::MarketPrice &last_tick = MarketPrice_vector[x - 1];
      int days_interval = (tick.datetime - last_tick.datetime) / (24 * 3600);

     tick.datetime -= (long long) days_interval * 24 * 3600;
      continue;
    }
  }

  // 检查时间顺序是否一致
  time_t t = 0;
  for (auto &tick : MarketPrice_vector)
  {
    if (t > tick.datetime)
    {
      P(std::ctime(&t));
      P(std::ctime(&tick.datetime));
      cout << "时间顺序不是顺的！" << endl;
      exit(-1);
    }
    t = tick.datetime;
  }
}

bool rfd::sort_by_date(fs::path p1, fs::path p2)
{
  // cout << p1.parent_path().filename() << endl;
  // cout << p2.parent_path().filename() << endl;

    auto a = p1.parent_path().filename();
  time_t datetime_p1 = get_seconds_by_date(p1.parent_path().filename().string());
  time_t datetime_p2 = get_seconds_by_date(p2.parent_path().filename().string());

  // P(p1);
  // P(datetime_p1);
  // P(p2);
  // P(datetime_p2);

  // 防止解析失败（得到1970年日期）
  if (datetime_p1 < 100000 || datetime_p2 < 100000)
  {
    cout << "时间解析失败！" << endl;
    P(p1);
    P(p2);
    exit(-1);
  }

  if (datetime_p1 == datetime_p2)
  {
    cout << "不能有同一日期的csv！" << endl;
    P(p1);
    P(p2);
    exit(-1);
  }

  return datetime_p1 < datetime_p2;
}

// path_vector:目标csv的vector
// 递归遍历目录下的所有csv
// 返回：已按日期排序（从旧到新）的目标csv文件列表
size_t rfd::get_satisfied_paths(vector<fs::path> &path_vector, const wstring &path, const string &code)
{

  string filename_to_find = code + ".csv";

  //cout << filename_to_find << endl;

#ifdef _WIN32
  for (auto &f : fs::directory_iterator(path))
  {
    if (f.is_directory())
    {
        get_satisfied_paths(path_vector, f.path().c_str(), code);
      continue;
    }
    else
    {
      string filename = f.path().filename().string();
      if (filename == filename_to_find)
      {
        path_vector.push_back(f.path());
      }
    }
  }

#else
  auto a = WString2String(path);
  for (auto& f : fs::directory_iterator(a))
  {
      if (f.is_directory())
      {

          get_satisfied_paths(path_vector, String2WString(f.path().c_str()), code);

          continue;
      }
      else
      {
          string filename = f.path().filename().string();
          if (filename == filename_to_find)
          {
              path_vector.push_back(f.path());
          }
      }
  }
 
#endif
  std::sort(path_vector.begin(), path_vector.end(), sort_by_date);

  return path_vector.size();
}

void rfd::save_binary_tick_file(const wstring &path, const string &code)
{
  vector<rfd::MarketPrice> MarketPrice_vector;
  size_t price_size = collect_all_ticks(MarketPrice_vector, path, code);

  FILE *file = fopen((WString2String(path)+ code + ".bin").c_str(), "wb");
  fwrite(&price_size, sizeof(price_size), 1, file);
  for (size_t i = 0; i < price_size; i++)
  {
    fwrite(&MarketPrice_vector[i], sizeof(rfd::MarketPrice), 1, file);
  }
  fflush(file);
  fclose(file);
  MarketPrice_vector.clear();
}

size_t rfd::read_binary_tick_file(vector<rfd::MarketPrice> &MarketPrice_vector, const wstring &path, const string &code)
{
  FILE *file = fopen((WString2String(path) + code + ".bin").c_str(), "rb");

  size_t price_size;
  fread(&price_size,sizeof(price_size),1,file);
  for(size_t i=0;i<price_size;i++)
  {
    rfd::MarketPrice price;
    fread(&price,sizeof(rfd::MarketPrice),1,file);
    MarketPrice_vector.push_back(price);
  }
  return MarketPrice_vector.size();
}

// 这个函数用于收集指定目录（递归）下所有的code.csv的数据
// 重要提醒：原始数据中，第一天的夜盘时间，是算在第二天里的！也就是说，某一天中21点的数据，出现得比当天10点的早！
//    用fix_time_problem()理顺时间（已自动这样做）
// 已知bug：第二次调用该函数时，tick的时间数据会解析失败，原因未知
size_t rfd::collect_all_ticks(vector<rfd::MarketPrice> &MarketPrice_vector, const wstring &path, const string &code)
{
  // 获取所有csv文件
  vector<fs::path> path_vector;
  rfd::get_satisfied_paths(path_vector, path, string("c2009"));
  for (fs::path &p : path_vector)
  {
    cout << p << endl;
  }

  // 分别把数据装入vector中
  for (fs::path &p : path_vector)
  {
#ifdef _WIN32
      getMarketPrice(MarketPrice_vector, p.c_str());
#else
      getMarketPrice(MarketPrice_vector,String2WString( p.c_str()));
#endif
  }

  fix_time_problem(MarketPrice_vector); // 重要
  return MarketPrice_vector.size();
}

// 以seconds为间隔，进行重采样，若两条数据的间隔小于1倍，则舍弃这一条；若大于1.5倍，则在中间插入一条不变值
// 在sample中,保证此tick实际时间比struct里的datetime字段要早
int rfd::resample(vector<rfd::MarketPrice> &MarketPrice_vector_original, vector<rfd::MarketPrice> &MarketPrice_vector_sample, size_t seconds)
{
  /*
      typedef struct
    {
        time_t datetime;
        float LastPrice;
        float Volume;
    } MarketPrice;
  */

  if (MarketPrice_vector_sample.size() != 0)
  {
    cout << "警告：MarketPrice_vector_sample().size()!=0" << endl;
  }

  // 下一条要选取的tick（此tick要发生在time_for_choose或之前）
  time_t time_for_choose = 0;
  // 标识交易时段是否结束（开始时视作结束）
  bool is_trading_period_over = true;
  rfd::MarketPrice tick;
  for (size_t x = 0; x < MarketPrice_vector_original.size();)
  {
    tick = MarketPrice_vector_original[x];
    time_t delta_time = tick.datetime - time_for_choose;

    // 若交易时段已结束，选取这一tick作为开始
    if (is_trading_period_over)
    {
      is_trading_period_over = false;
      //cout << "交易时段在" << std::ctime(&tick.datetime) << "开始---" << endl;
      time_for_choose = tick.datetime;
      MarketPrice_vector_sample.push_back(tick);
      x++;
      continue;
    }

    // 当delta_time大于3600s时，认为此交易时段已经结束，跳过这一tick，并标识交易时段已结束
    if (delta_time > 3600)
    {
      is_trading_period_over = true;
      x++;
      //cout << "交易时段在" << std::ctime(&time_for_choose) << "结束！" << endl;
      continue;
    }

    // 计算上下两时间差与所需时间差的比值，如果大于1.5倍，则中间插入一个不变的时刻
    // d<1.0：跳过这一条            x++
    // 1.0<= d <=1.5：选择这一条    x++
    // 1.5< d：插入一条不变的数据    x不变
    float times = delta_time / (float)seconds;
    if (times < 1.0)
    {
      x++;
      continue;
    }
    else if (1.0 <= times && times <= 1.5)
    {
      time_for_choose += seconds;
      MarketPrice_vector_sample.push_back(tick);
      x++;
      continue;
    }
    else if (times > 1.5)
    {
      time_for_choose += seconds;
      MarketPrice_vector_sample.push_back(tick);
      // x不变
      continue;
    }
  }
  return MarketPrice_vector_sample.size();
}

// 如何获取数据的例子
// int main1()
// {
//   vector<rfd::MarketPrice> MarketPrice_vector;
//   string path = "/mnt/c/Users/QinHuoBin/Downloads/2020.5.18~2020.6.10.期货全市场行情数据/Data/";
//   string code = "c2009";
//   collect_all_ticks(MarketPrice_vector, path, code);
//   printf("一共有%ld条tick记录\n", MarketPrice_vector.size());

//   vector<rfd::MarketPrice> MarketPrice_vector_sample;
//   resample(MarketPrice_vector, MarketPrice_vector_sample, 30);
//   printf("30s一共有%ld条tick记录\n", MarketPrice_vector_sample.size());
// }