// @ts-check

/**
 * 最长公共子序列
 */

 /**
  * @param {string} s1 
  * @param {string} s2 
  * @return {number}
  */
function LCS(s1, s2) {
    if (!s1) return 0;
    if (!s2) return 0;
    if (s1[0] === s2[0]) {
        return 1 + LCS(s1.slice(1), s2.slice(1));
    } else {
        return Math.max(LCS(s1, s2.slice(1)), LCS(s1.slice(1), s2))
    }
}

console.log(LCS('fc', 'fo')); // 3
console.log(LCS('ppt', 'programer')); // 1
console.log(LCS('zhihu', 'zxhxixhxux')); // 5

/** back of envelope calculation */
const _3century = 10 ** 10; // seconds
const _1day = 10 ** 5; // seconds
const _1quarter = 10 ** 3; // seconds
const _3hour = 10 ** 4; // seconds