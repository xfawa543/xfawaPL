; ModuleID = 'xfawac_llvm_ir'
declare i32 @puts(i8*)
declare i32 @SetConsoleOutputCP(i32)
@.null = private unnamed_addr constant [1 x i8] zeroinitializer

@.str0 = private unnamed_addr constant [19 x i8] c"你好，世界！\00", align 1
@.str1 = private unnamed_addr constant [7 x i8] c"114514\00", align 1
@.str2 = private unnamed_addr constant [11 x i8] c"HelloWorld\00", align 1

define i32 @main() {
entry:
  call i32 @SetConsoleOutputCP(i32 65001)
  %ptr0 = getelementptr inbounds [19 x i8], [19 x i8]* @.str0, i32 0, i32 0
  call i32 @puts(i8* %ptr0)
  %ptr1 = getelementptr inbounds [7 x i8], [7 x i8]* @.str1, i32 0, i32 0
  call i32 @puts(i8* %ptr1)
  %ptr2 = getelementptr inbounds [11 x i8], [11 x i8]* @.str2, i32 0, i32 0
  call i32 @puts(i8* %ptr2)
  ret i32 0
}
