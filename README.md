openwrt-hc5x61
==============

OpenWrt Patch for HiWiFi HC5661 / HC5761 / HC5861 (based on "chaos_calmer" branch)

极路由 HC5661 / HC5761 / HC5861 （极1S、极2、极3）OpenWrt补丁（基于chaos_calmer分支）

### 关于极路由
* 极路由官方产品页：http://www.hiwifi.com/

-------

#### OpenWrt补丁及ImageBuilder说明

* 包含了HC5661、HC5761、HC5861 3个机型的OpenWrt补丁；
* 可以Checkout下来代码直接make，会自动打补丁、自动载入menuconfig配置，并自动编译；
* 也可以直接利用ImageBuilder，将机型支持代码（dts描述文件），与OpenWrt官方二进制程序合并成相应机型的固件。

#### 固件包含的功能
* 基于 OpenWrt Barrier Breaker - r43770 版本；
* 支持获取正确的MAC地址（HiWiFi bdinfo中的文本格式）；
* 完全适配HiWiFi的flash layout，不会丢key；
* 自带5G Wi-Fi驱动；
* 内置基于IP地址、域名列表的透明代理服务（SS），及其LuCi配置界面；
* 内置基于IP地址、域名列表的非标准VPN服务（minivtun），及其LuCI配置界面；
* 内置“文件中转站”功能，自动挂载USB存储以及自动配置Samba服务。

#### 编译好的固件下载
* 请在本项目的Releases下载，或者 http://rssn.cn/roms/ 。

#### 前提工作

    # 安装必需的软件包（仅限Ubuntu/Debian）
    sudo apt-get install build-essential git subversion wget flex gettext libncurses5-dev unzip gawk liblzma-dev zlib1g-dev ccache u-boot-tools
      
    # Checkout项目代码
    git clone https://github.com/rssnsj/openwrt-hc5x61.git -b chaos_calmer openwrt-hc5x61-cc

#### 固件生成方法1 - 编译

    cd openwrt-hc5x61-cc
    make
    # 编译成功后，固件文件位于: openwrt-ramips/bin/oopenwrt-ramips-mt7620-hiwifi-hc5761-squashfs-sysupgrade.bin

#### 固件生成方法2 - 利用ImageBuilder将机型支持代码与OpenWrt官方二进制程序合并生成固件

    cd openwrt-hc5x61-cc/ImageBuilder
      
    # 解压ImageBuilder和SDK（事先从downloads.openwrt.org下载好）
    tar jxvf xxx/OpenWrt-ImageBuilder-ramips-mt7620.Linux-x86_64.tar.bz2
    tar jxvf xxx/OpenWrt-SDK-ramips-mt7620_gcc-4.8-linaro_uClibc-0.9.33.2.Linux-x86_64.tar.bz2
      
    # 生成极2固件：
    make HC5761 FEEDS=1 RALINK=1
    # 生成极3固件：
    make HC5861 FEEDS=1 RALINK=1
    # FEEDS=1 表示包含项目 rssnsj/network-feeds 的功能在内
    # RALINK=1 表示包含5G驱动在固件中

#### 刷机方法
  以极2为例，openwrt-ramips-mt7620-hiwifi-hc5761-squashfs-sysupgrade.bin 是sysupgrade格式的固件，传到路由器的/tmp下，通过SSH或串口登录路由器Shell，执行以下命令刷入：

  首先最好将U-boot替换成解锁版（tftp刷机时U-boot不对固件做校验）的，以防止万一刷砖无法直接tftp刷root固件：

    # 极1S
    cd /tmp
    wget http://rssn.cn/roms/uboot/HC5661-uboot.bin
    mtd write HC5661-uboot.bin u-boot
      
    # 极2
    cd /tmp
    wget http://rssn.cn/roms/uboot/HC5761-uboot.bin
    mtd write HC5761-uboot.bin u-boot
      
    # 极3
    cd /tmp
    wget http://rssn.cn/roms/uboot/HC5861-uboot.bin
    mtd write HC5861-uboot.bin u-boot

  如果上面的地址失效，也可以从这里获取U-boot映像： https://github.com/rssnsj/firmware-tools/tree/hiwifi

  然后刷入固件：

    sysupgrade -F -n openwrt-ramips-mt7620-hiwifi-hc5761-squashfs-sysupgrade.bin

#### 说明
* Releases中提供的xxx-hiwifi-hc5761-xxx是极1S、极2通用的，xxx-hiwifi-hc5861-xxx是极3；
* 编译时若碰到代码包下载失败，或下载过于缓慢，请先 Ctrl + C 暂停，手动下载同名的文件放到 openwrt-ramips/dl 下面，再执行“make”继续编译；
* HC5761（极2）的5G（MT7610E）、HC5861（极3）的5G（MT7612E）驱动暂未有开源支持，Releases中可下载到编译好的已带5G驱动的固件，但由于MTK版权限制本项目不提供其源代码。

