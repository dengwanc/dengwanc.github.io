
const _drive_table = {
    'L': (cur) => _verctor_plus(cur, [-1, 0]),
    'R': (cur) => _verctor_plus(cur, [1, 0]),
    'U': (cur) => _verctor_plus(cur, [0, 1]),
    'D': (cur) => _verctor_plus(cur, [0, -1]),
};

var judgeCircle = function(moves) {
    let cur = [0, 0];
    for (let m of moves) {
        cur = _drive_table[m](cur);
    }

    return _is_origin(cur);
}

function _is_origin(v) {
    return v.every(e => e == 0);
}

function _verctor_plus(v1, v2) {
    return v1.map((e, i) => {
        return e + v2[i];
    });
}