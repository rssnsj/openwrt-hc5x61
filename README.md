openwrt-hc5761
==============

OpenWrt Patch for HiWiFi HC5761 / HC5661 (based on "barrier_breaker" branch)

极路由 HC5761 / HC5661（极壹、极贰、极壹S）OpenWrt补丁（基于barrier_breaker分支）

### 关于极贰
* 极贰官方产品页：http://www.hiwifi.com/j2
* 极贰购买页面：http://item.jd.com/1184730.html
* 图片展示：http://www.hiwifi.com/wp-content/themes/hi4/images/products/j2-p1.jpg

-------

### HC5761/HC5661 OpenWrt补丁说明

#### 包含的功能
* MT7620A SoC板级支持;
* 2.4G Wi-Fi驱动支持;
* SD卡驱动支持;
* USB驱动支持;
* 注意：HC5761（极2）的5G Wi-Fi（MT7610E）驱动暂未有开源支持。

#### 编译好的固件下载
 * 请在本项目的Releases下载。

#### 固件编译方法

    # 安装必需的软件包（仅限Ubuntu/Debian）
    sudo apt-get install build-essential git flex gettext libncurses5-dev unzip gawk liblzma-dev u-boot-tools
    
    # 编译
    git clone https://github.com/rssnsj/openwrt-hc5761.git -b master openwrt-hc5761
    cd openwrt-hc5761
    make

#### 说明
* 以上svn目录包含了HC5761的OpenWrt补丁，checkout下来代码直接make，会自动打补丁、自动配置menuconfig，并自动编译；
* 编译时若碰到代码包下载失败，或下载过于缓慢，请先 Ctrl + C 暂停，手动下载同名的文件放到 openwrt-ramips/dl 下面，再执行“make”继续编译；
* openwrt-ramips-mt7620a-hiwifi-hc5761-squashfs-sysupgrade.bin 是sysupgrade格式的固件，传到路由器的/tmp下，通过串口登录极2/极1S，执行以下命令刷入：

    `sysupgrade -F -n openwrt-ramips-mt7620a-hiwifi-hc5761-squashfs-sysupgrade.bin`

* 本编译方法同时会在recovery.bin目录下生成 openwrt-HC5761-recovery.bin、openwrt-HC5661-recovery.bin 两个文件，分别是极2、极1S的带u-boot固件，u-boot解锁后可用TFTP刷机。

-------
#### 【附】TFTP刷机方法
* 首先确认你的u-boot是已经解锁的，方法你懂的 :) .
* 为你电脑的“本地连接”加一个IP: 192.168.1.88 / 255.255.255.0 
* 解压刷机工具包（例如 tftpd64.400.rar）, 根据操作系统位数, 32位运行tftpd32, 64位运行tftpd64.
* 用上面生成的相应机型的 XXXX-recovery.bin 替换刷机包目录下的 recovery.bin .
* 拔掉HiWiFi电源线.
* 网线一头连HiWiFi的LAN口, 一头连电脑.
* 用尖锐物按住HiWiFi背后的reset孔不松动, 同时给路由器接电.
* 直到看到tftpd出现进度条, 松开.
* 前面板的指示灯会轮流闪烁, 表明内部Flash正在擦写, 此时不要断电.
* ping 192.168.1.1, 直到ping通, 刷机过程总共3-5分钟.

-------
#### 旧资料
* HC6361（极壹）已被OpenWrt官方支持 ( http://wiki.openwrt.org/toh/hiwifi/hc6361 )，关于HC6361的编译、配置方法请移步至：https://github.com/rssnsj/openwrt-hc6361
