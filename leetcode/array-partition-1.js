/**
 * https://leetcode.com/problems/array-partition-i/description/
 * n is a positive integer, which is in the range of [1, 10000].
 * All the integers in the array will be in the range of [-10000, 10000].
 */

var arrayPairSum = function(nums) {
    const bucket = (new Array(20001)).fill(0);
    const offset = 10000;

    nums.forEach(i => bucket[i + offset]++);

    // console.log(bucket);

    let ip = 1;
    let sum = 0;
    bucket.forEach((count, i) => {
        for (let j = 0; j < count; j++) {
            if(ip & 1 === 1) {
                // console.log('===> ip happended', ip);
                sum+=(i-offset);
            }
            ip++;
        }
    });

    // console.log(ip);

    return sum;
}

console.assert(arrayPairSum([1,4,3,2]) === 4);