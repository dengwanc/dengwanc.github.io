const assert = require('assert');
/**
 * @param {number[]} nums
 * @return {number[]}
 */
var findDisappearedNumbers = function(nums) {
    const len = nums.length;
    for (let i = 0; i < len;) {
        const n = nums[i];
        const go = nums[i] - 1;
        const tmp = nums[go];
        // console.log(`===> n=${n}, tmp=${tmp}, go=${go}, i=${i}`);
        if (nums[i] === 0) {
            i++;
        } else if (tmp === n) {
            if (i !== go) nums[i] = 0;
            i++;
        } else {
            nums[go] = n;
            nums[i] = tmp;
        }
    }
    
    const ret = [];
    nums.forEach((n, i) => {
        if (n === 0) ret.push(i+1);
    });
    return ret;
};

assert.deepEqual(
    findDisappearedNumbers([1, 2, 3, 4, 5, 6]),
    [],
);

assert.deepEqual(
    findDisappearedNumbers([1, 2, 3, 2, 5, 6]),
    [4],
);

assert.deepEqual(
    findDisappearedNumbers([1, 2, 3, 2, 1, 6]),
    [4, 5],
);

assert.deepEqual(
    findDisappearedNumbers([6, 5, 4, 3, 2, 2]),
    [1],
);

assert.deepEqual(
    findDisappearedNumbers([4,3,2,7,8,2,3,1]),
    [5, 6],
);
