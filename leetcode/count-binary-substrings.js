// Give a string s, count the number of non-empty (contiguous) substrings that have the same number of 0's and 1's, and all the 0's and all the 1's in these substrings are grouped consecutively.

// Substrings that occur multiple times are counted the number of times they occur.

// Example 1:
// Input: "00110011" 
// Output: 6
// Explanation: There are 6 substrings that have equal number of consecutive 1's and 0's: "0011", "01", "1100", "10", "0011", and "01".
// Notice that some of these substrings repeat and are counted the number of times they occur.
// Also, "00110011" is not a valid substring because all the 0's (and 1's) are not grouped together.

// Example 2:
// Input: "10101"
// Output: 4
// Explanation: There are 4 substrings: "10", "01", "10", "01" that have equal number of consecutive 1's and 0's.

const assert = require('assert');
/**
 * @param {string} s
 * @return {number}
 */
var countBinarySubstrings = function(s) {
    const len = s.length;
    let ret = 0;
    for (let i = 1; i < len;) {
        const next_char = s[i];
        const pre_char = s[i-1];
        let pre = i-1;
        let next = i;
        if (next_char !== pre_char) {    
            while(1) {
                if (s[pre] === pre_char && s[next] === next_char) {
                    ret++;
                    next++;
                    pre--;
                } else {
                    break;
                }
            }
            i = next;
        } else {
            i++;
        }
        // console.log(i);
    }
    return ret;
};

assert(countBinarySubstrings('10') === 1);
assert(countBinarySubstrings('1') === 0);
assert(countBinarySubstrings('101010') === 5);
assert(countBinarySubstrings('0011') === 2);
assert(countBinarySubstrings('000111') === 3);
assert(countBinarySubstrings('111000111') === 6);
assert(countBinarySubstrings('00110') === 3);
assert(countBinarySubstrings('00110011') === 6);
assert(countBinarySubstrings('10101') === 4);
assert(countBinarySubstrings('11100') === 2);