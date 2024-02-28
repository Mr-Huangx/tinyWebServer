#include"./lst_timer.h"

/*销毁链表时，删除其中所有的定时器*/
sort_timer_lst::~sort_timer_lst(){
    util_timer* temp = head;
    while(temp){
        head = temp->next;
        delete temp;
        temp = head;
    }
}

//将目标定时器timer添加到链表中
void sort_timer_lst::add_timer(util_timer* timer){
    if( !timer ) return;

    if( !head ){
        //头结点为空的话
        head = tail = timer;
        return;
    }

    if(timer->expire < head->expire){
        timer->next = head;
        head->prev = timer;
        head = timer;
        return;
    }

    add_timer(timer, head);
}

//当某个定时器任务发生变化时，对应调整其在链表中的位置
//这里只考虑，如果当前timer的任务被触发，则延长时间，即向链尾移动
void sort_timer_lst::adjust_timer(util_timer* timer){
    if(!timer) return;

    util_timer* tmp = timer->next;

    if(!tmp || timer->expire < tmp->expire){
        return;
    }

    //如果当前元素是链表的头结点，则将定时器从链表中取出，并重新插入链表
    if(head == timer){
        head = head->next;
        head->prev = nullptr;
        timer->next = nullptr;
        add_timer(timer, head);
    }
    //如果目标定时器不是链表的头结点，则将该定时器从链表中取出，然后插入其原来所在位置后面的部分链表中
    else{
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        add_timer(timer, timer->next);
    }
}

//将目标节点从链表中删除
void sort_timer_lst:: del_timer(util_timer* timer){
    if(!timer) return;

    if(timer == head && timer == tail){
        delete timer;
        head = nullptr;
        tail = nullptr;
        return;
    }

    //如果链表中至少有两个定时器，且目标定时器是链表的头结点，则将链表的头结点置为原头结点的下一个节点，然后删除目标定时器
    if(timer == head){
        head = head->next;
        head->prev = nullptr;
        delete timer;
        return;
    }

    //如果链表中至少有两个定时器，且目标节点是尾节点，则将链表的尾节点重置为源节点的前一个节点，然后删除目标定时器
    if(timer == tail){
        tail = tail->prev;
        tail->next=nullptr;
        delete timer;
        return;
    }

    timer->prev->next = timer->next;
    timer->next->prev = timer->prev;
    delete timer;
}

//SIGALRM信号每次触发就在其信号处理函数中调用一次tick函数，来处理链表上到期的任务
void sort_timer_lst::tick(){
    if(!head){
        //链表为空
        return;
    }

    time_t cur = time(NULL);//获取系统当前时间
    util_timer* temp = head;
    //从头结点开始依次处理每个定时器，直到遇到第一个未到期的定时器
    while(temp){
        if(cur < temp->expire){
            break;
        }
        //调用定时器的回调函数，以执行定时任务
        temp->cb_func(temp->user_data);

        //删除定时器
        head = temp->next;
        if(head) head->prev = nullptr;
        delete temp;

        //继续判断下一个定时器
        temp = head;
    }
}

//重载函数，被公有add_timer和adjust_timer函数调用，该函数表示把目标定时器timer添加到节点lst_head之后的部分链表中
void sort_timer_lst::add_timer(util_timer* timer, util_timer* lst_head)
{
    util_timer* prev = lst_head;
    util_timer* temp = prev->next;

    //遍历lst_head节点之后的链表，直到找到一个超时时间大于timer的超时时间的节点，并将目标节点插入到该节点之前
    while(temp)
    {
        if(timer->expire < temp->expire){
            //找到，则插入到temp前
            prev->next = timer;
            timer->next = temp;
            temp->prev = timer;
            timer->prev = prev;
            break;
        }
        //没找到，则继续往下找
        prev = temp;
        temp = temp->next;
    }

    //如果找到结尾还是没有找到，则插入到结尾
    if(!temp){
        prev->next = timer;
        timer->prev = prev;
        timer->next = nullptr;
        tail = timer;
    }
}