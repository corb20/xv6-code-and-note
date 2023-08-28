# lab7 Networking

## 1.Lab: networking

**### 1.1实验要求（英文）：**  
In this lab you will write an xv6 device driver for a network interface card (NIC).

Background
Before writing code, you may find it helpful to review "Chapter 5: Interrupts and device drivers" in the xv6 book.

You'll use a network device called the E1000 to handle network communication. To xv6 (and the driver you write), the E1000 looks like a real piece of hardware connected to a real Ethernet local area network (LAN). In fact, the E1000 your driver will talk to is an emulation provided by qemu, connected to a LAN that is also emulated by qemu. On this emulated LAN, xv6 (the "guest") has an IP address of 10.0.2.15. Qemu also arranges for the computer running qemu to appear on the LAN with IP address 10.0.2.2. When xv6 uses the E1000 to send a packet to 10.0.2.2, qemu delivers the packet to the appropriate application on the (real) computer on which you're running qemu (the "host").

You will use QEMU's "user-mode network stack". QEMU's documentation has more about the user-mode stack here. We've updated the Makefile to enable QEMU's user-mode network stack and the E1000 network card.

The Makefile configures QEMU to record all incoming and outgoing packets to the file packets.pcap in your lab directory. It may be helpful to review these recordings to confirm that xv6 is transmitting and receiving the packets you expect. To display the recorded packets:

tcpdump -XXnr packets.pcap
We've added some files to the xv6 repository for this lab. The file kernel/e1000.c contains initialization code for the E1000 as well as empty functions for transmitting and receiving packets, which you'll fill in. kernel/e1000_dev.h contains definitions for registers and flag bits defined by the E1000 and described in the Intel E1000 Software Developer's Manual. kernel/net.c and kernel/net.h contain a simple network stack that implements the IP, UDP, and ARP protocols. These files also contain code for a flexible data structure to hold packets, called an mbuf. Finally, kernel/pci.c contains code that searches for an E1000 card on the PCI bus when xv6 boots.

Your Job (hard)
Your job is to complete e1000_transmit() and e1000_recv(), both in kernel/e1000.c, so that the driver can transmit and receive packets. You are done when make grade says your solution passes all the tests.

While writing your code, you'll find yourself referring to the E1000 Software Developer's Manual. Of particular help may be the following sections:

+ Section 2 is essential and gives an overview of the entire device.
+ Section 3.2 gives an overview of packet receiving.
+ Section 3.3 gives an overview of packet transmission, alongside section 3.4.
+ Section 13 gives an overview of the registers used by the E1000.
+ Section 14 may help you understand the init code that we've provided.
Browse the E1000 Software Developer's Manual. This manual covers several closely related Ethernet controllers. QEMU emulates the 82540EM. Skim Chapter 2 now to get a feel for the device. To write your driver, you'll need to be familiar with Chapters 3 and 14, as well as 4.1 (though not 4.1's subsections). You'll also need to use Chapter 13 as a reference. The other chapters mostly cover components of the E1000 that your driver won't have to interact with. Don't worry about the details at first; just get a feel for how the document is structured so you can find things later. The E1000 has many advanced features, most of which you can ignore. Only a small set of basic features is needed to complete this lab.

The *e1000_init()* function we provide you in e1000.c configures the E1000 to read packets to be transmitted from RAM, and to write received packets to RAM. This technique is called DMA, for direct memory access, referring to the fact that the E1000 hardware directly writes and reads packets to/from RAM.

Because bursts of packets might arrive faster than the driver can process them, e1000_init() provides the E1000 with multiple buffers into which the E1000 can write packets. The E1000 requires these buffers to be described by an array of "descriptors" in RAM; each descriptor contains an address in RAM where the E1000 can write a received packet. struct rx_desc describes the descriptor format. The array of descriptors is called the receive ring, or receive queue. It's a circular ring in the sense that when the card or driver reaches the end of the array, it wraps back to the beginning. e1000_init() allocates mbuf packet buffers for the E1000 to DMA into, using mbufalloc(). There is also a transmit ring into which the driver places packets it wants the E1000 to send. e1000_init() configures the two rings to have size RX_RING_SIZE and TX_RING_SIZE.

When the network stack in net.c needs to send a packet, it calls e1000_transmit() with an mbuf that holds the packet to be sent. Your transmit code must place a pointer to the packet data in a descriptor in the TX (transmit) ring. struct tx_desc describes the descriptor format. You will need to ensure that each mbuf is eventually freed, but only after the E1000 has finished transmitting the packet (the E1000 sets the E1000_TXD_STAT_DD bit in the descriptor to indicate this).

When the E1000 receives each packet from the ethernet, it first DMAs the packet to the mbuf pointed to by the next RX (receive) ring descriptor, and then generates an interrupt. Your e1000_recv() code must scan the RX ring and deliver each new packet's mbuf to the network stack (in net.c) by calling net_rx(). You will then need to allocate a new mbuf and place it into the descriptor, so that when the E1000 reaches that point in the RX ring again it finds a fresh buffer into which to DMA a new packet.

In addition to reading and writing the descriptor rings in RAM, your driver will need to interact with the E1000 through its memory-mapped control registers, to detect when received packets are available and to inform the E1000 that the driver has filled in some TX descriptors with packets to send. The global variable regs holds a pointer to the E1000's first control register; your driver can get at the other registers by indexing regs as an array. You'll need to use indices E1000_RDT and E1000_TDT in particular.

To test your driver, run make server in one window, and in another window run make qemu and then run nettests in xv6. The first test in nettests tries to send a UDP packet to the host operating system, addressed to the program that make server runs. If you haven't completed the lab, the E1000 driver won't actually send the packet, and nothing much will happen.

After you've completed the lab, the E1000 driver will send the packet, qemu will deliver it to your host computer, make server will see it, it will send a response packet, and the E1000 driver and then nettests will see the response packet. Before the host sends the reply, however, it sends an "ARP" request packet to xv6 to find out its 48-bit Ethernet address, and expects xv6 to respond with an ARP reply. kernel/net.c will take care of this once you have finished your work on the E1000 driver. If all goes well, nettests will print testing ping: OK, and make server will print a message from xv6!.

tcpdump -XXnr packets.pcap should produce output that starts like this:

reading from file packets.pcap, link-type EN10MB (Ethernet)
```sh
15:27:40.861988 IP 10.0.2.15.2000 > 10.0.2.2.25603: UDP, length 19
        0x0000:  ffff ffff ffff 5254 0012 3456 0800 4500  ......RT..4V..E.
        0x0010:  002f 0000 0000 6411 3eae 0a00 020f 0a00  ./....d.>.......
        0x0020:  0202 07d0 6403 001b 0000 6120 6d65 7373  ....d.....a.mess
        0x0030:  6167 6520 6672 6f6d 2078 7636 21         age.from.xv6!
15:27:40.862370 ARP, Request who-has 10.0.2.15 tell 10.0.2.2, length 28
        0x0000:  ffff ffff ffff 5255 0a00 0202 0806 0001  ......RU........
        0x0010:  0800 0604 0001 5255 0a00 0202 0a00 0202  ......RU........
        0x0020:  0000 0000 0000 0a00 020f                 ..........
15:27:40.862844 ARP, Reply 10.0.2.15 is-at 52:54:00:12:34:56, length 28
        0x0000:  ffff ffff ffff 5254 0012 3456 0806 0001  ......RT..4V....
        0x0010:  0800 0604 0002 5254 0012 3456 0a00 020f  ......RT..4V....
        0x0020:  5255 0a00 0202 0a00 0202                 RU........
15:27:40.863036 IP 10.0.2.2.25603 > 10.0.2.15.2000: UDP, length 17
        0x0000:  5254 0012 3456 5255 0a00 0202 0800 4500  RT..4VRU......E.
        0x0010:  002d 0000 0000 4011 62b0 0a00 0202 0a00  .-....@.b.......
        0x0020:  020f 6403 07d0 0019 3406 7468 6973 2069  ..d.....4.this.i
        0x0030:  7320 7468 6520 686f 7374 21              s.the.host!
```
Your output will look somewhat different, but it should contain the strings "ARP, Request", "ARP, Reply", "UDP", "a.message.from.xv6" and "this.is.the.host".

nettests performs some other tests, culminating in a DNS request sent over the (real) Internet to one of Google's name server. You should ensure that your code passes all these tests, after which you should see this output:

```sh
$ nettests
nettests running on port 25603
testing ping: OK
testing single-process pings: OK
testing multi-process pings: OK
testing DNS
DNS arecord for pdos.csail.mit.edu. is 128.52.129.126
DNS OK
all tests passed.
You should ensure that make grade agrees that your solution passes.
```

Hints
Start by adding print statements to e1000_transmit() and e1000_recv(), and running make server and (in xv6) nettests. You should see from your print statements that nettests generates a call to e1000_transmit.
Some hints for implementing e1000_transmit:

+ First ask the E1000 for the TX ring index at which it's expecting the next packet, by reading the E1000_TDT control register.
+ Then check if the the ring is overflowing. If E1000_TXD_STAT_DD is not set in the descriptor indexed by E1000_TDT, the E1000 hasn't finished the corresponding previous transmission request, so return an error.
+ Otherwise, use mbuffree() to free the last mbuf that was transmitted from that descriptor (if there was one).
+ Then fill in the descriptor. m->head points to the packet's content in memory, and m->len is the packet length. Set the necessary cmd flags (look at Section 3.3 in the E1000 manual) and stash away a pointer to the mbuf for later freeing.
+ Finally, update the ring position by adding one to E1000_TDT modulo TX_RING_SIZE.
+ If e1000_transmit() added the mbuf successfully to the ring, return 0. On failure (e.g., there is no descriptor available to transmit the mbuf), return -1 so that the caller knows to free the mbuf.

Some hints for implementing e1000_recv:

+ First ask the E1000 for the ring index at which the next waiting received packet (if any) is located, by fetching the E1000_RDT control register and adding one modulo RX_RING_SIZE.
+ Then check if a new packet is available by checking for the E1000_RXD_STAT_DD bit in the status portion of the descriptor. If not, stop.
+ Otherwise, update the mbuf's m->len to the length reported in the descriptor. Deliver the mbuf to the network stack using net_rx().
+ Then allocate a new mbuf using mbufalloc() to replace the one just given to net_rx(). Program its data pointer (m->head) into the descriptor. Clear the descriptor's status bits to zero.
+ Finally, update the E1000_RDT register to be the index of the last ring descriptor processed.
+ e1000_init() initializes the RX ring with mbufs, and you'll want to look at how it does that and perhaps borrow code.
+ At some point the total number of packets that have ever arrived will exceed the ring size (16); make sure your code can handle that.
You'll need locks to cope with the possibility that xv6 might use the E1000 from more than one process, or might be using the E1000 in a kernel thread when an interrupt arrives.

**### 1.2实验要求（中文）：**  

在这个实验中，您将为网络接口卡（NIC）编写一个xv6设备驱动程序。

背景
在编写代码之前，您可能会发现回顾xv6书中的“第5章：中断和设备驱动程序”有所帮助。

您将使用一个名为E1000的网络设备来处理网络通信。对于xv6（和您编写的驱动程序），E1000看起来像连接到真实以太网局域网（LAN）的真实硬件。实际上，您将与之交谈的E1000是由qemu提供的仿真，连接到由qemu仿真的LAN。在这个仿真的LAN上，xv6（“guest”）具有IP地址10.0.2.15。 Qemu还安排运行qemu的计算机以IP地址10.0.2.2出现在LAN上。当xv6使用E1000向10.0.2.2发送数据包时，qemu将数据包传递给您在运行qemu的（真实）计算机上的适当应用程序（“host”）。

您将使用QEMU的“用户模式网络堆栈”。 QEMU的文档在此处有关于用户模式堆栈的更多信息。我们已更新Makefile以启用QEMU的用户模式网络堆栈和E1000网络卡。

Makefile将QEMU配置为将所有传入和传出的数据包记录到您的实验目录中的packets.pcap文件中。查看这些记录可能有助于确认xv6是否传输和接收您期望的数据包。要显示记录的数据包：

tcpdump -XXnr packets.pcap
我们向xv6存储库添加了一些文件，用于此实验。文件kernel / e1000.c包含E1000的初始化代码以及用于传输和接收数据包的空函数，您将填写这些函数。 kernel / e1000_dev.h包含由E1000定义的寄存器和标志位的定义，并在Intel E1000软件开发人员手册中进行了描述。 kernel / net.c和kernel / net.h包含实现IP，UDP和ARP协议的简单网络堆栈。这些文件还包含用于保存数据包的灵活数据结构的代码，称为mbuf。最后，kernel / pci.c包含在xv6引导时在PCI总线上搜索E1000卡的代码。

你的工作（难）
您的工作是完成kernel / e1000.c中的e1000_transmit（）和e1000_recv（），以便驱动程序可以传输和接收数据包。当make grade说您的解决方案通过所有测试时，您就完成了。

编写代码时，您会发现自己参考E1000软件开发人员手册。以下部分可能特别有帮助：

+第2节是必不可少的，并概述了整个设备。
+第3.2节概述了数据包接收。
+第3.3节概述了数据包传输，以及第3.4节。
+第13节概述了E1000使用的寄存器。
+第14节可能有助于您理解我们提供的init代码。
浏览E1000软件开发人员手册。此手册涵盖几个密切相关的以太网控制器。 QEMU模拟82540EM。现在浏览第2章以了解设备的感觉。要编写驱动程序，您需要熟悉第3章和第14章，以及4.1（但不是4.1的子节）。您还需要使用第13章作为参考。其他章节大多涵盖E1000的组件，您的驱动程序不必与之交互。起初不要担心细节;只是对文档的结构有所了解，以便以后找到东西。 E1000具有许多高级功能，其中大多数可以忽略。只需要一小组基本功能即可完成此实验。

我们在e1000.c中为您提供的*e1000_init（）*函数配置了E1000以从RAM读取要传输的数据包，并将接收到的数据包写入RAM。这种技术称为DMA，即直接内存访问，指的是E1000硬件直接将数据包写入和读取到RAM中。

由于数据包的突发可能比驱动程序处理它们的速度更快，因此e1000_init（）为E1000提供了多个缓冲区，E1000可以将数据包写入其中。 E1000要求这些缓冲区由RAM中的“描述符”数组描述；每个描述符包含E1000可以写入接收到的数据包的RAM中的地址。结构rx_desc描述了描述符格式。描述符数组称为接收环或接收队列。在环的意义上，当卡或驱动程序到达数组的末尾时，它会回到开头。 e1000_init（）为E1000分配了mbuf数据包缓冲区，以供E1000进行DMA，使用mbufalloc（）。还有一个传输环，驱动程序将要发送的数据包放入其中。 e1000_init（）将两个环配置为具有RX_RING_SIZE和TX_RING_SIZE的大小。

当网络堆栈在net.c中需要发送数据包时，它使用包含要发送的数据包的mbuf调用e1000_transmit（）。您的传输代码必须将指向数据包数据的指针放入TX（传输）环中的描述符中。结构tx_desc描述了描述符格式。您需要确保每个mbuf最终被释放，但只有在E1000完成传输数据包后才能释放（E1000在描述符中设置E1000_TXD_STAT_DD位以指示此）。

当E1000从以太网接收到每个数据包时，它首先将数据包DMA到由下一个RX（接收）环描述符指向的mbuf中，然后生成中断。您的e1000_recv（）代码必须扫描RX环，并通过调用net_rx（）将每个新数据包的mbuf传递给网络堆栈（在net.c中）。然后，您需要分配一个新的mbuf并将其放入描述符中，以便当E1000再次到达RX环中的那一点时，它会找到一个新的缓冲区，以便将新数据包DMA到其中。

除了在RAM中读取和写入描述符环之外，您的驱动程序还需要通过其内存映射的控制寄存器与E1000进行交互，以检测何时有可用的接收数据包，并通知E1000驱动程序已经填充了一些TX描述符以发送数据包。全局变量regs保存指向E1000的第一个控制寄存器的指针；您的驱动程序可以通过将regs索引为数组来获取其他寄存器。您将需要使用索引E1000_RDT和E1000_TDT。

要测试驱动程序，请在一个窗口中运行make server，在另一个窗口中运行make qemu，然后在xv6中运行nettests。 nettests中的第一个测试尝试向主机操作系统发送UDP数据包，地址为make server运行的程序。如果您尚未完成实验，则E1000驱动程序实际上不会发送数据包，也不会发生太多事情。

完成实验后，E1000驱动程序将发送数据包，qemu将将其传递给您的主机计算机，make server将看到它，它将发送响应数据包，然后E1000驱动程序和nettests将看到响应数据包。然而，在主机发送回复之前，它会向xv6发送“ARP”请求数据包，以查找其48位以太网地址，并期望xv6以ARP回复响应。 kernel / net.c将在您完成E1000驱动程序的工作后处理此问题。如果一切顺利，nettests将打印testing ping：OK，并且make server将打印来自xv6的消息！。

tcpdump -XXnr packets.pcap应该产生以下输出：

reading from file packets.pcap, link-type EN10MB (Ethernet)
```sh
15:27:40.861988 IP 10.0.2.15.2000 > 10.0.2.2.25603: UDP, length 19
        0x0000:  ffff ffff ffff 5254 0012 3456 0800 4500  ......RT..4V..E.
        0x0010:  002f 0000 0000 6411 3eae 0a00 020f 0a00  ./....d.>.......
        0x0020:  0202 07d0 6403 001b 0000 6120 6d65 7373  ....d.....a.mess
        0x0030:  6167 6520 6672 6f6d 2078 7636 21         age.from.xv6!
15:27:40.862370 ARP, Request who-has 10.0.2.15 tell 10.0.2.2, length 28
        0x0000:  ffff ffff ffff 5255 0a00 0202 0806 0001  ......RU........
        0x0010:  0800 0604 0001 5255 0a00 0202 0a00 0202  ......RU........
        0x0020:  0000 0000 0000 0a00 020f                 ..........
15:27:40.862844 ARP, Reply 10.0.2.15 is-at 52:54:00:12:34:56, length 28
        0x0000:  ffff ffff ffff 5254 0012 3456 0806 0001  ......RT..4V....
        0x0010:  0800 0604 0002 5254 0012 3456 0a00 020f  ......RT..4V....
        0x0020:  5255 0a00 0202 0a00 0202                 RU........
15:27:40.863036 IP 10.0.2.2.25603 > 10.0.2.15.2000: UDP, length 17
        0x0000:  5254 0012 3456 5255 0a00 0202 0800 4500  RT..4VRU......E.
        0x0010:  002d 0000 0000 4011 62b0 0a00 0202 0a00  .-....@.b.......
        0x0020:  020f 6403 07d0 0019 3406 7468 6973 2069  ..d.....4.this.i
        0x0030:  7320 7468 6520 686f 7374 21              s.the.host!
```
您的输出将略有不同，但应包含字符串“ARP，请求”，“ARP，回复”，“UDP”，“a.message.from.xv6”和“this.is.the.host”。

nettests执行其他一些测试，最终发送DNS请求到Google的一个名称服务器（真实的互联网）。您应该确保您的代码通过了所有这些测试，之后您应该看到此输出：
```sh
$ nettests
nettests running on port 25603
testing ping: OK
testing single-process pings: OK
testing multi-process pings: OK
testing DNS
DNS arecord for pdos.csail.mit.edu. is 128.52.129.126
DNS OK
all tests passed.
You should ensure that make grade agrees that your solution passes.
```

提示：  
首先向e1000_transmit()和e1000_recv()添加打印语句，然后运行make server和（在xv6中）nettests。您应该从打印语句中看到，nettests生成了对e1000_transmit的调用。
一些实现e1000_transmit的提示：

+ 首先通过读取E1000_TDT控制寄存器来询问E1000期望的TX环索引，即下一个数据包的位置。
+ 然后检查环是否溢出。如果E1000_TXD_STAT_DD在由E1000_TDT索引的描述符中未设置，则E1000尚未完成相应的先前传输请求，因此返回错误。
+ 否则，使用mbuffree()释放从该描述符传输的最后一个mbuf（如果有）。
+ 然后填写描述符。m->head指向内存中数据包的内容，m->len是数据包的长度。设置必要的cmd标志（请参阅E1000手册中的第3.3节），并存储mbuf的指针以供稍后释放。
+ 最后，通过将E1000_TDT模TX_RING_SIZE加1来更新环位置。
+ 如果e1000_transmit()成功将mbuf添加到环中，则返回0。如果失败（例如，没有可用的描述符来传输mbuf），则返回-1，以便调用者知道要释放mbuf。

一些实现e1000_recv的提示：

+ 首先通过获取E1000_RDT控制寄存器并将其加1模RX_RING_SIZE来询问E1000下一个等待接收数据包（如果有）的环索引。
+ 然后通过检查描述符状态部分中的E1000_RXD_STAT_DD位来检查是否有新数据包可用。如果没有，则停止。
+ 否则，将mbuf的m->len更新为描述符中报告的长度。使用net_rx()将mbuf传递给网络堆栈。
+ 然后使用mbufalloc()分配一个新的mbuf，以替换刚刚给予net_rx()的mbuf。将其数据指针（m->head）编程描述符。将描述符的状态位清零。
+ 最后，将E1000_RDT寄存器更新为已处理的最后一个环描述符的索引。
+ e1000_init()使用mbufs初始化RX环，您需要查看它是如何实现的，可能需要借用代码。
+ 在某个时刻，已到达的总数据包数将超过环大小（16）；确保您的代码可以处理该情况。
您需要锁来应对xv6可能从多个进程使用E1000的可能性，或者在中断到达时可能在内核线程中使用E1000。

**### 1.3实验思路与代码：**  

用于传输的buffer
tx_ring
```
cricular buffer (buf ring)

   +->  +-------------------+
   |    |                   |
   |    +-------------------+ 
   |    |       *****       | <-- head  TDH
   |    +-------------------+
   |    |       *****       |
   |    +-------------------+
   |    |       *****       |   Transmit queue
   |    +-------------------+
   |    |       *****       |
   |    +-------------------+
   |    |       *****       | <-- tail  TDT
   |    +-------------------+ 
   |    |                   |
   |    +-------------------+
   |    |                   |
   |    +-------------------+
   |              |
   +--------------+

```

用于接收的buffer
rx_ring
```
cricular buffer (buf ring)

   +->  +-------------------+
   |    |                   |
   |    +-------------------+ 
   |    |       *****       | <-- tail  RDT
   |    +-------------------+
   |    |       *****       |
   |    +-------------------+
   |    |       *****       |   Receive queue
   |    +-------------------+
   |    |       *****       |
   |    +-------------------+
   |    |       *****       | <-- head  RDH
   |    +-------------------+ 
   |    |                   |
   |    +-------------------+
   |    |                   |
   |    +-------------------+
   |              |
   +--------------+

```
我们始终关注的都是tail，head是由硬件自身驱动的，要传输的buf要挂载在tail上，要接收的内容要从tail读。对于tx_ring是head追tail，对于rx_ring是tail追head。

实验代码如下

`kernel/e1000.c`
```c
int
e1000_transmit(struct mbuf *m)
{
  //
  // Your code here.
  //
  // the mbuf contains an ethernet frame; program it into
  // the TX descriptor ring so that the e1000 sends it. Stash
  // a pointer so that it can be freed after sending.
  //
  acquire(&e1000_lock);
  printf("test_transmit\n");
  uint32 index = regs[E1000_TDT];
  if((tx_ring[index].status & E1000_TXD_STAT_DD)==0){
    release(&e1000_lock);
    printf("error::the ring is overflowing");
    return -1;
  }
  if(tx_mbufs[index])
    mbuffree(tx_mbufs[index]);
  tx_mbufs[index] = m;
  tx_ring[index].addr = (uint64)m->head;
  tx_ring[index].length = m->len;
  tx_ring[index].cmd=E1000_TXD_CMD_RS | E1000_TXD_CMD_EOP;
  regs[E1000_TDT] = (index + 1) % TX_RING_SIZE;
  release(&e1000_lock);

  return 0;
}

static void
e1000_recv(void)
{
  //
  // Your code here.
  //
  // Check for packets that have arrived from the e1000
  // Create and deliver an mbuf for each packet (using net_rx()).
  //
  printf("test_recv\n");
  
  uint32 index = regs[E1000_RDT];
  index = (index + 1) % RX_RING_SIZE;
  
  while (rx_ring[index].status & E1000_RXD_STAT_DD)
  {
    //acquire(&e1000_lock);
    rx_mbufs[index]->len = rx_ring[index].length;
    net_rx(rx_mbufs[index]);
    struct mbuf *newbuf = mbufalloc(0);
    if(!newbuf)
      panic("error::alloc mbuf failed");
    rx_mbufs[index] = newbuf;
    rx_ring[index].addr =(uint64)newbuf->head;
    rx_ring[index].status = 0;
    //release(&e1000_lock);

    index = (index + 1) % RX_RING_SIZE;
  }
  regs[E1000_RDT] = (index - 1) % RX_RING_SIZE;
}
```


---
# network 笔记

e1000从底层到顶层依次为:
```
        +-------------------+
        |        eth        |
        +-------------------+
        |        arp        |
        +-------------------+
        |        ip         |
        +-------------------+
        |        udp        |
        +-------------------+
        |                   |
        |       data        |
        |                   |
        +-------------------+
```

ethernet的数据包格式为:
```c
// an Ethernet packet header (start of the packet).
struct eth {
  uint8  dhost[ETHADDR_LEN];
  uint8  shost[ETHADDR_LEN];
  uint16 type;
} __attribute__((packed));
```

这部分遵循以太协议（包含了数据链路层和物理层），局域网通过主机的mac地址，可以找到主机，将信息发送到主机去。

mac地址是48bit的地址，在eth中表现为6个字节，前三个字节是厂商id，后三个字节是厂商分配的id。

ethernet的type字段，表示上层协议的类型，比如0x0800表示ip协议，0x0806表示arp协议，0x86dd表示ipv6协议。

arp协议的数据包格式为:
```c
// an ARP packet (comes after an Ethernet header).
struct arp {
  uint16 hrd; // format of hardware address
  uint16 pro; // format of protocol address
  uint8  hln; // length of hardware address
  uint8  pln; // length of protocol address
  uint16 op;  // operation

  char   sha[ETHADDR_LEN]; // sender hardware address
  uint32 sip;              // sender IP address
  char   tha[ETHADDR_LEN]; // target hardware address
  uint32 tip;              // target IP address
} __attribute__((packed));
```

arp协议的大致意思是，通过ip地址找到mac地址，这样就可以通过mac地址找到主机了。

当局域网收到这个包时，会对该区域所有的主机进行广播，如果有主机的ip地址和tip相同，那么该主机就会回复一个arp包，包含自己的mac地址。局域网就可以通过mac地址找到该主机了。

ip协议的数据包格式为:
```c
// an IP packet header (comes after an Ethernet header).
struct ip {
  uint8  ip_vhl; // version << 4 | header length >> 2
  uint8  ip_tos; // type of service
  uint16 ip_len; // total length
  uint16 ip_id;  // identification
  uint16 ip_off; // fragment offset field
  uint8  ip_ttl; // time to live
  uint8  ip_p;   // protocol
  uint16 ip_sum; // checksum
  uint32 ip_src, ip_dst;
};
``` 

其中核心的信息就是ip_src和ip_dst，表示源ip地址和目的ip地址，ip_p表示上层协议的类型，比如0x11表示udp协议，0x06表示tcp协议。

udp是一个简易的协议，不需要建立连接，直接发送数据包就可以了，udp的数据包格式为:
```c
// a UDP packet header (comes after an IP header).
struct udp {
  uint16 sport; // source port
  uint16 dport; // destination port
  uint16 ulen;  // length, including udp header, not including IP header
  uint16 sum;   // checksum
};
```

ps.（tcp是一个更稳定的连接协议，被广泛运用，他需要三次握手和四次挥手，在本实验中没有是使用）

进程使用socket api，表示对特定端口匹配的packet感兴趣，os会返回一个fd，主机收到与端口匹配的packet时，会将packet的内容写入到fd中，进程可以通过read系统调用读取fd中的内容。

常见的端口：dns端口--53

---
## Virtual Memory

### 1. 实现vm需要的特性（用户进程可以直接使用的vm）

+ 支持应用程序使用虚拟内存的系统调用

`mmap`（Memory Map）是一个常见的系统调用，用于在进程的地址空间中创建一个内存映射区域。它允许将一个文件或设备的内容映射到进程的地址空间中，使得进程可以直接读取和写入这些映射区域的数据，而不需要通过传统的文件读写接口。

通过`mmap`系统调用，进程可以将文件或设备映射为一段连续的虚拟内存，使得对这段内存的读写操作直接映射到文件或设备中的数据。这种映射方式可以简化文件或设备的访问，提高读写性能，并且支持一些特殊的内存操作。

下面是一个`mmap`系统调用的例子，假设我们打开一个名为"example.txt"的文本文件，并将其映射到进程的地址空间：

```c
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

int main() {
    int fd = open("example.txt", O_RDONLY); // 打开文件

    // 获取文件大小
    struct stat file_stat;
    fstat(fd, &file_stat);
    off_t file_size = file_stat.st_size;

    // 映射文件到内存
    char *file_data = mmap(NULL, file_size, PROT_READ, MAP_SHARED, fd, 0);

    // 从内存中读取文件内容
    printf("%s", file_data);

    // 解除内存映射
    munmap(file_data, file_size);

    // 关闭文件
    close(fd);

    return 0;
}
```

在这个例子中，我们使用`open`系统调用打开了一个文本文件，并通过`fstat`获取了文件的大小。然后，使用`mmap`将文件映射到进程的地址空间中，指定了映射的大小、访问权限（只读）、共享映射方式和文件描述符。最后，我们可以通过访问`file_data`指针来读取文件的内容。完成操作后，使用`munmap`解除内存映射，最后关闭文件。

通过`mmap`系统调用，我们可以直接在内存中操作文件的内容，而不需要使用传统的读写函数。这种映射方式可以提供更高效的文件访问和更灵活的内存操作。