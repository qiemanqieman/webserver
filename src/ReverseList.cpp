#include "pch.h"

struct ListNode{
  ListNode(int d, ListNode *n = nullptr):data(d),next(n){}
  int data;
  ListNode* next;
};

struct List{
  ListNode* head;
  List(initializer_list<int> l);
  ~List();
  void print();
  friend inline ostream& operator<<(ostream& os, List& l){l.print(); return os;}
  void reverseEveryK(int k);
};

void List::print(){
  ListNode* cur = head;
  while(cur){
    cout << cur->data << " ";
    cur = cur->next;
  }
  cout << endl;
}

List::List(initializer_list<int> l){
  auto it = l.begin();
  head = new ListNode(*it++);
  ListNode* cur = head;
  while(it != l.end()){
    cur->next = new ListNode(*it++);
    cur = cur->next;
  }
}

List::~List(){
  ListNode* cur = head;
  while(cur){
    ListNode* next = cur->next;
    delete cur;
    cur = next;
  }
}

void List::reverseEveryK(int k)
{
  if (not head or k < 2 or not head->next) return;
  ListNode* preTail = nullptr, *curTail = head, *curHead;
  while(curTail){
    int count = 0;
    curHead = curTail;
    while(curHead->next and ++count < k){
      curHead = curHead->next;
    }
    auto pre = curTail, cur = curTail->next, nextTail = curHead->next;
    while(cur != nextTail){
      auto next = cur->next;
      cur->next = pre;
      pre = cur;
      cur = next;
    }
    if (preTail) preTail->next = curHead;
    else head = curHead;
    preTail = curTail;
    curTail = nextTail;
  }
  if (preTail) preTail->next = nullptr;
  return;
}