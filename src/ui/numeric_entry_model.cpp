#include "numeric_entry_model.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
void NumericEntryModel::open(float value,float minimum,float maximum,uint8_t decimals) {
    minimum_=minimum;maximum_=maximum;decimals_=std::min<uint8_t>(decimals,3);
    std::snprintf(text_.data(),text_.size(),"%.*f",decimals_,static_cast<double>(value));replace_=true;
}
void NumericEntryModel::append(char key) {
    if(!((key>='0' && key<='9') || key=='.' || key=='-'))return;
    if(key=='.' && !decimals_)return;
    if(key=='-' && minimum_>=0)return;
    if(replace_) {text_.fill('\0');replace_=false;}
    const size_t n=std::strlen(text_.data());if(n+1>=text_.size())return;
    if(key=='-' && n)return;
    const char* dot=std::strchr(text_.data(),'.');
    if(key=='.' && dot)return;
    if(dot && key!='-' && n-static_cast<size_t>(dot-text_.data())-1>=decimals_)return;
    text_[n]=key;text_[n+1]='\0';
}
void NumericEntryModel::erase() {
    replace_=false;const size_t n=std::strlen(text_.data());if(n)text_[n-1]='\0';
}
float NumericEntryModel::value() const {return std::strtof(text_.data(),nullptr);}
bool NumericEntryModel::valid() const {
    char* end=nullptr;const float v=std::strtof(text_.data(),&end);
    return end!=text_.data() && *end=='\0' && std::isfinite(v) && v>=minimum_ && v<=maximum_;
}
