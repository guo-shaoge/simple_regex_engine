1. 只支持 ( | ) * + ?

2. graph-easy graph_nfa.txt 可以生成nfa的ascii图像
	e.g	pattern: a|b
		output: 		

+--------+  SPLIT   +--------+  a   +--------+  EMPTY   +--------+
| state2 | -------> | state0 | ---> | state3 | -------> | accept |
+--------+          +--------+      +--------+          +--------+
  |                                   ^
  | SPLIT                             |
  v                                   |
+--------+  b                         |
| state1 | ---------------------------+
+--------+
