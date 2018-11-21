// @ts-check

/**
 * @param {number[][]} grid
 * @return {number}
 */
var islandPerimeter = function(grid) {
    let neighbor_num = 0;
    let blocks = 0;
    let rows = grid.length;
    let cols = grid[0].length;

    // grid.forEach((row, i) => {
    //     row.forEach((v, j) => {
    //         if (v) {
    //             blocks++;

    //             if (j < cols-1 && grid[i][j+1]) {
    //                 // console.log(`===> (${i}, ${j})`);
    //                 neighbor_num++;
    //             }
    //             if (i < rows-1 && grid[i+1][j]) {
    //                 // console.log(`===> (${i}, ${j})`);
    //                 neighbor_num++;
    //             }
    //         }
            
    //     });
    // });
    // opmitize for loop 
    // opmitize forEach => for is poor way
    for (let row = 0; row < rows; row++) {
        for (let col = 0; col < cols; col++) {
            
            if (grid[row][col] === 1) { 
                blocks++;
                if (row < rows - 1 && grid[row + 1][col] === 1) { neighbor_num++; }
                if (col < cols - 1 && grid[row][col + 1] === 1) { neighbor_num++; }
            }
        }
    }
    
    
    // console.log(`===> blocks: ${blocks}, neighbor_num: ${neighbor_num}`);

    // opmitize call
    // return caculatePerimeter(blocks, neighbor_num);
    return 4*blocks - 2*neighbor_num;
};

/**
 * @param {number} blocks 
 * @param {number} neighbor_num 
 */
function caculatePerimeter(blocks, neighbor_num) {
    // type check block
    // type check neighbor_num
    
    return 4*blocks - 2*neighbor_num;
}