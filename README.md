openwrt-hc5x61
==============

OpenWrt Patch for HiWiFi HC5661 / HC5761 / HC5861 (based on "barrier_breaker" branch)

极路由 HC5661 / HC5761 / HC5861 （极1S、极2、极3）OpenWrt补丁（基于barrier_breaker分支）

### 关于极路由
* 极路由官方产品页：http://www.hiwifi.com/

-------

#### OpenWrt补丁及ImageBuilder说明

* 包含了HC5661、HC5761、HC5861 3个机型的OpenWrt补丁；
* 可以Checkout下来代码直接make，会自动打补丁、自动载入menuconfig配置，并自动编译；
* 也可以直接利用ImageBuilder，将机型支持代码（dts描述文件），与OpenWrt官方二进制程序合并成相应机型的固件。

#### 固件包含的功能
* 极1S、极2、极3均完美支持；
* 2.4G开源版Wi-Fi驱动支持；
* 自带5G Wi-Fi驱动（仅限本项目Releases中已编译好的固件）；
* SD卡驱动支持；
* USB驱动支持；
* 极1S、极2、极3完善的LED定义与配置；
* 支持获取正确的MAC地址（HiWiFi bdinfo中的文本格式）；
* 内置基于ipset按需代理，支持最小流量模式的Shadowsocks服务（ss-redir.sh）；
* 支持Shadowsocks加速服务，及其LuCI界面，支持自定义gfwlist。

#### 编译好的固件下载
* 请在本项目的Releases下载，或者 http://rssn.cn/roms/ 。

#### 前提工作

    # 安装必需的软件包（仅限Ubuntu/Debian）
    sudo apt-get install build-essential git subversion wget flex gettext libncurses5-dev unzip gawk liblzma-dev zlib1g-dev ccache u-boot-tools
      
    # Checkout项目代码
    git clone https://github.com/rssnsj/openwrt-hc5x61.git

#### 固件生成方法1 - 编译

    cd openwrt-hc5x61
    make
    # 编译成功后，固件文件位于: openwrt-ramips/bin/oopenwrt-ramips-mt7620a-hiwifi-hc5761-squashfs-sysupgrade.bin

#### 固件生成方法2 - 利用ImageBuilder将机型支持代码与OpenWrt官方二进制程序合并生成固件

    cd openwrt-hc5x61/ImageBuilder
      
    # 解压ImageBuilder和SDK（事先从downloads.openwrt.org下载好）
    tar jxvf xxx/OpenWrt-ImageBuilder-ramips_mt7620a-for-linux-x86_64.tar.bz2
    tar jxvf xxx/OpenWrt-SDK-ramips-for-linux-x86_64-gcc-4.8-linaro_uClibc-0.9.33.2.tar.bz2
      
    # 生成极2固件：
    make HC5761 FEEDS=1 RALINK=1
    # 生成极3固件：
    make HC5861 FEEDS=1 RALINK=1
    # FEEDS=1 表示包含项目 rssnsj/network-feeds 的功能在内
    # RALINK=1 表示包含5G驱动在固件中

#### 刷机方法
  以极2为例，openwrt-ramips-mt7620a-hiwifi-hc5761-squashfs-sysupgrade.bin 是sysupgrade格式的固件，传到路由器的/tmp下，通过SSH或串口登录路由器Shell。

  首先最好将U-boot替换成解锁版（不校验固件是否是官方发布的）的，以防止万一刷砖无法直接tftp刷root固件：

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

  如果上面的地址失效，也可以从这里获取： https://github.com/rssnsj/firmware-tools/tree/hiwifi

  然后刷入固件：

    sysupgrade -F -n openwrt-ramips-mt7620a-hiwifi-hc5761-squashfs-sysupgrade.bin

#### 说明
* 编译时若碰到代码包下载失败，或下载过于缓慢，请先 Ctrl + C 暂停，手动下载同名的文件放到 openwrt-ramips/dl 下面，再执行“make”继续编译；
* HC5761（极2）的5G（MT7610E）、HC5861（极3）的5G（MT7612E）驱动暂未有开源支持，Releases中可下载到编译好的已带5G驱动的固件，但由于MTK版权限制本项目不提供其源代码。

