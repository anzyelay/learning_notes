# 原文地址
1. [Debug a Vala or C App](https://www.marco.betschart.name/areas/elementary-os/debug-vala-or-c-app)

# Enable Debug Output
Set the environment variable **G_MESSAGES_DEBUG** to all to get all log messages printed to stdout:
```shell
export G_MESSAGES_DEBUG=all
```


# Using GNU Debugger
## Installing GNU Debugger
```sh
sudo apt install gdb
```
## Retrieve a backtrace (e.g. to debug a Segmentation fault)
Run the program in question with GNU Debugger:
```sh
$ gdb /path/to/executable
...
Reading symbols from /path/to/executable...
(gdb) run
```
Once the application crashes you should see a message similar to the following:
```sh
Thread 1 "executable" received signal SIGSEGV, Segmentation fault.
0x00007ffff709b434 in my_function_in_which_the_code_crashed () from /lib/x86_64-linux-gnu/libxyz-1.6.so.62
(gdb)
```
To retrieve a backtrace at this state, simply execute the **backtrace** command:
```sh
(gdb) backtrace
#0  0x00007ffff709b434 in my_function_in_which_the_code_crashed () at /lib/x86_64-linux-gnu/libxyz-1.6.so.62
#1  0x00005555555962ef in my_previously_executed_function (node=0x7fffffffcbc0, highest=1537725293) at ../src/MyCode/UsefulStuff.vala:168
...
(gdb) quit
```
## Retrieve a backtrace for a warning or critical message
In this case you want to abort the program as soon as a warning or critical message is logged. To do so, the G_DEBUG environment variable comes in handy:
```sh
# Causes GLib to abort the program at the first call to g_warning()
# or g_critical(). Use of this flag is not recommended except when
# debugging:
export G_DEBUG=fatal-warnings

# Causes GLib to abort the program at the first call to g_critical().
# This flag can be useful during debugging and testing:
export G_DEBUG=fatal-criticals
```
Once the environment variable is set, it causes the application to crash at the desired place. You are then able to retrieve a backtrace as described above.


# Using core dumps
You can let Linux automatically collect the required information for a backtrace whenever an application crashes unexpectedly. This information is stored in a so called "core dump" file which contains the recorded state of the complete memory from the crashed application. Those core dumps are especially useful for errors which occur seemingly at random, because you don't need to explicitly start the application using GNU Debugger and you are still able to retrieve a backtrace after a crash.
## Installing systemd-coredump
```sh
sudo apt install systemd-coredump
```
## Debug existing core dump
```sh
$ sudo coredumpctl debug /path/to/executable
...
(gdb) backtrace
#0  0x00007ffff709b434 in my_function_in_which_the_code_crashed () at /lib/x86_64-linux-gnu/libxyz-1.6.so.62
#1  0x00005555555962ef in my_previously_executed_function (node=0x7fffffffcbc0, highest=1537725293) at ../src/MyCode/UsefulStuff.vala:168
...
(gdb) quit
```
