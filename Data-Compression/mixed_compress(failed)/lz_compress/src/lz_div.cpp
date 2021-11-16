//
// Created by 颜嘉钦 on 11/2/21.
//

#include "../include/lz_div.h"
#include <iostream>
#include <sstream>
#include <fstream>

const size_t MAX_LENGTH = 64;
const size_t RECORD_SIZE = 128;

//11.5MB/32B=384个采样
//384可以用9bit表示
const size_t MAX_DISTANCE = 256*256*32;

const size_t DISTANCE_DIVISION = 32;

inline bool accept_match_case(int distance,int length){

    //不是16的整数倍就false
    //该条件挪入算法中提前运行，以此加速

    if(length>2){
        return true;
    }else
    if(length==2){
        if(distance>>8 == 0){
            return true;
        }
    }
    return false;
}

int small_than_256 = 0;
int from_256_to_512 = 0;
int bigger_than_512 = 0;
void push_d_l(int distance,int length,vector<unsigned char> &c){
    distance = distance/DISTANCE_DIVISION;
    if(distance < 256){
        small_than_256++;
        c.push_back((unsigned char)length);
        c.push_back((char)distance);
    }else{
        if(distance<512){
            from_256_to_512++;
        }else{
            bigger_than_512++;
        }
        c.push_back((unsigned char)length+128);
        distance-=256;
        c.push_back((char)distance);
    }
}

struct pos_list{
    unsigned int position[RECORD_SIZE];
    unsigned int current = 0;
    unsigned int size = 0;
    void put(unsigned int pos){
        current = (current+1)%RECORD_SIZE;
        position[current] = pos;
        if(size<RECORD_SIZE){
            size++;
        }
    }
    unsigned int get(int p){
        return position[(current-p+RECORD_SIZE)%RECORD_SIZE];

    }
    unsigned int get_size(){
        return size;
    }

    void clear(){
        size = 0;
        current = 0;
    }
};

pos_list codes[256 * 256];
void lz_div(const unsigned char* data, unsigned int position,const int read_size, vector<unsigned char> &out){
    int header_index = 0;

    vector<unsigned char> ori_data;
    while(header_index < read_size){

        pos_list poslist = codes[(data[header_index]<<8) + data[header_index+1]];

        int match_length = 0;
        int distance = 0;
        //以上为distance,length对，执行下面的while后就可以获得

        for(int a = 0;a<poslist.get_size();a++){
            int temp_distance = position+header_index-poslist.get(a);
            if(temp_distance>MAX_DISTANCE || temp_distance<=0){
                continue;
            }
            if((distance%DISTANCE_DIVISION)!=0){
                continue;
            }
            int temp_length = 0;

            const unsigned char* model = data+header_index;
            const unsigned char* compare = data+header_index-temp_distance;
            while(compare!=nullptr && *model==*compare){
                //cout<<"check "<<model<<" "<<compare<<endl;
                model++;
                compare++;
                temp_length++;
            }
            if(accept_match_case(temp_distance,temp_length)){
                int check_temp_length = temp_length;
                int check_length = temp_length;
                if(distance>256){
                    check_length--;
                }
                if(temp_distance>256){
                    check_temp_length--;
                }
                if(temp_length>match_length ||
                    (temp_length==match_length && temp_distance<distance)){
                    match_length = temp_length;
                    distance = temp_distance;
                }
            }
        }
        //寻找最大匹配字符串

        if(match_length==0){
            ori_data.push_back(data[header_index]);
            codes[(data[header_index]*256+data[header_index+1])].put(position+header_index);
            header_index++;

            if(ori_data.size()>=127-MAX_LENGTH){
                int size = ori_data.size();
                out.push_back(size+MAX_LENGTH);
                for(int a = 0;a<size;a++){
                    out.push_back(ori_data[a]);
                }
                ori_data.clear();
            }
            //data内容过多提前输出
            continue;
        }
        //没有匹配则保存元数据，进行下次循环。

        if(!ori_data.empty()){
            int size = ori_data.size();
            out.push_back(size+MAX_LENGTH);
            for(int a = 0;a<size;a++){
                out.push_back(ori_data[a]);
            }
            ori_data.clear();
        }
        //输出d,l对前如果发现有元数据未输出，则先输出元数据。

        if(match_length>MAX_LENGTH){
            match_length=MAX_LENGTH;
        }
        if(header_index+match_length > read_size){
            int delta = header_index+match_length - read_size;
            match_length-=delta;
            header_index = read_size;
        }

        //如果处理字符个数超过要求处理个数，则裁剪length

        push_d_l(distance,match_length,out);
        //cout<<match_length<<" "<<distance<<" "<<endl;

        for(int a = 0;a<match_length;a++){
            codes[(data[header_index]*256+data[header_index+1])].put(position+header_index);
            //cout<<"put: " <<" "<<position+(header_index)<<endl;
            header_index++;
        }
        //将新内容放入code表。
    }

    if(!ori_data.empty()){
        int size = ori_data.size();
        out.push_back(size+MAX_LENGTH);
        for(int a = 0;a<size;a++){
            out.push_back(ori_data[a]);
        }
        ori_data.clear();
    }
    cout<<small_than_256<<"\t"<<from_256_to_512<<"\t"<<bigger_than_512<<endl;
}
void lz_read(const unsigned char* data, unsigned int position,const int read_size){
    for(int a = 0;a<read_size-1;a++){
        codes[data[a]*256+data[a+1]].put(position+a);
        //cout<<"put: " <<data[a]<<data[a+1]<<" "<<position+a<<endl;
    }
}

void lz_reset(){
    for(int a = 0;a<256*256;a++){
        codes[a].clear();
    }
    from_256_to_512 = 0;
    small_than_256 = 0;
    bigger_than_512 = 0;
}

void lz_decompress(const vector<unsigned char> &data,vector<unsigned char> &out){
    size_t header_index = 0;

    while(header_index<data.size()){
        unsigned char length = data[header_index];
        bool two_bit_distance = false;
        if(length>128){
            length-=128;
            two_bit_distance = true;
        }
        if(length>MAX_LENGTH){
            header_index++;
            for(int a = 0;a<length-MAX_LENGTH;a++){
                out.push_back(data[header_index]);
                header_index++;
            }
            //读取元数据
        }else{
            header_index++;
            int distance = data[header_index];
            if(two_bit_distance){
                header_index++;
                distance += data[header_index]*256;
            }
            distance=distance*DISTANCE_DIVISION;
            //读取distance数据
            int model_index = out.size()-distance;
            //cout<<"model start "<<(int)(out[model_index])<<endl;
            for(int a = 0;a<length;a++){
                out.push_back(out[model_index]);
                model_index++;
            }
            header_index++;
        }
    }
}



int mainn(){
    unsigned char p[32] = {5,6,7,7,7,5,6,1,5,6,8,1,5,6,8,1,5};
    vector<unsigned char> out;
    vector<unsigned char> decom;
    lz_div(p,0,16,out);
    for(unsigned char c : out){
        cout<<(int)c<<" ";
    }
    cout<<endl;
    lz_decompress(out,decom);
    for(unsigned char c : decom){
        cout<<(int)c<<" ";
    }
}