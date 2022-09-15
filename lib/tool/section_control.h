#ifndef __SECTION_CONTROL_H__
#define __SECTION_CONTROL_H__

#include<iostream>
#include<vector>


class secCont{
public:
    void addSection(std::pair<int, int> sec);
    int getfirstblank();
    int moveFirstSection();
    bool empty() const{return fillSection.size() == 0;}
    void showSec();

private:
    std::vector<std::pair<int, int>> fillSection;
};

#endif