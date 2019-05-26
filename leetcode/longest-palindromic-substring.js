// @ts-check

/**
 * @param {string} s
 * @return {string}
 */
var longestPalindrome = function(s) {
    // if (!s) return s;

    const len = s.length;
    
    let minleft = 0;
    let maxright = 0;
    let maxLen = 0;
    for (let i = 0; i < len; i++) {
        setMax(i, i);
        if (s[i] === s[i+1]) setMax(i, i+1);
    }

    function setMax(left, right) {
        // console.log('111', {left, right});
        while (s[left-1] === s[right+1] && (left-1)>=0 && (right+1)<len) {
            left--;
            right++;
        }

        // console.log('222', {left, right});

        const curLen = right - left + 1;

        if (curLen > maxLen) {
            // console.log('333', {left, right, curLen, maxLen});
            maxLen = curLen;
            minleft = left;
            maxright = right;
        }
    }

    return s.substring(minleft, maxright + 1);
};

console.assert(longestPalindrome("babad") === "bab", 'test case 1: ');
console.assert(longestPalindrome("cbbd") === "bb", 'test case 2: ' + longestPalindrome("cbbd"));
console.assert(longestPalindrome("ccccccc") === "ccccccc", 'test case 3');
console.assert(longestPalindrome(" ") === " ", 'test case 4');
console.assert(longestPalindrome("") === "", 'test case 4');