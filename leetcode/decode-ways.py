class Solution(object):
    def numDecodings(self, s):
        """
        :type s: str
        :rtype: int
        """
        pass 

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