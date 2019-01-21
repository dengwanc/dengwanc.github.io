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

int isPrime(int n) {
    if (n == 1) return 0;
    if (n == 2) return 1;
    if (n % 2 == 0) return 0;
    
    int end = sqrt(n);
    
    for (int i = 3; i <= end; i+=2) {
        if (n % i == 0) return 0;
    }
    
    return 1;
}

int countPrimeSetBits(int L, int R) {
    int ret = 0;
    for (int i = L; i <= R; i++) {
        if (isPrime(count1(i))) ret++;
    }
    return ret;
}