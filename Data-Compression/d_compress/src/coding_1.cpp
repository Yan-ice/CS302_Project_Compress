#include "../include/coding_1.h"
#include "../include/frame_io.h"
#include "../include/utils.h"
#include "../../mixed_compress/lz_compress/include/lz_div.h"
#include <iostream>
using namespace std;

void delta_compress(const unsigned char* data, const size_t kDataSize, std::vector<unsigned char> &result) {
    long long timestamp;
    unsigned char model, channel;
    get_header(&data[0], timestamp, model, channel);
    push_integer_to_vector(channel, result);
    push_integer_to_vector(model, result);
    push_integer_to_vector(timestamp, result);

    const int kSampleSize = kSampleSizeList[model];
    unsigned char looked_buff[kSampleSize];
    memset(looked_buff, 0, kSampleSize*sizeof(unsigned char));

    long long last_change_ts = timestamp-1;
    long long current_ts = timestamp;
    for (size_t i=0; i<kDataSize; i+=kFrameSize) { //处理一个数据帧

        result.push_back(0);
        vector<unsigned char>::iterator timestamp_flag = result.end()-1;
        //TODO: 这里应该需要通过数据帧的头信息读取帧大小到timestamp_flag里。
        for (int para = 0;para<4;para++) { //处理一个dataframe,pos为一段开始的位置,para为段序数。
            size_t pos = i+kBytePerLine+256*para;

            std::vector<unsigned char> sample_coding_list;//该list为增量编码压缩一段的成果。
            std::vector<unsigned char> lz77_coding_list;//该list为LZ77编码压缩一段的成果。
            //以下是增量编码处理方式。
            for (int row_start=0, row_idx=0; row_start<256; row_start+=kBytePerLine, ++row_idx) { //处理一段为8行

                std::vector<unsigned char> changed_list;
                int32_t changed_flag = 0;
                for (int idx = 0; idx < kBytePerLine; ++idx) {
                    size_t para_idx = pos + row_start + idx;
                    size_t looked_idx = (para_idx-i-kBytePerLine)%kSampleSize;
                    if (data[para_idx] != looked_buff[looked_idx]) {
                        //(pos-i-kBytePerLine)%kSampleSize 是本次处理段开头在sample中的相对位置。
                        changed_flag += (1 << idx);
                        changed_list.push_back(data[para_idx]);
                        looked_buff[looked_idx] = data[para_idx];
                    }
                }
                sample_coding_list.push_back(kRowFront + row_idx); // 添加(行标签)
                auto *p = (unsigned char *) &changed_flag;
                for (int a = 0; a < 4; a++) {
                    sample_coding_list.push_back(*p);
                    p++;
                }

                for (auto row_code : changed_list) { // 将一行中的编码添加至sample_coding
                    sample_coding_list.push_back(row_code);
                }
                changed_list.clear();
            }


            if(sample_coding_list.size()>0){
                //以下是LZ77处理方式。
                lz_div(data+pos, pos,256, lz77_coding_list);
                //cout<<lz77_coding_list.size()<<"\t"<<sample_coding_list.size()<<endl;
                if(lz77_coding_list.size()<sample_coding_list.size()){
                    *timestamp_flag += 1<<para;
                    for (const auto& ts_code : lz77_coding_list) {
                        result.push_back(ts_code);
                    }
                }else{
                    for (const auto& ts_code : sample_coding_list) {
                        result.push_back(ts_code);
                    }
                }
            }else {
                //以下是不采用LZ77处理方式。
                lz_read(data + pos, pos, 256);
                for (const auto &ts_code : sample_coding_list) {
                    result.push_back(ts_code);
                }
            }

        } // 完成1个dataframe
    }
    auto tail_ts_code = integer_to_array(current_ts-last_change_ts); // 尾编码,为了防止最后多个采样数据一致造成数据丢失,补一个delta_ts
    for (const auto &code : tail_ts_code) {
        result.push_back(code);
    }
//    delete[] looked_buff;
}

std::vector<unsigned char> gen_header_line(const long long timestamp, const unsigned char model,
                                           const unsigned char channel, const unsigned int packet_count) {
    std::vector<unsigned char> head_line = {0,0,0,0,0,0,16,0,32,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,103,69,35,1,0,0,0,0};
    for (int i=0, j=0; i<=5; ++i, ++j) {
        head_line[i] = (timestamp >> (j*8)) & 0xFF;
//        std::cout << "head_line i: " << i << ", " << (int)head_line[i] << std::endl;
    }

    unsigned short code_model = 16*(1<<model);
//    std::cout << "model_number: " << code_model << std::endl;
    for (int i=6, j=0; i<=7; ++i, ++j) {
        head_line[i] = (code_model >> (j*8)) & 0xFF;
//        std::cout << "head_line i: " << i << ", " << (int)head_line[i] << std::endl;
    }

    for (int i=9, j=0; i<=12; ++i, ++j) {
        head_line[i] = (packet_count >> (j*8)) & 0xFF;
//        std::cout << "head_line i: " << i << ", " << (int)head_line[i] << std::endl;
    }

    head_line[14] = channel+1;

    return head_line;
}

void append_sample(long long &timestamp, const unsigned char kModel, const unsigned char kChannel,
                   const int kSampleLines, size_t &line_count, unsigned int &packet_count,
                   const std::vector<unsigned char> &looked_sample, std::vector<unsigned char>&out) {
    if (line_count%34==0) {
        auto header_line = gen_header_line(timestamp, kModel, kChannel, packet_count);
        out.insert(out.end(), header_line.cbegin(), header_line.cend());
        ++line_count;
    }
    out.insert(out.end(), looked_sample.cbegin(), looked_sample.cend());
    line_count += kSampleLines;
    ++timestamp;
//    int i=0;
//    for (auto &ele: out) {
//        std::cout << (int)ele << ", ";
//        ++i;
//        if (i%32==0) {
//            std::cout << std::endl;
//        }
//    }
//    std::cout << "looked_sample: ";
//    for (auto &ele: looked_sample) {
//        std::cout << (int)ele << ", ";
//    }
//    std::cout << std::endl;
//    std::cout << "line_count: " << line_count << std::endl;
//    std::cout << "timestamp: " << timestamp << std::endl;
    if (line_count%34==33) {
        out.insert(out.end(), kTailLine.cbegin(), kTailLine.cend());
        ++line_count;
        ++packet_count;
    }
}

//void delta_decompress(std::vector<unsigned char> &in, std::vector<unsigned char> &out) {
//    auto channel = array_to_integer<unsigned char>(&in[0], sizeof(unsigned char));
//    auto model = array_to_integer<unsigned char>(&in[1], sizeof(unsigned char));
//    auto timestamp  = array_to_integer<long long>(&in[2], sizeof(long long));
//
//    std::cout << "channel: " << (int)channel << ", model: " << (int)model << ", timestamp: " << timestamp << std::endl;
//
//    std::vector<unsigned char> looked_sample(kSampleSizeList[model]);
//    const int kSampleLines = kSampleLineList[model];
//
////    auto header_line = gen_header_line(timestamp, model, channel, 0); //得到第一帧帧头
////    out.insert(out.end(), header_line.cbegin(), header_line.cend()); //将第一帧帧头写入结果文件
////    for (auto &ele: out) {
////        std::cout << (int)ele << ", " ;
////    }
////    std::cout << std::endl;
//    unsigned int packet_count = 0;
//    size_t line_count = 0;
//    bool is_first = true;
//
//    unsigned char row;
//    size_t pos = 10;
//    while(pos<in.size()-8) {
////        std::cout << "pos: " << pos << ", in[pos]: " << (int)in[pos++] << std::endl;
//        if(in[pos]<32) { // 此时读入的是delta timestamp, 将delta_ts间未变化的sample补齐
//            long long delta_ts;
//            if(in[pos]<16) {
//                delta_ts = in[pos]+1;
//                ++pos;
//            } else {
//                int ts_len = in[pos]-16;
//                delta_ts = array_to_integer<long long>(&in[pos+1], ts_len);
//                pos += ts_len;
//            }
//
////            std::cout << "pos: " << pos << ", delta_ts: " << delta_ts << std::endl;
//            if (is_first && delta_ts==1) {
//                is_first = false;
//            } else {
//                for (int i=0; i<delta_ts; ++i) {
////                    std::cout << "timestamp: " << timestamp << ", model: " << (int)model << ", channel: " << (int)channel <<", line_count: " << line_count << ", packet_count: " << packet_count << std::endl;
//                    append_sample(timestamp, model, channel, kSampleLines, line_count, packet_count,
//                                  looked_sample, out); // 补齐delta_ts间的sample
//                }
//            }
//        } else if (in[pos]>=32 && in[pos]<64) { // 此时读入的是row,由此开始还原出一个sample
//            row = in[pos]-kRowFront;
////            std::cout << "row: " << (int)row << ", pos: " << pos << ", in[pos]: " << (int)in[pos] << std::endl;
//            ++pos;
//        } else if (in[pos]>=128) {
//            unsigned char col_group = (in[pos] & 0x70)>>4;
//            unsigned char col_flag  = in[pos] & 0x0f;
////            std::cout << "pos: " << pos << ", in[pos]: " << (int)in[pos] << ", col_group: " << (int)col_group << ", col_flag: " << (int)col_flag << std::endl;
//            for (int idx=0; idx<kGroupSize; ++idx) {
//                if (col_flag & (1<<idx)) {
//                    looked_sample[row*kBytePerLine+col_group*kGroupSize+idx] = in[++pos];
////                    std::cout << "looked_sample[" << row*kBytePerLine+col_group*kGroupSize+idx << "]=" << (int)looked_sample[row*kBytePerLine+col_group*kGroupSize+idx]<<std::endl;
//                }
//            }
//            ++pos;
////            if (in[pos+1]<32) {
////                append_sample(timestamp, model, channel, kSampleLines, line_count, packet_count,
////                              looked_sample, out);
////            }
//        }
//    }
//    auto tail_ts = array_to_integer<long long>(&in[pos], 8);
//    for (int i=0; i<tail_ts; ++i) {
//        if (line_count%34==0) {
//            auto header_line = gen_header_line(timestamp, model, channel, packet_count);
//            out.insert(out.end(), header_line.cbegin(), header_line.cend());
//            ++line_count;
//        }
//        out.insert(out.end(), looked_sample.cbegin(), looked_sample.cend());
//        line_count += kSampleLines;
//        ++timestamp;
//        if (line_count%34==33) {
//            out.insert(out.end(), kTailLine.cbegin(), kTailLine.cend());
//            ++line_count;
//            ++packet_count;
//        }
//    }
//}
