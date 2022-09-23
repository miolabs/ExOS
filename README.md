# ExOS
Embedded posix operating system for microcontrollers

# Introduction 

My team and I, have been working as developers of our own hardware as well as third-party hardware for more than 15 years already.

We started off, like most development teams, with countless ideas and the greatest enthusiasm ever. However, during the development process, we met several of those typical problems any company dedicated to electronics’ development nowadays, generally meet, such as: The problems faced when the step of turning an idea into a prototype is reached, and how, at the same time, this prototype needs to become a viable commercial solution. Arduously complicated task! 

That specific step or transition tends to be extremely important since it will define how the final prototype will turn out to be, under normal circumstances. But, reaching this point involves taking decisions that are always difficult, and such decisions are generally filled with problems and drawbacks.

When you lack experience in electronics, it is common to choose those platforms whose learning curve seems to be faster. The problem of such option resides in the fact that, supposing that, whether more power or simply changing the micro in order to improve the final solution costs were needed, for instance, it wouldn’t be possible since the programming simplicity generally goes associated to disadvantages because it exclusively allows using that platform, alone. Those platforms often have customized development environments, and use their own exclusive code libraries, restricting or even making it impossible to change it to a different platform, given the case.  

On the contrary, when talking about experienced programmers, the usual thing to do is to program the micro yourself or to use some specific kernel for micro-controllers. This option is not exempted from problems since, in general, changing platforms can be quite difficult on itself, unless any of the chosen operative systems support that platform otherwise. In any case, and though this option might be better than the former one, specifically regarding the change of platforms issue, and since normally plenty of them support most of the micros found in the market, its limitation comes defined when you find yourself forced to programming in a very particular way, such as how it had been established for the selected kernel since there is no standard. If circumstances were to demand a change between two kernels, learning how to program the new one would become unavoidable, which would invalidate a large part of the programing code and logics used before.  

Previously, most of the hardware products were what we know as ‘standalone’, that is, that those didn’t communicate with other elements, or, if they did, it merely happened through simple protocols such as RS232, RS485, etc... 

Nowadays, the majority of the developed devices can communicate with smartphones or computers through high-level protocols like USB, Wi-Fi, Bluetooth, etc. so that settings and operating access can be made easily through them. As a matter of fact, integrated web servers usage in those devices is a common practice providing direct access to those from anywhere and allowing its configuration. 

Although possible, the previously mentioned solutions, are obviously harder, more expensive and less versatile. To solve this, choosing Linux is the most common option since we are talking about a desktop operative system that has been recently reduced significantly so that it can be run, and thanks to the micros experimenting an important evolution at the same time in recent years, all in all, sets the necessary environment in such a way, that the complex communication software development in little micro-controllers becomes easier due to having high-level functions provided by the OS itself. Using this system offers a great advantage in terms of programming since we are talking about a multi-platform OS that allows changing between platforms with barely having to change any lines of code. On the whole speaking, the code used in Linux is POSIX compatible. 

However, the OS minimum requirements are way higher than those for the two previous mentioned solutions as they demand more power and memory for its proper operation which increases exponentially the production costs of the commercial solution and hinders the access even to the simplest hardware since you are forced to go through the OS for every resource of the micro due to being the latter the one managing them all, and compels the programmer to create a driver, not an easy task, whenever you need to access any resource that the OS does not support. 

One more factor to consider is the development environment. If your only intention is programming to create a ‘standalone’ device, the process itself doesn’t normally present any problems. You just learn how to use it and that’s all that there is to it. However, if your purpose is creating some device with a communication system, then you’ll have to connect it to an application, either being a computer or an smartphone one, and there is when the problems begin. First of all, code is generally incompatible between platforms; secondly, programming the device is normally done in a particular way, in an environment that it is, often, quite different from the environment you are going to use when programming the application in the computer or smartphone, causing you to waste your time going back and forth from one environment to another, not to mention the absence of code compatibility and the fact that debugging on a micro all the time has major limitations than doing it for the same code but from a computer instead.

Three years ago, we decided to create ExOS in order to change that situation. 

The idea behind ExOS is that, now, you will just need to focus on your application device. Don’t waste time learning new things or testing hardware. Here, we present the ExOS key features:

Hereafter, starting your hardware project will be really easy since you won’t need any hardware at all. 

Simply using your computer you can code and test your hardware project thanks to ExOS being POSIX compatible. Start your code right in your computer and, if the project meets your expectations, move to the next step and build the same code in the real hardware. Either use a commercial prototype board or design your own hardware. Possibilities are endless. We are not constrained to any specific hardware; choose the one you are interested in, but don't worry if you need to change it, ExOS makes it possible because we have created a built-in multi-platform support so that you don't have to change your code again and again. 

ExOS abides two different kinds of programming: dynamic programming, which is all the POSIX programming based on dynamic memory; and static programming, which uses static memory. The latter is widely used in the world of hardware since, in a large number of cases, the micro-controllers have really limited resources and the costs for adding dynamic memory is potentially expensive in terms of processor resources. Therefore, for such cases, all the POSIX calls have their counterpart in the ExOS calls, being the only difference among them, the addition of another parameter to pass by reference the static memory in order to work. All ExOS resources such as USB, sockets, threads, etc. can be programmed either static or dynamically and even can be alternated during the programming process without distinction. 

 One of the main reasons why we kept into consideration the POSIX standard is the abundant documentation existing these days. Internet is full of tutorials, documents, etc. that makes it very easy to learn how to program pretty much anything. Many operative systems meant for computers are POSIX compatible, examples of that are, Linux or OSX, therefore we already have plenty information about it and yet, the SDK that we provide will be accompanied by its corresponding tutorials as well, complete and together with wide documentation.

# Hardware and OS services direct access, all at once! 

Though ExOS focuses on POSIX programming, we did not want to close doors to ourselves by restricting hardware access as it happens with a plain desk’s OS. Accessing can be as simply done as to go directly to the micro you want to use in order to, for example, enable or disable a pin. 

Our motivation has always been, mainly, that developers could have access to non restricted hardware. In fact, deep down, we are not exactly developing an application for computers, what we actually do is to develop an electronic device that runs the application we are developing at that right moment only. Sharing resources with other parallel applications won’t be necessary any more. 

Within ExOS, we find several hardware access levels and, depending on the requirements of the product to be developed, the developer will be able to decide what is the level of access he wants to choose. 

The most basic level of all is the direct access to the microprocessor resources, for instance: GPIO, memory, registers, etc. This is the level in which you access directly to the micro at issue without having to go through any ExOS layer. Nevertheless, such access is suitable exclusively for those cases where you want to perform simple actions or pretty easy ones such as activating a pin, a single command whose operation in upper layers would make, using as many code as possible, unavoidable. On the contrary, this option is not recommended when performing more difficult operations since its access is restricted to the micro from which programming is being done. If we changed the micro in this situation, that part of the code will probably turn incompatible with the new one. As a result, restricting its use to simple actions would be very easy for the new micro when the time to change codes comes. 

The next level is the hardware abstraction layer (HAL). This layer allows the calls being used to be multi-platform without requiring many lines of code. This is the layer where the ExOS is based on and the most suitable for using the micro’s basic peripherals, such as RS232, CAN, SPI buses, etc., or other peripherals like the timers, ADC, etc. This layer is not recommended, on another hand, for highly complex resources such as sockets, USB, Bluetooth, etc. since it is way harder to use and because such resources usually involve programming stacks such as the TCP/IP that ExOS provides already in the next access level. 

The third level of access to hardware takes places though the system services, just like it would happen in any desktop application programming. By means of these resources, sockets, Bluetooth, USB, etc. programming becomes easy since all the hardware abstraction and the operation logics of those peripherals are already included in the standard ExOS. Thus, for example, programming some code that acts as a server in an Ethernet network is just as easy as if you did it using a desktop OS like Linux, instead. In fact, using the POSIX programming, the code would be exactly the same. This is the maximum access layer and the one we recommend when you want to use more complex peripherals and protocols. 

The most important advantage that defines ExOS is that choosing the abstraction layer you want to use while you are programming won’t be necessary. Within the code itself, having access to any of the three layers at any time it’s perfectly possible. That is, it would be feasible to just create a TCP/IP server in the microcontroller using the services of the system’s layer to manage sockets and, once the data packet is received, we can send it through the bus CAN using the API of the ExOS’ abstraction layer and then activate a LED by directly accessing the pin of the micro, to which the LED is connected to, in order to view the moment the board is receiving the data. 

Example of fully functional ExOS real code on a micro LPC177x, as mentioned above:

Amazing, isn’t it? Just a few lines of code to accomplish that!!
Amazing, isn’t it? Just a few lines of code to accomplish that!!
A unified platform where you won’t have to work with several development environments. 

Don't you hate being forced to develop a combined solution using an application for smartphone or computer as well as an embedded hardware on several IDEs?. And having to code your smartphone or computer application on an IDE like Xcode or Visual Studio and then change it to another IDE like Microchip, CodeWarrior, CrossWorks, etc. in order to develop the hardware?. We do not like it either!. 

Nowadays, the environments most used for applications’ programming, either desktop or mobility ones, are the Xcode and the Visual Studio. Both are programming environments with many years of experience under their belts. Those two have a high amount of resources for the developer both in terms of programming and debugging. 

We adapted the ExOS’ SDK in order for programming in Xcode or Visual Studio to be possible without changing the environment when implementing the desired function into the micro. This way, you can have both projects in the same solution, the mobile/desk application you want to connect to and the application you will run on the real hardware.

For debugging to be easily done, the SDK allows 4 ways of debugging and testing the application being developed through ExOS in order to know, as soon as possible, if the intended functionality is right or if its code has been well done. 

In some cases, for all the hardware mainly using OS services such as sockets, serial ports, USB, etc. you can code directly in your computer just using POSIX. This helps greatly when debugging on a computer with wider resources and then, once the development is done, the only part left would be compiling the same code in the hardware through ExOS, without changing a single line of code. That would be the case of, for instance, programming an Ethernet adapter to a Serial where it would be enough to use the OS services. 

In other cases, performing this tests on the computer wouldn’t be possible since there can be certain special hardware requirements that the computer lacks. Given that situation, we have two choices: First one is the simulator and second one is running the code in the native device. 

About the simulator. Simulation of hardware elements and peripherals not existing in the computer can be done through controls and virtual displays included in the SDK; or else, the simulator can be connected to a certain device in ‘master-slave’ mode, reading and writing the peripheral values such as, for instance, LEDs, ADC, etc. in the real board. In this case, we wouldn’t be emulating any specific micro, and it would be specifically recommended to debug the programming logics since the computer where it is being executed has more tools for such debug.

And last but not least, we need to talk about the hardware itself. There are situations in which emulating the whole hardware that it’s being developed comes of as impossible. When this happens, the IDE can run and debug directly on the board we are working with. Obviously, debugging will be restricted to the resources available for the micro that’s being used for that purpose.

Most of the boards our SDK supports can serve as a link for the simulator working as a slave board of the former one or acting as an independent board able to record the program to the board itself and without having to go through the simulator steadily. However, even though we have two development boards, a fundamentally basic one and an advanced one, to process video and audio, it would actually be feasible using any other commercial board in the market since ExOS is not limited to our own boards alone.

Another tool the SDK includes is an assistant for the development boards configuration. This allows creating the BSP as if it was the easiest task ever, even when we lack the electronic configuration of that board in ExOS.

# Access to tons of libraries and services 

In order to make possible the transition to a prototype from an idea, and to bring forth the proper tests to check its feasibility in a flexible and quick way, we implemented huge amounts of code libraries, services, modules, frameworks and drivers to the ExOS so that you can focus on testing your idea solely. In any case, if it happens that any peripheral service or driver, despite everything, was not included, creating the missing part would be rather easy. 

Within ExOS we find, in the first place, the system’s API which is the entire set of calls that make up the POSIX standard, as well as the ExOS static version of those calls. As a part of this classification we have sockets, timers, threads, semaphores, locks, mutex, queue, etc. At the OS’s API, we can also find additional services that didn’t exist originally in the standard which are helpful when programming events, like the dispatcher. 

Secondly, we have drivers. To prevent you from searching for third-parties’ code in order to implement peripherals, we included many drivers. We have drivers for most devices:

USB: keyboard, mouse, web cams, printers, flash drives, ftdi, etc... 
File systems: fat, jfs, ext2, fat32, nfs, etc... 
Buses: RS232, RS485, CAN, SPI, I2C, etc...
Codecs: NTSC, PAL, MP3, mp4, amr, etc... 
Peripherals: Touch screens, ADC, etc... 
Communication: Bluetooth, Ethernet, WiFi, ZigBee, etc...
In the third place, there are the services. These shouldn’t be confused with the system’s services but rather it is about those services in which all the most common network services meet, as e.g., DHCP, FTP, HTTP, SSH, BONJOUR, DNS, etc. All of them have been implemented and optimized for its perfect performance on the application you are running, reducing to the maximum the resources they consume.

And finally, we must talk about the libraries and frameworks. On certain occasions, when you already got used to programming using high-level languages, usually, it is not only used the language itself but you use the libraries included in the environment as well. For this purpose, regarding our particular situation, we had been using C/C++/Obj-C for our development projects for so long that we decided to make a wrapper for ExOS of the most used libraries/frameworks when programming for OSX and iOS, as for example, CoreFoundation, CoreNetwork, CoreGraphics, CoreBluetooth, etc. so that we could even use the high-level code employed in an OSX/iOS application.

One of the most important libraries we included in ExOS is the Apple accessory protocol library (iAP). With this library creating an accessory to communicate with an iPhone, iPad or iPod becomes very easy. Inside the code you just have to open the protocol as if it was a device and that’s it. You can read and write from and by the iPhone.

Just a little code example:

Open an iAP protocol echoing the data data from the iPhone
Open an iAP protocol echoing the data data from the iPhone
ExOS has been designed and programmed in C, the same way it happens with all the libraries that it includes but, currently, it supports the C, C++, OBJ-C and Swift languages as well, and we are planning to extend its development so that it becomes possible to handle even more frameworks and not just the libraries in C, but also the ones in OBJ-C like UIKit or AppKit, to be able to even design user interfaces on the Xcode itself.
