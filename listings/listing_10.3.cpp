/*** 
 * @Author: baisichen
 * @Date: 2023-04-26 16:40:32
 * @LastEditTime: 2023-07-05 23:03:22
 * @LastEditors: baisichen
 * @Description: 
 */
#include <vector>
#include <string>
#include <unordered_map>
#include <numeric>
#include <execution>
struct log_info {
    std::string page;
    time_t visit_time;
    std::string browser;
    // any other fields
};

extern log_info parse_log_line(std::string const &line);

using visit_map_type= std::unordered_map<std::string, unsigned long long>;

visit_map_type
count_visits_per_page(std::vector<std::string> const &log_lines) {

    struct combine_visits {
        visit_map_type
        operator()(visit_map_type lhs, visit_map_type rhs) const {
            if(lhs.size() < rhs.size())
                std::swap(lhs, rhs);
            for(auto const &entry : rhs) {
                lhs[entry.first]+= entry.second;
            }
            return lhs;
        }

        visit_map_type operator()(log_info log, visit_map_type map) const {
            ++map[log.page];
            return map;
        }
        visit_map_type operator()(visit_map_type map, log_info log) const {
            ++map[log.page];
            return map;
        }
        visit_map_type operator()(log_info log1, log_info log2) const {
            visit_map_type map;
            ++map[log1.page];
            ++map[log2.page];
            return map;
        }
    };
    /*
        transform_reduce的运行流程是，对容器log_lines的每个元素运行parse_log_line()函数(操作范围从begin()开始到end结束)，得到一系列对应的中介结果，其类型为log_info结构体，然后对相邻的log_info结构体执行combine_visits()
        因为transform_reduce的执行策略设定成std::execution::par, 所以combine_visits()会由多个线程执行。
        若线程遇到两个相邻的log_info结构体，则整合成一个map作为中间结果；其他线程也有可能遇到相邻的一个map和一个log_info结构体，因而也要对其整合；还有可能遇到两个相邻的map，同样整合。(所以有四个combine_visits的重载函数)
        其执行过程是自下而上的汇聚行为。原始信息有如底层雪花，相邻的雪花汇聚成小雪球，小雪球再与旁边的雪花汇聚，或与相邻的两个小雪球汇聚成大雪球。这些汇聚操作在整个容器范围内并行发生。
    */
    return std::transform_reduce(
        std::execution::par, log_lines.begin(), log_lines.end(),
        visit_map_type(), combine_visits(), parse_log_line);
}
