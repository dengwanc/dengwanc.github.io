#include <assert.h>

#define MAX(a, b) (a > b) ? (a) : (b)

/**
 * |________________|_|
 *                  |️
 *  max____by__local|
 *       max_by_tail|tail
 */
int maxSubArray(int* nums, int numsSize) {
    if (numsSize == 0) return 0;

    int maxby_local = nums[0];
    int maxby_tail = nums[0];

    for (int i = 1; i < numsSize; i++) {
        int n = nums[i];
        maxby_tail = MAX(maxby_tail + n, n);
        maxby_local = MAX(maxby_local, maxby_tail);
    }

    return maxby_local;
}

int main() {
    return 0;
}