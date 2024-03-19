#include "ReverseList.cpp"

#include <gtest/gtest.h>

TEST(ReverseListTest, ReverseListTest)
{
  List* head = new List({1,2,3,4,5,6,7,8,9,10,11,12,13});
  head->reverseEveryK(3);
  stringstream ss;
  ss << *head;
  EXPECT_EQ(ss.str() == string("3 2 1 6 5 4 9 8 7 12 11 10 13 "), 0);
  delete head;
}
