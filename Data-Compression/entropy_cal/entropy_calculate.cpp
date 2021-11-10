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
#include "include/frame_io.h"
#include "include/coding_1.h"
#include <sstream>

using namespace std;
int main(int argc, char* argv[]) {

    string m_list[] = {"256","1024","4096"};
    string r_list[] = {"2","3","11","20","40"};

    for(string M : m_list){
        for(string R : r_list){

            stringstream fmt2;
            fmt2 << "./Output/OutputM" << M << "R" << R<<".bin";
            //fmt2<<"./"<<M<<"模式/DataFrameM"<<M<<"R"<<R<<".bin";
            cout<<fmt2.str().c_str()<<endl;
            std::ifstream in(fmt2.str().c_str(), std::ios_base::binary);
            if (in.fail()) {
                std::cout << "Error: Open input file Fail!" << std::endl;
                continue;
                //exit(1);
            }

            std::vector<unsigned char> file_data = read_file(in);
            int cal[256] = {0};
            double total = file_data.size();
            double ent = 0;

            for(unsigned char c : file_data){
                cal[c]++;
            }
            for(int a = 0;a<256;a++){
                if(cal[a]!=0){
                    double p = cal[a]/total;

                    ent+= p* log2(1/p);
                }
            }
            cout<<"entropy: "<<ent<<endl;

        }
    }


    return 0;
}
