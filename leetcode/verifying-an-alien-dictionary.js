/**
 * @param {string[]} words
 * @param {string} order
 * @return {boolean}
 */
var isAlienSorted = function(words, order) {
    const orderMap = {};
    for (let i = 0; i < order.length; i++) {
        orderMap[order[i]] = i;
    }
    
    for (let i = 0; i < words.length-1; i++) {
        const r = cmp(words[i], words[i+1], orderMap);
        if (r === 1) {
            return false;
        }
    }
    
    return true;
};

function cmp(w1, w2, orderMap) {
    const len1 = w1.length;
    const len2 = w2.length;
    
    for (let i = 0; i < Math.min(len1, len2); i++) {
        const c1 = w1[i];
        const c2 = w2[i];
        if (orderMap[c1] > orderMap[c2]) {
            return 1;
        }
        
        if (orderMap[c1] < orderMap[c2]) {
            return -1;
        }
    }
    
    if (len1 > len2) {
        return 1;
    }
    
    if (len1 < len2) {
        return -1;
    }
    
    return 0;
}

console.assert(isAlienSorted(["hello","leetcode"], "hlabcdefgijkmnopqrstuvwxyz") === true);
console.assert(isAlienSorted(["word","world","row"], "worldabcefghijkmnpqstuvxyz") === false);
console.assert(isAlienSorted(["apple","app"], "abcdefghijklmnopqrstuvwxyz") === false);