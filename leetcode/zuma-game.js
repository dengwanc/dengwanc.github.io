// @ts-check

function setAutoIncrementMap(map, k) {
    const tmp = map.get(k);
    const v = tmp === undefined ? 1 : tmp+1;
    map.set(k, v);
    return v;
}

/**
 * @param {string} board 
 * @param {Map} handMap 
 */
function _find(board, handMap) {
    // todo
}

/**
 * remove continuous balls recursive
 * @param {string} board
 */
function _shrink(board) {
    // 
}

var findMinStep = function(board, hand) {
    const handMap = new Map();
    for (let h of hand) {
        setAutoIncrementMap(handMap, h);
    }

    return _find(board + '#', handMap);
};