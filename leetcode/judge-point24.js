var step = 3;
var logs = [];
/**
 * @param {number[]} nums
 * @return {boolean}
 */
var judgePoint24 = function(nums) {
    console.log('24 points for', nums);
    const ret = _judge(nums);
    console.log(logs.join('\n'));
    return ret;
};

function _judge(nums) {
    if (!nums) return false;
    
    const len = nums.length;

    if (len == 1) return is24(nums[0]);

    for (let i = 0; i < len; i++) {
        for (let j = 0; j < len; j++) {
            if (i == j) continue;

            const B = nums.filter((n, k) =>  i != k && j != k);
            for (let op of ['+', '-', '*', '/']) {
                if (nums[j] === 0 && op == '/') {
                    continue;
                } else {
                    const a = nums[i];
                    const b = nums[j];
                    const opr = operate(a, b, op);
                    B.push(opr);
                    if (_judge(B)) {
                        logs.unshift(`step${step--}: ${a} ${op} ${b} = ${opr}`);
                        return true;
                    }
                    B.pop();
                }
            }
        }
    }

    return false;
}

function is24(x) {
    return x === 24 ? true : Math.abs(x - 24) < 1e-6;
}

function operate(a, b, op) {
    switch (op) {
        case '+': return (a + b);
        case '*': return (a * b);
        case '-': return (a - b);
        case '/': return (a / b);
    }
}

judgePoint24([3, 3, 8, 8]);