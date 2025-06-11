# debug

如何将内核中的接口导出给用户态使用

## 方法一 `DEVICE_ATTR`的宏定义: 内核驱动与文件系统的交互

>> DEVICE_ATTR是Linux内核中用于声明设备属性文件的一个宏，它定义在<linux/device.h>头文件中。通过DEVICE_ATTR，可以在sys文件系统（sysfs）中自动创建文件，这些文件代表了设备的属性。用户空间的应用程序可以通过标准的文件操作（如cat和echo命令）来读取和写入这些属性文件，从而实现与内核驱动的交互。

示例： 使用时在sys目录查找下**my_device_test**文件名即可找到对应接口

```c
static DEVICE_ATTR(my_device_test, S_IWUSR | S_IRUSR, show_my_device, set_my_device); // my_device_test是属性文件名，S_IWUSR | S_IRUSR是文件权限，show_my_device和set_my_device是读取和写入属性文件的回调函数。
static ssize_t show_my_device(struct device *dev, struct device_attribute *attr, char *buf)
{
    // 实现读取数据的逻辑，并将结果存储在buf中
    return sprintf(buf, "%s\n", "some_data");
}

static ssize_t set_my_device(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
    // 实现写入数据的逻辑
    // 例如，将buf中的数据保存到某个变量中
    return len; // 返回写入的字节数
}
// 在probe函数中添加以下代码：
device_create_file(&pdev->dev, &dev_attr_my_device_test);
// 在remove函数中添加以下代码：
device_remove_file(&pdev->dev, &dev_attr_my_device_test);
```

还有`DEVICE_ATTR_RO`

```c
static ssize_t phy_state_show(struct device *dev, struct device_attribute *attr, char *buf)
{
struct phy_device *phydev = to_phy_device(dev);
return sprintf(buf, "0x%.8lx\n", (unsigned long)phydev->phy_id);
}
static DEVICE_ATTR_RO(phy_state);

```

## 方法二 ：使用debugfs

1. 需要引用的头文件： debugfs.h - a tiny little debug file system
1. 创建目录： `debugfs_create_dir(const char *name, struct dentry *parent)`
1. 创建文件： `debugfs_create_file(const char *name, umode_t mode, struct dentry *parent, void *data, const struct file_operations *fops)`
1. 删除文件： `debugfs_remove(const struct dentry *dentry)`
1. 删除目录： `debugfs_remove_recursive(const struct dentry *dentry)`
1. 定义文件操作函数： file_operations

    示例代码：接口参数不一定能用，需要根据实际需求修改

    ```c
    #include <linux/debugfs.h>

    static ssize_t my_device_test_get(struct file *file, char __user *user_buf, size_t count, loff_t *ppos)
    {
        // 实现读取数据的逻辑
        return count;
    }

    static int my_device_test_set(struct file *file, const char __user *user_buf, size_t count, loff_t *ppos)
    {
        // 实现写入数据的逻辑
        return count;
    }


    DEFINE_DEBUGFS_ATTRIBUTE(my_device_test_fops, my_device_test_get, my_device_test_set, "");

    static int __init my_device_init(void)
    {
        // 创建debugfs目录
        struct dentry *debugfs_dir = debugfs_create_dir("my_device", NULL);
        if (!debugfs_dir) {
            pr_err("Failed to create debugfs directory\n");
            return -ENOMEM;
        }

        // 创建debugfs文件
        debugfs_create_file("test", 0644, debugfs_dir, NULL, &my_device_test_fops);

        return 0;
    }

    static void __exit my_device_exit(void)
    {
        // 删除debugfs文件
        debugfs_remove(debugfs_create_file("test", 0644, NULL, NULL, &my_device_test_fops));

        // 删除debugfs目录
        debugfs_remove_recursive(debugfs_create_dir("my_device", NULL));
    }

    module_init(my_device_init);
    module_exit(my_device_exit);
    ```

1. 编译并加载模块：
1. 查看debugfs目录： cat /sys/kernel/debug/my_device/test

## 查看调用堆栈

使用`WARN(1, "====\n")`打印可以输出调用堆栈

## 使能Kernal Panic查看上次panic信息

```sh
commit 2137ec501b6842eff88fe0f8a979447c06fd1c30
Author: Shujun Wang <walter.sj.wang@fii-foxconn.com>
Date:   Fri Jan 24 15:56:17 2025 +0800

    Kernel Panic: Enable RAMOOPS to capture the last crash or panic log to RAM

    After kernel panic, we may be able to get the crash log in next reboot

    How to get the log:
    $ mkdir /tmp/pstore
    $ mount -t pstore pstore /tmp/pstore
    $ cd /tmp/pstore
    $ cat console-ramoops-0
    $ cat dmesg-ramoops-0

    We can get the logs from the files: console-ramoops-0 or dmesg-ramoops-0

diff --git a/sources/meta-foxconn/recipes-kernel/linux/files/linux-6.6-foxconn/arch/arm64/configs/defconfig b/sources/meta-foxconn/recipes-kernel/linux/files/linux-6.6-foxconn/arch/arm64/configs/defconfig
index 2f271df40..f4b50dc83 100755
--- a/sources/meta-foxconn/recipes-kernel/linux/files/linux-6.6-foxconn/arch/arm64/configs/defconfig
+++ b/sources/meta-foxconn/recipes-kernel/linux/files/linux-6.6-foxconn/arch/arm64/configs/defconfig
@@ -1329,6 +1329,17 @@ CONFIG_TMPFS=y
 CONFIG_TMPFS_POSIX_ACL=y
 CONFIG_TMPFS_XATTR=y

+CONFIG_PSTORE_CONSOLE=y
+CONFIG_PSTORE_PMSG=y
+CONFIG_PSTORE_RAM=y
+CONFIG_PSTORE_ZONE=y
+CONFIG_PSTORE_BLK=y
+CONFIG_PSTORE_BLK_BLKDEV="lastpanic"
+CONFIG_PSTORE_BLK_KMSG_SIZE=512
+CONFIG_PSTORE_BLK_MAX_REASON=2
+CONFIG_PSTORE_BLK_PMSG_SIZE=128
+CONFIG_PSTORE_BLK_CONSOLE_SIZE=128
+
 # CONFIG_WLAN_VENDOR_ADMTEK is not set
 # CONFIG_WLAN_VENDOR_ATH is not set
 # CONFIG_WLAN_VENDOR_ATMEL is not set
```

## 查看kernel bootup 的各模块耗时

> initcall_debug	[KNL] Trace initcalls as they are executed.  Useful
> for working out where the kernel is dying during
> startup.
>
>           ------ <<Documentation/admin-guide/kernel-parameters.txt>>

可以用来检查开机启动时哪个服务比较慢

- linux

    CONFIG_CMDLINE="initcall_debug loglevel=8"

- colect tty log to log.txt, sort by consumer time

    ```sh
    cat log.txt | grep "initcall" | sed "s/\(.*\)after\(.*\)/\2 \1/g" | sort -n
    ```

- 该参数定义在init的main.c文件中。

    ```c
    bool initcall_debug;
    core_param(initcall_debug, initcall_debug, bool, 0644);
    ```

## PM时间分析工具

> pm-graph: suspend/resume/boot timing analysis tools
