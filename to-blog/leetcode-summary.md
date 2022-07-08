
~~~ java
Arrays.copyOf(int[], length); // 返回一个新数组

//  java.util.Arrays
public static <T> T[] copyOf(@RecentlyNonNull T[] original, int newLength) {
        throw new RuntimeException("Stub!");
}
~~~
### 116. 填充每个节点的下一个右侧节点指针
https://leetcode-cn.com/problems/populating-next-right-pointers-in-each-node/

层序遍历解法：
~~~ java
class Solution {
    public Node connect(Node root) {
        if (root == null) {
            return root;
        }
        LinkedList<Node> list =new LinkedList();
        list.offer(root);

        while(!list.isEmpty()) {
            int size = list.size();
            for (int i=0;i<size;i++) {
                Node node = list.poll();// 取出元素
                if (i < size -1) {
                    node.next = list.peek(); // 返回队列首个元素，不出队列
                }
                if (null != node.left) {
                    list.offer(node.left);
                }
                if (null != node.right) {
                    list.offer(node.right);
                }
            }
    
        }
        return root;
    }
}
~~~

### 77. 组合
https://leetcode-cn.com/problems/combinations/

画出回溯树，
这就是典型的回溯算法，k 限制了树的高度，n 限制了树的宽度

根据回溯树，使用回溯算法：
~~~ java
class Solution {

    private List<List<Integer>> ans = new LinkedList();

    public List<List<Integer>> combine(int n, int k) { 
        if (k <=0 || n<=0) return ans;
        LinkedList<Integer> track = new LinkedList();
        backtrack(n, k, 1 ,track);
        return ans;
    }

    public void backtrack(int n, int k,int start, LinkedList<Integer>  track) {
        if (track.size() == k) { //模板方法，判断是否到达回溯树底部
            ans.add(new LinkedList(track));
            return;
        }
        for (int i=start; i<= n; i++) { // 层数为n，【1，2】和 【2，1】算作重复
            track.add(i);
            backtrack(n, k, i+1, track);
            track.removeLast();
        }
    }
}



~~~
题解在：https://mp.weixin.qq.com/s/qT6WgR6Qwn7ayZkI3AineA

其实 DFS 算法就是回溯算法

### 784. 字母大小写全排列
https://leetcode-cn.com/problems/letter-case-permutation/
#### BFS
相当于树的横向遍历

BFS 的核心思想应该不难理解的，就是把一些问题抽象成图，从一个点开始，向四周开始扩散。一般来说，我们写 BFS 算法都是用「队列」这种数据结构，每次将一个节点周围的所有节点加入队列。

~~~ java
class Solution {
    List<String> ans = new ArrayList<String>();

    public List<String> letterCasePermutation(String s) {
        StringBuilder path = new StringBuilder(s);
        dfs(s, 0, path);
        return ans;
    }

    public void dfs(String s, int pos, StringBuilder path) {
        if (pos == s.length()) {
            ans.add(path.toString());
            return;
        }

        if (Character.isDigit(s.charAt(pos))) {
            path.setCharAt(pos, s.charAt(pos));
            dfs(s, pos + 1, path);
        } else if (Character.isLowerCase(s.charAt(pos))) {
            path.setCharAt(pos, s.charAt(pos));
            dfs(s, pos + 1, path);
            path.setCharAt(pos, Character.toUpperCase(s.charAt(pos)));
            dfs(s, pos + 1, path);
        } else {
            path.setCharAt(pos, s.charAt(pos));
            dfs(s, pos + 1, path);
            path.setCharAt(pos, Character.toLowerCase(s.charAt(pos)));
            dfs(s, pos + 1, path);
        }
    }
}
~~~

题解： https://labuladong.gitee.io/algo/4/29/113/

### 473. 火柴拼正方形

回溯算法，类似DFS

~~~ java
class Solution {
    public List<Integer> nums;
    public int[] sums;
    public int possibleSquareSide;

    public Solution() {
        this.sums = new int[4];
    }

    // Depth First Search function.
    public boolean dfs(int index) {

        // If we have exhausted all our matchsticks, check if all sides of the square are of equal length
        if (index == this.nums.size()) {
            return sums[0] == sums[1] && sums[1] == sums[2] && sums[2] == sums[3];
        }

        // Get current matchstick.
        int element = this.nums.get(index);

        // Try adding it to each of the 4 sides (if possible)
        for(int i = 0; i < 4; i++) {
            if (this.sums[i] + element <= this.possibleSquareSide) {
                this.sums[i] += element;
                if (this.dfs(index + 1)) {
                    return true;
                }
                this.sums[i] -= element;
            }
        }

        return false;
    }

    public boolean makesquare(int[] nums) {
        // Empty matchsticks.
        if (nums == null || nums.length == 0) {
            return false;
        }

        // Find the perimeter of the square (if at all possible)
        int L = nums.length;
        int perimeter = 0;
        for(int i = 0; i < L; i++) {
            perimeter += nums[i];
        }

        this.possibleSquareSide =  perimeter / 4;
        if (this.possibleSquareSide * 4 != perimeter) {
            return false;
        }

        // Convert the array of primitive int to ArrayList (for sorting).
        this.nums = Arrays.stream(nums).boxed().collect(Collectors.toList());
        Collections.sort(this.nums, Collections.reverseOrder());
        return this.dfs(0);
    }
}

~~~


链接：https://leetcode-cn.com/problems/matchsticks-to-square/solution/huo-chai-pin-zheng-fang-xing-by-leetcode/