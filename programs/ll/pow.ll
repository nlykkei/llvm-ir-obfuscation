; ModuleID = 'programs/c/pow.c'
source_filename = "programs/c/pow.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [15 x i8] c"Compute power\0A\00", align 1
@.str.1 = private unnamed_addr constant [25 x i8] c"Usage: pow <base> <exp>\0A\00", align 1
@.str.2 = private unnamed_addr constant [34 x i8] c"Arguments required: <base> <exp>\0A\00", align 1
@.str.3 = private unnamed_addr constant [53 x i8] c"Invalid argument: <base> must be a positive integer\0A\00", align 1
@.str.4 = private unnamed_addr constant [55 x i8] c"Invalid argument: <exp> must be a nonnegative integer\0A\00", align 1

; Function Attrs: nounwind uwtable
define void @help() #0 {
entry:
  %call = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([15 x i8], [15 x i8]* @.str, i32 0, i32 0))
  %call1 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([25 x i8], [25 x i8]* @.str.1, i32 0, i32 0))
  ret void
}

declare i32 @printf(i8*, ...) #1

; Function Attrs: nounwind uwtable
define i32 @main(i32 %argc, i8** %argv) #0 {
entry:
  %retval = alloca i32, align 4
  %argc.addr = alloca i32, align 4
  %argv.addr = alloca i8**, align 8
  %base = alloca i32, align 4
  %exp = alloca i32, align 4
  %result = alloca i32, align 4
  store i32 0, i32* %retval, align 4
  store i32 %argc, i32* %argc.addr, align 4
  store i8** %argv, i8*** %argv.addr, align 8
  %0 = load i32, i32* %argc.addr, align 4
  %cmp = icmp slt i32 %0, 3
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  %call = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([34 x i8], [34 x i8]* @.str.2, i32 0, i32 0))
  store i32 -1, i32* %retval, align 4
  br label %return

if.end:                                           ; preds = %entry
  %1 = load i8**, i8*** %argv.addr, align 8
  %arrayidx = getelementptr inbounds i8*, i8** %1, i64 1
  %2 = load i8*, i8** %arrayidx, align 8
  %call1 = call i64 @strtol(i8* %2, i8** null, i32 0) #3
  %conv = trunc i64 %call1 to i32
  store i32 %conv, i32* %base, align 4
  %cmp2 = icmp sle i32 %conv, 0
  br i1 %cmp2, label %if.then4, label %if.end6

if.then4:                                         ; preds = %if.end
  %call5 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([53 x i8], [53 x i8]* @.str.3, i32 0, i32 0))
  store i32 -1, i32* %retval, align 4
  br label %return

if.end6:                                          ; preds = %if.end
  %3 = load i8**, i8*** %argv.addr, align 8
  %arrayidx7 = getelementptr inbounds i8*, i8** %3, i64 2
  %4 = load i8*, i8** %arrayidx7, align 8
  %call8 = call i64 @strtol(i8* %4, i8** null, i32 0) #3
  %conv9 = trunc i64 %call8 to i32
  store i32 %conv9, i32* %exp, align 4
  %cmp10 = icmp slt i32 %conv9, 0
  br i1 %cmp10, label %if.then12, label %if.end14

if.then12:                                        ; preds = %if.end6
  %call13 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([55 x i8], [55 x i8]* @.str.4, i32 0, i32 0))
  store i32 -1, i32* %retval, align 4
  br label %return

if.end14:                                         ; preds = %if.end6
  %5 = load i32, i32* %base, align 4
  %6 = load i32, i32* %exp, align 4
  %call15 = call i32 @power(i32 %5, i32 %6)
  store i32 %call15, i32* %result, align 4
  %7 = load i32, i32* %result, align 4
  store i32 %7, i32* %retval, align 4
  br label %return

return:                                           ; preds = %if.end14, %if.then12, %if.then4, %if.then
  %8 = load i32, i32* %retval, align 4
  ret i32 %8
}

; Function Attrs: nounwind
declare i64 @strtol(i8*, i8**, i32) #2

; Function Attrs: nounwind uwtable
define i32 @power(i32 %base, i32 %exp) #0 {
entry:
  %retval = alloca i32, align 4
  %base.addr = alloca i32, align 4
  %exp.addr = alloca i32, align 4
  %result = alloca i32, align 4
  store i32 %base, i32* %base.addr, align 4
  store i32 %exp, i32* %exp.addr, align 4
  %0 = load i32, i32* %exp.addr, align 4
  %cmp = icmp eq i32 %0, 0
  br i1 %cmp, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  store i32 1, i32* %retval, align 4
  br label %return

if.else:                                          ; preds = %entry
  %1 = load i32, i32* %exp.addr, align 4
  %rem = srem i32 %1, 2
  %cmp1 = icmp eq i32 %rem, 0
  br i1 %cmp1, label %if.then2, label %if.else3

if.then2:                                         ; preds = %if.else
  %2 = load i32, i32* %base.addr, align 4
  %3 = load i32, i32* %exp.addr, align 4
  %div = sdiv i32 %3, 2
  %call = call i32 @power(i32 %2, i32 %div)
  store i32 %call, i32* %result, align 4
  %4 = load i32, i32* %result, align 4
  %5 = load i32, i32* %result, align 4
  %mul = mul nsw i32 %4, %5
  store i32 %mul, i32* %retval, align 4
  br label %return

if.else3:                                         ; preds = %if.else
  %6 = load i32, i32* %base.addr, align 4
  %7 = load i32, i32* %base.addr, align 4
  %8 = load i32, i32* %exp.addr, align 4
  %sub = sub nsw i32 %8, 1
  %call4 = call i32 @power(i32 %7, i32 %sub)
  %mul5 = mul nsw i32 %6, %call4
  store i32 %mul5, i32* %retval, align 4
  br label %return

return:                                           ; preds = %if.else3, %if.then2, %if.then
  %9 = load i32, i32* %retval, align 4
  ret i32 %9
}

attributes #0 = { nounwind uwtable "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind }

!llvm.ident = !{!0}

!0 = !{!"clang version 3.9.1 (tags/RELEASE_391/final)"}
