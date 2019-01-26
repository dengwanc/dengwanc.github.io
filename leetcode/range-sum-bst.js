/**
 * Definition for a binary tree node.
 * function TreeNode(val) {
 *     this.val = val;
 *     this.left = this.right = null;
 * }
 */
/**
 * @param {TreeNode} root
 * @param {number} L
 * @param {number} R
 * @return {number}
 */
var rangeSumBST = function(root, L, R) {
    if (root === null) return 0;
    const curval = root.val;
    
    if (curval >= R) return (curval == R ? curval : 0) + rangeSumBST(root.left, L, R);
    if (curval <= L) return (curval == L ? curval : 0) + rangeSumBST(root.right, L, R);

    return curval +
        rangeSumBST(root.left, L, curval-1) +
        rangeSumBST(root.right, curval+1, R);
};