#include"section_control.h"

void secCont::addSection(std::pair<int, int> sec){
    if(fillSection.size() == 0){
        fillSection.push_back(sec);
        return;
    }

    int front = -1;
    for(int i = 0; i < fillSection.size(); i++){
        if(fillSection[i].first > sec.first || fillSection[i].second > sec.first){
            front = i;
            break;
        }
    }
    int back = -1;
    for(int i = 0; i < fillSection.size(); i++){
        if(fillSection[i].first > sec.second || fillSection[i].second > sec.second){
            back = i;
            break;
        }
    }

    if(front == -1){
        fillSection.insert(fillSection.end(), sec);
        return;
    }

    if(sec.first < fillSection[front].first){
        int delCount;
        if(back == -1)
            delCount = fillSection.size() - front;
        else
            delCount = back - front;

        auto iter = fillSection.begin();
        for(int i = front; i < delCount; i++)
            fillSection.erase(iter+i);

        if(back == -1){
            if(front > 0 && sec.first == fillSection[front-1].second){
                fillSection[front-1].second = sec.second;
                return;
            }
            fillSection.insert(iter+front, sec);
        }
        else if(sec.second < fillSection[back].first){
            if(front > 0 && sec.first == fillSection[front-1].second){
                fillSection[front-1].second = sec.second;
                return;
            }
            fillSection.insert(iter+front, sec);
        }
        else{
            if(front > 0 && sec.first == fillSection[front-1].second){
                fillSection[front-1].second = fillSection[front].second;
                fillSection.erase(iter+front);
                return;
            }
            fillSection[front].first = sec.first;
        }
    }
    else{
        auto iter = fillSection.begin();
        if(back == -1){
            for(int i = front+1; i < fillSection.size(); i++)
                fillSection.erase(iter+i);
            fillSection[front].second = sec.second;
        }
        else if(sec.second < fillSection[back].first){
            for(int i = front+1; i < back; i++)
                fillSection.erase(iter+i);
            fillSection[front].second = sec.second;
        }
        else{
            int tail = fillSection[back].second;
            for(int i = front+1; i <= back; i++)
                fillSection.erase(iter+i);
            fillSection[front].second = tail;
        }
    }

}

int secCont::moveFirstSection(){
    if(!fillSection.empty()){
        int rv = fillSection[0].second;
        fillSection.erase(fillSection.begin());
        return rv;
    }
}

void secCont::showSec(){
    for(int i = 0; i < fillSection.size(); i++){
        std::cout << fillSection[i].first << "--" << fillSection[i].second << "   ";
    }
    std::cout << std::endl;
}