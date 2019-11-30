class Solution(object):
    def numDecodings(self, s):
        """
        :type s: str
        :rtype: int
        """
        if not s:
            return 0
        
        n = len(s)
        dp = [0]*(n + 1)
        dp[0] = 1
        dp[1] = 0 if s[0] == '0' else 1

        for i in range(2, n+1):
            first = int(s[i-1:i])
            second = int(s[i-2:i])
            if 1 <= first <= 9:
                dp[i] += dp[i-1]
            if 10 <= second <= 26:
                dp[i] += dp[i-2]
        
        return dp[n]

sol = Solution()
assert sol.numDecodings('12')==2
assert sol.numDecodings('226')==3
assert sol.numDecodings('1122312')==16
assert sol.numDecodings('10')==1
assert sol.numDecodings('0')==0
assert sol.numDecodings('00')==0
assert sol.numDecodings('230')==0
assert sol.numDecodings('27')==1
assert sol.numDecodings('301')==0
assert sol.numDecodings('226')==3

# class Solution(object):
#     def numDecodings(self, s):
#         """
#         :type s: str
#         :rtype: int
#         """
#         self.onenumSet = set([10, 20])
#         return self.ways(s, {})
    
#     def ways(self, s, solutionMap):
#         if not s:
#             return 0

#         if s in solutionMap:
#             return solutionMap[s]
        
#         # special case for 01, 00, 0, 0931
#         if s[0] == '0':
#             return 0
        
#         if len(s) == 1:
#             return 1

#         test = int(s[0:2])
#         rest1 = s[1:]
#         rest2 = s[2:]

#         if test > 26:
#             solutionMap[rest1] = self.ways(rest1, solutionMap)
#             return solutionMap[rest1]
#         else:
#             if len(s) == 2:
#                 solutionMap[s] = 1 if test in self.onenumSet else 2
#                 return solutionMap[s]
                
#             solutionMap[rest2] = self.ways(rest2, solutionMap)
#             solutionMap[rest1] = self.ways(rest1, solutionMap)
#             return solutionMap[rest1] + solutionMap[rest2]