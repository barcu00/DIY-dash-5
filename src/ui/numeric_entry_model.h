#pragma once
#include <array>
#include <cstdint>
class NumericEntryModel {
public:
    void open(float value,float minimum,float maximum,uint8_t decimals);
    void append(char key);
    void erase();
    const char* text() const {return text_.data();}
    bool valid() const;
    float value() const;
private:
    std::array<char,24> text_{};
    float minimum_=0,maximum_=0;
    uint8_t decimals_=0;
    bool replace_=true;
};
