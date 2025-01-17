# A Compiler Writing Journey

Welcome to the repository documenting my journey to create a self-compiling compiler for a subset of the C language. This project is intended to be a practical guide, allowing others to follow along with detailed explanations of what I did and why, while referencing relevant compiler theory as needed—without diving too deep into the theoretical aspects.

---

## Progress Overview

Below is the list of steps completed so far. Each step includes a detailed explanation in its respective directory:

1. [Part 0](00_Introduction/Readme.md): Introduction to the Journey  
2. [Part 1](01_Scanner/Readme.md): Introduction to Lexical Scanning  
3. [Part 2](02_Parser/Readme.md): Introduction to Parsing  
4. [Part 3](03_Precedence/Readme.md): Operator Precedence  
5. [Part 4](04_Assembly/Readme.md): An Actual Compiler  
6. [Part 5](05_Statements/Readme.md): Statements  
7. [Part 6](06_Variables/Readme.md): Variables  
8. [Part 7](07_Comparisons/Readme.md): Comparison Operators  
9. [Part 8](08_If_Statements/Readme.md): If Statements  
10. [Part 9](09_While_Loops/Readme.md): While Loops  
11. [Part 10](10_For_Loops/Readme.md): For Loops  
12. [Part 11](11_Functions_pt1/Readme.md): Functions, part 1  
13. [Part 12](12_Types_pt1/Readme.md): Types, part 1  
14. [Part 13](13_Functions_pt2/Readme.md): Functions, part 2  
15. [Part 14](14_ARM_Platform/Readme.md): Generating ARM Assembly Code  
16. [Part 15](15_Pointers_pt1/Readme.md): Pointers, part 1  
17. [Part 16](16_Global_Vars/Readme.md): Declaring Global Variables Properly  
18. [Part 17](17_Scaling_Offsets/Readme.md): Better Type Checking and Pointer Offsets  
19. [Part 18](18_Lvalues_Revisited/Readme.md): Lvalues and Rvalues Revisited  
20. [Part 19](19_Arrays_pt1/Readme.md): Arrays, part 1  
21. [Part 20](20_Char_Str_Literals/Readme.md): Character and String Literals  
22. [Part 21](21_More_Operators/Readme.md): More Operators  
23. [Part 22](22_Design_Locals/Readme.md): Design Ideas for Local Variables and Function Calls  
24. [Part 23](23_Local_Variables/Readme.md): Local Variables  
25. [Part 24](24_Function_Params/Readme.md): Function Parameters  
26. [Part 25](25_Function_Arguments/Readme.md): Function Calls and Arguments  
27. [Part 26](26_Prototypes/Readme.md): Function Prototypes  
28. [Part 27](27_Testing_Errors/Readme.md): Regression Testing and a Nice Surprise  
29. [Part 28](28_Runtime_Flags/Readme.md): Adding More Run-time Flags  
30. [Part 29](29_Refactoring/Readme.md): A Bit of Refactoring  
31. [Part 30](30_Design_Composites/Readme.md): Designing Structs, Unions, and Enums  
32. [Part 31](31_Struct_Declarations/Readme.md): Implementing Structs, Part 1  
33. [Part 32](32_Struct_Access_pt1/Readme.md): Accessing Members in a Struct  
34. [Part 33](33_Unions/Readme.md): Implementing Unions and Member Access  
35. [Part 34](34_Enums_and_Typedefs/Readme.md): Enums and Typedefs  
36. [Part 35](35_Preprocessor/Readme.md): The C Pre-Processor  
37. [Part 36](36_Break_Continue/Readme.md): `break` and `continue`  
38. [Part 37](37_Switch/Readme.md): Switch Statements  
39. [Part 38](38_Dangling_Else/Readme.md): Dangling Else and More  
40. [Part 39](39_Var_Initialisation_pt1/Readme.md): Variable Initialization, part 1  
41. [Part 40](40_Var_Initialisation_pt2/Readme.md): Global Variable Initialization  
42. [Part 41](41_Local_Var_Init/Readme.md): Local Variable Initialization  
43. [Part 42](42_Casting/Readme.md): Type Casting and NULL  
44. [Part 43](43_More_Operators/Readme.md): Bugfixes and More Operators  
45. [Part 44](44_Fold_Optimisation/Readme.md): Constant Folding  
46. [Part 45](45_Globals_Again/Readme.md): Global Variable Declarations, revisited  
47. [Part 46](46_Void_Functions/Readme.md): Void Function Parameters and Scanning Changes  
48. [Part 47](47_Sizeof/Readme.md): A Subset of `sizeof`  
49. [Part 48](48_Static/Readme.md): A Subset of `static`  
50. [Part 49](49_Ternary/Readme.md): The Ternary Operator  
51. [Part 50](50_Mop_up_pt1/Readme.md): Mopping Up, part 1  
52. [Part 51](51_Arrays_pt2/Readme.md): Arrays, part 2  
53. [Part 52](52_Pointers_pt2/Readme.md): Pointers, part 2  
54. [Part 53](53_Mop_up_pt2/Readme.md): Mopping Up, part 2  
55. [Part 54](54_Reg_Spills/Readme.md): Spilling Registers  
56. [Part 55](55_Lazy_Evaluation/Readme.md): Lazy Evaluation  
57. [Part 56](56_Local_Arrays/Readme.md): Local Arrays  
58. [Part 57](57_Mop_up_pt3/Readme.md): Mopping Up, part 3  
59. [Part 58](58_Ptr_Increments/Readme.md): Fixing Pointer Increments/Decrements  
60. [Part 59](59_WDIW_pt1/Readme.md): Why Doesn't It Work, part 1  
61. [Part 60](60_TripleTest/Readme.md): Passing the Triple Test  
62. [Part 61](61_What_Next/Readme.md): What's Next?  
63. [Part 62](62_Cleanup/Readme.md): Code Cleanup  
64. [Part 63](63_QBE/Readme.md): A New Backend using QBE  
65. [Part 64](64_6809_Target/Readme.md): A Backend for the 6809 CPU  

---

## Copyright

I have borrowed some code and many ideas from the [SubC](http://www.t3x.org/subc/) compiler by Nils M Holm. His code is in the public domain. I believe my code is significantly different and therefore applies a different license.

Unless otherwise noted:  
- All source code and scripts are © Warren Toomey under the GPL3 license.  
- All non-source code documents (e.g., English documents, image files) are © Warren Toomey under the Creative Commons BY-NC-SA 4.0 license.
