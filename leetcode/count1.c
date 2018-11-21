#include <assert.h>

int count1(int n)
{
    int mask1 = 0x55555555;
    int mask2 = 0x33333333;
    int mask3 = 0x0f0f0f0f;
    int mask4 = 0x00ff00ff;
    int mask5 = 0x0000ffff;
    int tmp1 = (n & mask1) + ((n>>1) & mask1);
    int tmp2 = (tmp1 & mask2) + ((tmp1>>2) & mask2);
    int tmp3 = (tmp2 & mask3) + ((tmp2>>4) & mask3);
    int tmp4 = (tmp3 & mask4) + ((tmp3>>8) & mask4);
    int tmp5 = (tmp4 & mask5) + ((tmp4>>16) & mask5);

    return tmp5;
}

int main()
{
    assert(count1(5) == 2);
    assert(count1(16) == 1);
    assert(count1(31) == 5);
    assert(count1(26) == 3);
}