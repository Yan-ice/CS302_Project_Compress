/*
 * @Author         : Long XIANG
 * @AuthorMail     : xiangl3@mail.sustech.edu.cn
 * @LastEditor     : Long XIANG
 * @LastEditorMail : xiangl3@mail.sustech.edu.cn
 * @Version        : 2.0
 * @Description    : 压缩引擎一，使用增量编码（delta coding）剔除数据帧格式数据中，连续采样之间未发生变化的数据以实现数据压缩的目的
 *                 : 并且保留增量数据，周期数据中高频出现的数据模式（frequency pattern）为后一级的压缩做好数据准备
 *                 : 此版本将delta_coding部分完全独立出来供上板实现
 */

#include <iostream>
#include <vector>
#include <string>
#include "include/lz_div.h"
//#include "./d_compress/src/coding_1.cpp"
#include <sstream>
#include <fstream>

using namespace std;

std::vector<unsigned char> read_file(std::ifstream& in) {
    std::vector<unsigned char> tmp((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return tmp;
}
void write_result(std::ofstream& out, const std::vector<unsigned char>& result) {
//    for (const auto& ch : result) {
//        out << ch;
//    }
    out.write(reinterpret_cast<const char*>(&result[0]), result.size()*sizeof(unsigned char));
}


//#define UNCOMPRESS_MODE


#ifndef UNCOMPRESS_MODE

int main(int argc, char* argv[]) {

    string m_list[] = {"256","512","1024","4096"};
    string r_list[] = {"2","3","11","20","40"};
    double start_time = clock();
    for(string M : m_list){
        for(string R : r_list){

            stringstream fmt2;
            fmt2<<"./"<<M<<"模式/DataFrameM"<<M<<"R"<<R<<".bin";
            cout<<fmt2.str().c_str()<<endl;
            std::ifstream in(fmt2.str().c_str(), std::ios_base::binary);
            if (in.fail()) {
                std::cout << "Error: Open input file Fail!" << std::endl;
                continue;
                //exit(1);
            }
            stringstream fmt;
            fmt << "./Output/OutputM" << M << "R" << R<<".bin";
            std::ofstream out(fmt.str().c_str(), std::ios_base::binary);

            if (out.fail()) {
                std::cerr << "Error: Open output file Fail!" << std::endl;
                exit(1);
            }

            vector<unsigned char> file_data = read_file(in);

            const size_t kDataSize = file_data.size();

            vector<unsigned char> result;
            result.reserve(kDataSize*2);
            lz_div(&file_data[0], 0, kDataSize, result);

            write_result(out, result);
            lz_reset();
        }
    }

    double end_time = clock();
    printf("time total: %.3f s\n",(end_time-start_time)/1000000);

    return 0;
}
#endif

#ifdef UNCOMPRESS_MODE
int main(){
    string m_list[] = {"256","1024","4096"};
    string r_list[] = {"2","3","11","20","40"};
    double start_time = clock();
    for(string M : m_list){
        for(string R : r_list){

            stringstream fmt2;
            fmt2<<"./Output/OutputM" << M << "R" << R<<".bin";
            cout<<fmt2.str().c_str()<<endl;
            std::ifstream in(fmt2.str().c_str(), std::ios_base::binary);
            if (in.fail()) {
                std::cout << "Error: Open input file Fail!" << std::endl;
                continue;
                //exit(1);
            }
            stringstream fmt;
            fmt << "./Output/Decompress_M" << M << "R" << R<<".bin";
            std::ofstream out(fmt.str().c_str(), std::ios_base::binary);

            if (out.fail()) {
                std::cerr << "Error: Open output file Fail!" << std::endl;
                exit(1);
            }

            vector<unsigned char> file_data = read_file(in);

            const size_t kDataSize = file_data.size();

            vector<unsigned char> result;
            result.reserve(kDataSize*2);
            lz_decompress(file_data,result);

            write_result(out, result);
            lz_reset();
        }
    }

    double end_time = clock();
    printf("time total: %.3f s\n",(end_time-start_time)/1000000);

    return 0;
}
#endif