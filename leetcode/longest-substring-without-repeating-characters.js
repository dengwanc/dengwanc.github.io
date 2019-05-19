// @ts-check

/**
 * @param {string} s
 * @return {number}
 * More Clear Solution
 * use left pointer and right pointer dynamic move
 */
var lengthOfLongestSubstring = function(s) {
    if (!s) return 0;

    const len = s.length;
    const indicatingMap = new Map();
    let maxSubLen = 0;
    let i = 0;
    while (i < len) {
        const char = s[i];
        const index = i;
        if (indicatingMap.has(char)) {
            const curSubLen = indicatingMap.size;
            const backIndex = indicatingMap.get(char); 
            maxSubLen = curSubLen > maxSubLen ? curSubLen : maxSubLen;
            indicatingMap.clear();
            i = backIndex;
        } else {
            indicatingMap.set(char, index);
        }
            
        i++;
    }

    const curSubLen = indicatingMap.size; 
    maxSubLen = curSubLen > maxSubLen ? curSubLen : maxSubLen;

    return maxSubLen;
};

console.assert(lengthOfLongestSubstring("abcabcbb") === 3, 'test case 1');
console.assert(lengthOfLongestSubstring("bbbbb") === 1, 'test case 2');
console.assert(lengthOfLongestSubstring("pwwkew") === 3, 'test case 3');
console.assert(lengthOfLongestSubstring(" ") === 1, 'test case 4');
console.assert(lengthOfLongestSubstring("dvdf") === 3, 'test case 5');
