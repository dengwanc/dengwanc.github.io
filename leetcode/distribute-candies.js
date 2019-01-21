/**
 * @param {number[]} candies
 * @return {number}
 */
var distributeCandies = function(candies) {
    const sisterGot = new Set();
    let brotherGot = 0;

    for (let c of candies) {
        if (sisterGot.has(c)) {
            brotherGot++;
        } else {
            sisterGot.add(c);
        }
    }

    const sisterGotCount = sisterGot.size;
    if (sisterGotCount <= brotherGot) {
        return sisterGotCount;
    } else {
        return candies.length >> 1; // half
    }
};

const assert = require('assert');
assert(distributeCandies([1,1,2,3]), 2);
assert(distributeCandies([1,1,1,1]), 1);
assert(distributeCandies([1,2,3,4]), 2);