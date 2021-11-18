//
// Created by 颜嘉钦 on 11/2/21.
//

#include "../include/lz_div.h"
#include <iostream>
#include <sstream>
#include <fstream>

const size_t RECORD_SIZE = 128;

const char code[32]{
    0,2,3,4,5,6,7,8,
    9,10,11,12,14,16,18,20,
    22,24,26,28,30,32,34,36,
    38,40,43,46,49,52,55,60
};

char cut_length(int &length){
    if(length<2){
        cout<<"WARN: length smaller than 2 is denied";
    }
    for(int a = 31;a>=0;a--){
        if(code[a]<=length){
            length = code[a];
            return a;
        }
    }
}
char resume_length(char code_){
    code_ = code_%32;
    return code[code_];
}



size_t MAX_DISTANCE = 128*256*32;
size_t DISTANCE_DIVISION = 32;

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

void push_d_l(int distance,int length,vector<unsigned char> &new_char,vector<unsigned char> &c){
    //cout<<DISTANCE_DIVISION<<" "<<distance<<" "<<length<<endl;
    distance = distance/DISTANCE_DIVISION;

    length+=new_char.size()<<6;

    if(length%64!=0){
        if(distance>=256){
            length+=1<<5;
            c.push_back((unsigned char)length);
            c.push_back((unsigned char)distance%256);
            c.push_back((unsigned char)distance/256);
        }else{
            c.push_back((unsigned char)length);
            c.push_back((unsigned char)distance);
        }
    }else{
        c.push_back((unsigned char)length);
    }
    for(unsigned char ch : new_char){
        c.push_back(ch);
    }
}

struct pos_list{
    unsigned short position[RECORD_SIZE];
    unsigned char current = 0;
    unsigned char size = 0;
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
void lz_div(int division, const unsigned char* data, unsigned int position,const int read_size, vector<unsigned char> &out){
    DISTANCE_DIVISION = division;
    MAX_DISTANCE = 256*256*division;

    int header_index = 0;

    vector<unsigned char> ori_data;

    for(;header_index<division;header_index++){
        out.push_back(data[header_index]);
    }

    while(header_index < read_size){

        pos_list poslist = codes[(data[header_index]<<8) + data[header_index+1]];

        int match_length = 0;
        char length_code = 0;
        int distance = 0;
        //以上为distance,length对，执行下面的while后就可以获得

        for(int a = 0;a<poslist.get_size();a++){
            int temp_distance = position+header_index-poslist.get(a);
            if(temp_distance>MAX_DISTANCE || temp_distance<=0){
                continue;
            }
            if((temp_distance%DISTANCE_DIVISION)!=0){
                continue;
            }
            int temp_length = 0;
            char temp_code = 0;
            const unsigned char* model = data+header_index;
            const unsigned char* compare = data+header_index-temp_distance;
            while(compare!=nullptr && *model==*compare){
                //cout<<"check "<<model<<" "<<compare<<endl;
                model++;
                compare++;
                temp_length++;
            }
            if(accept_match_case(temp_distance,temp_length)){
                temp_code = cut_length(temp_length);

                if(temp_code>length_code ||
                    (temp_code==length_code && temp_distance<distance)){
                    match_length = temp_length;
                    length_code = temp_code;
                    distance = temp_distance;
                }
            }
        }
        //寻找最大匹配字符串

        if(match_length==0){

            if(ori_data.size()>=3){
                push_d_l(0,0,ori_data,out);
                ori_data.clear();
            }
            //data内容过多提前输出

            ori_data.push_back(data[header_index]);
            codes[(data[header_index]*256+data[header_index+1])].put(position+header_index);
            header_index++;
            continue;
        }
        //没有匹配则保存元数据，进行下次循环。

        if(header_index+match_length > read_size){
            int delta = header_index+match_length - read_size;
            match_length-=delta;
            header_index = read_size;
        }
        //如果处理字符个数超过要求处理个数，则裁剪length

        push_d_l(distance,length_code,ori_data,out);
        ori_data.clear();

        //cout<<match_length<<" "<<distance<<" "<<endl;

        for(int a = 0;a<match_length;a++){
            codes[(data[header_index]*256+data[header_index+1])].put(position+header_index);
            //cout<<"put: " <<" "<<position+(header_index)<<endl;
            header_index++;
        }
        //将新内容放入code表。
    }

    if(!ori_data.empty()){
        push_d_l(0,0,ori_data,out);
    }
    //cout<<"origin:"<<origin<<" - olc:"<<olc<<endl;
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
}

void lz_decompress(int division, const vector<unsigned char> &data,vector<unsigned char> &out){
    DISTANCE_DIVISION = division;
    size_t header_index = 0;

    for(;header_index<division;header_index++){
        out.push_back(data[header_index]);
    }

    while(header_index<data.size()){
        unsigned char length_byte = data[header_index];
        unsigned char char_length = length_byte/64;
        bool two_byte_length = (length_byte/32)%2 == 1;

        char length = resume_length(length_byte%32);

        int distance = 0;
        if(length!=0){
            header_index++;
            distance = data[header_index];
            if(two_byte_length){
                header_index++;
                distance += data[header_index]*256;
            }
            distance*=DISTANCE_DIVISION;
        }
        //cout<<"length:"<<(int)length<<" char:"<<(int)char_length<<" distance:"<<distance<<endl;
        header_index++;
        for(int a = 0;a<char_length;a++){
            out.push_back(data[header_index]);
            header_index++;
        }

        //读取distance数据
        int model_index = out.size()-distance;
        //cout<<"model start "<<(int)(out[model_index])<<endl;
        for(int a = 0;a<length;a++){
            out.push_back(out[model_index]);
            model_index++;
        }
    }
}

