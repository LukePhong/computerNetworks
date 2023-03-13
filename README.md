# computerNetworks
My homework for computer networks lesson in Nankai University.

## Lab1

Simple UDP chat console application.

## Lab2

HTML page packet capture experiment.

## Lab3-Lab5

RDT(Reliable data transmission) proxies based on UDP.

## Lab6

Performance analysis on RDT proxies.

# computerNetworks

这是我在南开大学撰写的本科生计算机网络课程编程作业。

每个章节的内容如上所述，具体实现请参考其中的实验报告。

# 如果你是NKU的学生请不要试图抄袭其中的内容，抄袭无法提升你的能力。

一个优秀程序员应该**具备独立实现整个TCP/IP协议栈的能力**，你应该在精力允许的情况下充分学习相关**理论**和优秀**框架**，完成自己的尝试。

这些实现和实验结果适合你在亲手完成实验后，进行分析对比之用。

这些代码撰写匆忙、质量一般，但如果你认为有任何值得探讨的地方，请留下issue。

# 关于具体设计

在lab1中尝试实现MVC设计框架（但是实际非常拉跨），中间你会发现我使用了自己组装的线程间消息队列

在RDT实现中，我将文件进行分片后再进行包装再传给下层，以达到文件处理层（应用层）和传输层之间更好的解耦

我唯一比较满意的是Lab5中的gbnSender和gbnRenoSender。自认为函数划分的比较好，整个代码看起来很干净，没有把所有东西都堆积在一个函数中；甚至还不需要写注释，因为函数名完全表达了它的功能，每个函数也就是 10 - 20 行这样。最后这个实验也是拿到了满分。但是因为时间有限，这种函数划分相对随意，在复用性上考虑还不是特别周到。
