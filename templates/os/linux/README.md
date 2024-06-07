
# Linux by Zabbix agent

## Overview

This is an official Linux template. It requires Zabbix agent 7.2 or newer.

#### Notes on filesystem (FS) discovery:
- The ext4/3/2 FS reserves space for privileged usage, typically set at 5% by default.
- BTRFS allocates a default of 10% of the volume for its own needs.
- To mitigate potential disasters, FS usage triggers are based on the maximum available space.
  - Utilization formula: `pused = 100 - 100 * (available / total - free + available)`
- The FS utilization chart, derived from graph prototypes, reflects FS reserved space as the difference between used and available space from the total volume.

## Requirements

Zabbix version: 7.2 and higher.

## Tested versions

This template has been tested on:
- Linux OS

## Configuration

> Zabbix should be configured according to the instructions in the [Templates out of the box](https://www.zabbix.com/documentation/7.2/manual/config/templates_out_of_the_box) section.

## Setup

Install Zabbix agent on Linux OS following Zabbix [documentation](https://www.zabbix.com/documentation/7.2/manual/concepts/agent#agent-on-unix-like-systems).

### Macros used

|Name|Description|Default|
|----|-----------|-------|
|{$AGENT.TIMEOUT}|<p>Timeout after which agent is considered unavailable. Works only for agents reachable from Zabbix server/proxy (passive mode).</p>|`3m`|
|{$CPU.UTIL.CRIT}|<p>Critical threshold of CPU utilization expressed in %.</p>|`90`|
|{$LOAD_AVG_PER_CPU.MAX.WARN}|<p>The CPU load per core is considered sustainable. If necessary, it can be tuned.</p>|`1.5`|
|{$VFS.FS.FSNAME.NOT_MATCHES}|<p>Used for filesystem discovery. Can be overridden on the host or linked template level.</p>|`^(/dev\|/sys\|/run\|/proc\|.+/shm$)`|
|{$VFS.FS.FSNAME.MATCHES}|<p>Used for filesystem discovery. Can be overridden on the host or linked template level.</p>|`.+`|
|{$VFS.FS.FSTYPE.MATCHES}|<p>Used for filesystem discovery. Can be overridden on the host or linked template level.</p>|`Macro too long. Please see the template.`|
|{$VFS.FS.FSTYPE.NOT_MATCHES}|<p>Used for filesystem discovery. Can be overridden on the host or linked template level.</p>|`^\s$`|
|{$VFS.FS.INODE.PFREE.MIN.CRIT}|<p>The critical threshold of the filesystem metadata utilization.</p>|`10`|
|{$VFS.FS.INODE.PFREE.MIN.WARN}|<p>The warning threshold of the filesystem metadata utilization.</p>|`20`|
|{$VFS.FS.PUSED.MAX.CRIT}|<p>The critical threshold of the filesystem utilization.</p>|`90`|
|{$VFS.FS.PUSED.MAX.WARN}|<p>The warning threshold of the filesystem utilization.</p>|`80`|
|{$MEMORY.UTIL.MAX}|<p>Used as a threshold in the memory utilization trigger.</p>|`90`|
|{$MEMORY.AVAILABLE.MIN}|<p>Used as a threshold in the available memory trigger.</p>|`20M`|
|{$SWAP.PFREE.MIN.WARN}|<p>The warning threshold of the minimum free swap.</p>|`50`|
|{$VFS.DEV.READ.AWAIT.WARN}|<p>The average response time (in ms) of disk read before the trigger fires.</p>|`20`|
|{$VFS.DEV.WRITE.AWAIT.WARN}|<p>The average response time (in ms) of disk write before the trigger fires.</p>|`20`|
|{$VFS.DEV.DEVNAME.NOT_MATCHES}|<p>Used for block device discovery. Can be overridden on the host or linked template level.</p>|`Macro too long. Please see the template.`|
|{$VFS.DEV.DEVNAME.MATCHES}|<p>Used for block device discovery. Can be overridden on the host or linked template level.</p>|`.+`|
|{$IF.ERRORS.WARN}|<p>Warning threshold of error packet rate. Can be used with interface name as context.</p>|`2`|
|{$IFCONTROL}|<p>Link status trigger will be fired only for interfaces where the context macro equals "1".</p>|`1`|
|{$NET.IF.IFNAME.MATCHES}|<p>Used for network interface discovery. Can be overridden on the host or linked template level.</p>|`^.*$`|
|{$NET.IF.IFNAME.NOT_MATCHES}|<p>Filters out `loopbacks`, `nulls`, docker `veth` links and `docker0` bridge by default.</p>|`Macro too long. Please see the template.`|
|{$IF.UTIL.MAX}|<p>Used as a threshold in the interface utilization trigger.</p>|`90`|
|{$SYSTEM.FUZZYTIME.MAX}|<p>The threshold for the difference of system time in seconds.</p>|`60`|
|{$KERNEL.MAXPROC.MIN}||`1024`|
|{$KERNEL.MAXFILES.MIN}||`256`|

### Items

|Name|Description|Type|Key and additional info|
|----|-----------|----|-----------------------|
|Version of Zabbix agent running||Zabbix agent|agent.version<p>**Preprocessing**</p><ul><li><p>Discard unchanged with heartbeat: `1d`</p></li></ul>|
|Host name of Zabbix agent running||Zabbix agent|agent.hostname<p>**Preprocessing**</p><ul><li><p>Discard unchanged with heartbeat: `1d`</p></li></ul>|
|Zabbix agent ping|<p>The agent always returns "1" for this item. May be used in combination with `nodata()` for the availability check.</p>|Zabbix agent|agent.ping|
|Zabbix agent availability|<p>Used for monitoring the availability status of the agent.</p>|Zabbix internal|zabbix[host,agent,available]|
|Number of CPUs||Zabbix agent|system.cpu.num<p>**Preprocessing**</p><ul><li><p>Discard unchanged with heartbeat: `1d`</p></li></ul>|
|Load average (1m avg)|<p>Calculated as the system CPU load divided by the number of CPU cores.</p>|Zabbix agent|system.cpu.load[all,avg1]|
|Load average (5m avg)|<p>Calculated as the system CPU load divided by the number of CPU cores.</p>|Zabbix agent|system.cpu.load[all,avg5]|
|Load average (15m avg)|<p>Calculated as the system CPU load divided by the number of CPU cores.</p>|Zabbix agent|system.cpu.load[all,avg15]|
|CPU utilization|<p>CPU utilization expressed in %.</p>|Dependent item|system.cpu.util<p>**Preprocessing**</p><ul><li><p>JavaScript: `//Calculate utilization<br>return (100 - value)`</p></li></ul>|
|CPU idle time|<p>Time the CPU has spent doing nothing.</p>|Zabbix agent|system.cpu.util[,idle]|
|CPU system time|<p>Time the CPU has spent running the kernel and its processes.</p>|Zabbix agent|system.cpu.util[,system]|
|CPU user time|<p>Time the CPU has spent running users' processes that are not niced.</p>|Zabbix agent|system.cpu.util[,user]|
|CPU nice time|<p>Time the CPU has spent running users' processes that have been niced.</p>|Zabbix agent|system.cpu.util[,nice]|
|CPU iowait time|<p>Time the CPU has been waiting for I/O to complete.</p>|Zabbix agent|system.cpu.util[,iowait]|
|CPU steal time|<p>The amount of "stolen" CPU from this virtual machine by the hypervisor for other tasks, such as running another virtual machine.</p>|Zabbix agent|system.cpu.util[,steal]|
|CPU interrupt time|<p>Time the CPU has spent servicing hardware interrupts.</p>|Zabbix agent|system.cpu.util[,interrupt]|
|CPU softirq time|<p>Time the CPU has been servicing software interrupts.</p>|Zabbix agent|system.cpu.util[,softirq]|
|CPU guest time|<p>Time spent on running a virtual CPU for a guest operating system.</p>|Zabbix agent|system.cpu.util[,guest]|
|CPU guest nice time|<p>Time spent on running a niced guest (a virtual CPU for guest operating systems under the control of the Linux kernel).</p>|Zabbix agent|system.cpu.util[,guest_nice]|
|Context switches per second|<p>The combined rate at which all processors on the computer are switched from one thread to another.</p>|Zabbix agent|system.cpu.switches<p>**Preprocessing**</p><ul><li>Change per second</li></ul>|
|Interrupts per second|<p>Number of interrupts processed.</p>|Zabbix agent|system.cpu.intr<p>**Preprocessing**</p><ul><li>Change per second</li></ul>|
|Get filesystems|<p>The `vfs.fs.get` key acquires raw information set about the filesystems. Later to be extracted by preprocessing in dependent items.</p>|Zabbix agent|vfs.fs.get|
|Memory utilization|<p>The percentage of used memory is calculated as `100-pavailable`.</p>|Dependent item|vm.memory.utilization<p>**Preprocessing**</p><ul><li><p>JavaScript: `return (100-value);`</p></li></ul>|
|Available memory in %|<p>The available memory as percentage of the total. See also Appendixes in Zabbix Documentation about parameters of the `vm.memory.size` item.</p>|Zabbix agent|vm.memory.size[pavailable]|
|Total memory|<p>Total memory expressed in bytes.</p>|Zabbix agent|vm.memory.size[total]|
|Available memory|<p>The available memory:</p><p>- in Linux = free + buffers + cache;</p><p>- on other platforms calculation may vary.</p><p></p><p>See also Appendixes in Zabbix Documentation about parameters of the `vm.memory.size` item.</p>|Zabbix agent|vm.memory.size[available]|
|Total swap space|<p>The total space of the swap volume/file expressed in bytes.</p>|Zabbix agent|system.swap.size[,total]|
|Free swap space|<p>The free space of the swap volume/file expressed in bytes.</p>|Zabbix agent|system.swap.size[,free]|
|Free swap space in %|<p>The free space of the swap volume/file expressed in %.</p>|Zabbix agent|system.swap.size[,pfree]|
|System uptime|<p>The system uptime expressed in the following format: "N days, hh:mm:ss".</p>|Zabbix agent|system.uptime|
|System boot time||Zabbix agent|system.boottime<p>**Preprocessing**</p><ul><li><p>Discard unchanged with heartbeat: `1h`</p></li></ul>|
|System local time|<p>The local system time of the host.</p>|Zabbix agent|system.localtime|
|System name|<p>The host name of the system.</p>|Zabbix agent|system.hostname<p>**Preprocessing**</p><ul><li><p>Discard unchanged with heartbeat: `12h`</p></li></ul>|
|System description|<p>The information as normally returned by `uname -a`.</p>|Zabbix agent|system.uname<p>**Preprocessing**</p><ul><li><p>Discard unchanged with heartbeat: `12h`</p></li></ul>|
|Number of logged in users|<p>The number of users who are currently logged in.</p>|Zabbix agent|system.users.num|
|Maximum number of open file descriptors|<p>May be increased by using the `sysctl` utility or modifying the file `/etc/sysctl.conf`.</p>|Zabbix agent|kernel.maxfiles<p>**Preprocessing**</p><ul><li><p>Discard unchanged with heartbeat: `1d`</p></li></ul>|
|Maximum number of processes|<p>May be increased by using the `sysctl` utility or modifying the file `/etc/sysctl.conf`.</p>|Zabbix agent|kernel.maxproc<p>**Preprocessing**</p><ul><li><p>Discard unchanged with heartbeat: `1d`</p></li></ul>|
|Number of processes||Zabbix agent|proc.num|
|Number of running processes||Zabbix agent|proc.num[,,run]|
|Checksum of /etc/passwd||Zabbix agent|vfs.file.cksum[/etc/passwd,sha256]<p>**Preprocessing**</p><ul><li><p>Discard unchanged with heartbeat: `1h`</p></li></ul>|
|Operating system||Zabbix agent|system.sw.os<p>**Preprocessing**</p><ul><li><p>Discard unchanged with heartbeat: `1d`</p></li></ul>|
|Operating system architecture|<p>The architecture of the operating system.</p>|Zabbix agent|system.sw.arch<p>**Preprocessing**</p><ul><li><p>Discard unchanged with heartbeat: `1d`</p></li></ul>|
|Number of installed packages||Zabbix agent|system.sw.packages.get<p>**Preprocessing**</p><ul><li><p>JSON Path: `$.length()`</p></li><li><p>Discard unchanged with heartbeat: `12h`</p></li></ul>|

### Triggers

|Name|Description|Expression|Severity|Dependencies and additional info|
|----|-----------|----------|--------|--------------------------------|
|Zabbix agent is not available|<p>For passive agents only, host availability is used with `{$AGENT.TIMEOUT}` as a time threshold.</p>|`max(/Linux by Zabbix agent/zabbix[host,agent,available],{$AGENT.TIMEOUT})=0`|Average|**Manual close**: Yes|
|Load average is too high|<p>The load average per CPU is too high. The system may be slow to respond.</p>|`min(/Linux by Zabbix agent/system.cpu.load[all,avg1],5m)/last(/Linux by Zabbix agent/system.cpu.num)>{$LOAD_AVG_PER_CPU.MAX.WARN} and last(/Linux by Zabbix agent/system.cpu.load[all,avg5])>0 and last(/Linux by Zabbix agent/system.cpu.load[all,avg15])>0`|Average||
|High CPU utilization|<p>CPU utilization is too high. The system might be slow to respond.</p>|`min(/Linux by Zabbix agent/system.cpu.util,5m)>{$CPU.UTIL.CRIT}`|Warning|**Depends on**:<br><ul><li>Load average is too high</li></ul>|
|High memory utilization|<p>The system is running out of free memory.</p>|`min(/Linux by Zabbix agent/vm.memory.utilization,5m)>{$MEMORY.UTIL.MAX}`|Average|**Depends on**:<br><ul><li>Lack of available memory</li></ul>|
|Lack of available memory|<p>The system is running out of memory.</p>|`max(/Linux by Zabbix agent/vm.memory.size[available],5m)<{$MEMORY.AVAILABLE.MIN} and last(/Linux by Zabbix agent/vm.memory.size[total])>0`|Average||
|High swap space usage|<p>If there is no swap configured, this trigger is ignored.</p>|`max(/Linux by Zabbix agent/system.swap.size[,pfree],5m)<{$SWAP.PFREE.MIN.WARN} and last(/Linux by Zabbix agent/system.swap.size[,total])>0`|Warning|**Depends on**:<br><ul><li>Lack of available memory</li><li>High memory utilization</li></ul>|
|{HOST.NAME} has been restarted|<p>The host uptime is less than 10 minutes.</p>|`last(/Linux by Zabbix agent/system.uptime)<10m`|Warning|**Manual close**: Yes|
|System time is out of sync|<p>The host's system time is different from Zabbix server time.</p>|`fuzzytime(/Linux by Zabbix agent/system.localtime,{$SYSTEM.FUZZYTIME.MAX})=0`|Warning|**Manual close**: Yes|
|System name has changed|<p>The name of the system has changed. Acknowledge to close the problem manually.</p>|`change(/Linux by Zabbix agent/system.hostname) and length(last(/Linux by Zabbix agent/system.hostname))>0`|Info|**Manual close**: Yes|
|Configured max number of open filedescriptors is too low||`last(/Linux by Zabbix agent/kernel.maxfiles)<{$KERNEL.MAXFILES.MIN}`|Info||
|Configured max number of processes is too low||`last(/Linux by Zabbix agent/kernel.maxproc)<{$KERNEL.MAXPROC.MIN}`|Info|**Depends on**:<br><ul><li>Getting closer to process limit</li></ul>|
|Getting closer to process limit||`last(/Linux by Zabbix agent/proc.num)/last(/Linux by Zabbix agent/kernel.maxproc)*100>80`|Warning||
|/etc/passwd has been changed||`last(/Linux by Zabbix agent/vfs.file.cksum[/etc/passwd,sha256],#1)<>last(/Linux by Zabbix agent/vfs.file.cksum[/etc/passwd,sha256],#2)`|Info|**Manual close**: Yes<br>**Depends on**:<br><ul><li>System name has changed</li><li>Operating system description has changed</li></ul>|
|Operating system description has changed|<p>The description of the operating system has changed. Possible reasons are that the system has been updated or replaced. Acknowledge to close the problem manually.</p>|`change(/Linux by Zabbix agent/system.sw.os) and length(last(/Linux by Zabbix agent/system.sw.os))>0`|Info|**Manual close**: Yes<br>**Depends on**:<br><ul><li>System name has changed</li></ul>|
|Number of installed packages has been changed||`change(/Linux by Zabbix agent/system.sw.packages.get)<>0`|Warning|**Manual close**: Yes|

### LLD rule Mounted filesystem discovery

|Name|Description|Type|Key and additional info|
|----|-----------|----|-----------------------|
|Mounted filesystem discovery|<p>The discovery of mounted filesystems with different types.</p>|Dependent item|vfs.fs.dependent.discovery<p>**Preprocessing**</p><ul><li><p>JavaScript: `The text is too long. Please see the template.`</p></li><li><p>Discard unchanged with heartbeat: `1h`</p></li></ul>|

### Item prototypes for Mounted filesystem discovery

|Name|Description|Type|Key and additional info|
|----|-----------|----|-----------------------|
|FS [{#FSNAME}]: Get data|<p>Intermediate data of `{#FSNAME}` filesystem.</p>|Dependent item|vfs.fs.dependent[{#FSNAME},data]<p>**Preprocessing**</p><ul><li><p>JSON Path: `$.[?(@.fsname=='{#FSNAME}')].first()`</p></li></ul>|
|FS [{#FSNAME}]: Option: Read-only|<p>The filesystem is mounted as read-only. It is available only for Zabbix agents 6.4 and higher.</p>|Dependent item|vfs.fs.dependent[{#FSNAME},readonly]<p>**Preprocessing**</p><ul><li><p>JSON Path: `$.options`</p><p>⛔️Custom on fail: Discard value</p></li><li><p>Regular expression: `(?:^|,)ro\b 1`</p><p>⛔️Custom on fail: Set value to: `0`</p></li></ul>|
|FS [{#FSNAME}]: Space: Used|<p>Used storage expressed in bytes.</p>|Dependent item|vfs.fs.dependent.size[{#FSNAME},used]<p>**Preprocessing**</p><ul><li><p>JSON Path: `$.bytes.used`</p></li></ul>|
|FS [{#FSNAME}]: Space: Total|<p>Total space expressed in bytes.</p>|Dependent item|vfs.fs.dependent.size[{#FSNAME},total]<p>**Preprocessing**</p><ul><li><p>JSON Path: `$.bytes.total`</p></li></ul>|
|FS [{#FSNAME}]: Space: Used, in %|<p>Calculated as the percentage of currently used space compared to the maximum available space.</p>|Dependent item|vfs.fs.dependent.size[{#FSNAME},pused]<p>**Preprocessing**</p><ul><li><p>JSON Path: `$.bytes.pused`</p></li></ul>|
|FS [{#FSNAME}]: Space: Available|<p>Available storage space expressed in bytes.</p>|Dependent item|vfs.fs.dependent.size[{#FSNAME},free]<p>**Preprocessing**</p><ul><li><p>JSON Path: `$.bytes.free`</p></li></ul>|
|FS [{#FSNAME}]: Inodes: Free, in %|<p>Free metadata space expressed in %.</p>|Dependent item|vfs.fs.dependent.inode[{#FSNAME},pfree]<p>**Preprocessing**</p><ul><li><p>JSON Path: `$.inodes.pfree`</p></li></ul>|

### Trigger prototypes for Mounted filesystem discovery

|Name|Description|Expression|Severity|Dependencies and additional info|
|----|-----------|----------|--------|--------------------------------|
|FS [{#FSNAME}]: Filesystem has become read-only|<p>The filesystem has become read-only, possibly due to an I/O error. Available only for Zabbix agents 6.4 and higher.</p>|`last(/Linux by Zabbix agent/vfs.fs.dependent[{#FSNAME},readonly],#2)=0 and last(/Linux by Zabbix agent/vfs.fs.dependent[{#FSNAME},readonly])=1`|Average|**Manual close**: Yes|
|FS [{#FSNAME}]: Space is critically low|<p>The volume's space usage exceeds the `{$VFS.FS.PUSED.MAX.CRIT:"{#FSNAME}"}%` limit.<br>The trigger expression is based on the current used and maximum available spaces.<br>Event name represents the total volume space, which can differ from the maximum available space, depending on the filesystem type.</p>|`min(/Linux by Zabbix agent/vfs.fs.dependent.size[{#FSNAME},pused],5m)>{$VFS.FS.PUSED.MAX.CRIT:"{#FSNAME}"}`|Average|**Manual close**: Yes|
|FS [{#FSNAME}]: Space is low|<p>The volume's space usage exceeds the `{$VFS.FS.PUSED.MAX.WARN:"{#FSNAME}"}%` limit.<br>The trigger expression is based on the current used and maximum available spaces.<br>Event name represents the total volume space, which can differ from the maximum available space, depending on the filesystem type.</p>|`min(/Linux by Zabbix agent/vfs.fs.dependent.size[{#FSNAME},pused],5m)>{$VFS.FS.PUSED.MAX.WARN:"{#FSNAME}"}`|Warning|**Manual close**: Yes<br>**Depends on**:<br><ul><li>FS [{#FSNAME}]: Space is critically low</li></ul>|
|FS [{#FSNAME}]: Running out of free inodes|<p>Disk writing may fail if index nodes are exhausted, leading to error messages like "No space left on device" or "Disk is full", despite available free space.</p>|`min(/Linux by Zabbix agent/vfs.fs.dependent.inode[{#FSNAME},pfree],5m)<{$VFS.FS.INODE.PFREE.MIN.CRIT:"{#FSNAME}"}`|Average||
|FS [{#FSNAME}]: Running out of free inodes|<p>Disk writing may fail if index nodes are exhausted, leading to error messages like "No space left on device" or "Disk is full", despite available free space.</p>|`min(/Linux by Zabbix agent/vfs.fs.dependent.inode[{#FSNAME},pfree],5m)<{$VFS.FS.INODE.PFREE.MIN.WARN:"{#FSNAME}"}`|Warning|**Depends on**:<br><ul><li>FS [{#FSNAME}]: Running out of free inodes</li></ul>|

### LLD rule Block devices discovery

|Name|Description|Type|Key and additional info|
|----|-----------|----|-----------------------|
|Block devices discovery||Zabbix agent|vfs.dev.discovery<p>**Preprocessing**</p><ul><li><p>Discard unchanged with heartbeat: `1h`</p></li></ul>|

### Item prototypes for Block devices discovery

|Name|Description|Type|Key and additional info|
|----|-----------|----|-----------------------|
|{#DEVNAME}: Get stats|<p>The contents of get `/sys/block/{#DEVNAME}/stat` to get the disk statistics.</p>|Zabbix agent|vfs.file.contents[/sys/block/{#DEVNAME}/stat]<p>**Preprocessing**</p><ul><li><p>JavaScript: `return JSON.stringify(value.trim().split(/ +/));`</p></li></ul>|
|{#DEVNAME}: Disk read rate|<p>r/s (read operations per second) - the number (after merges) of read requests completed per second for the device.</p>|Dependent item|vfs.dev.read.rate[{#DEVNAME}]<p>**Preprocessing**</p><ul><li><p>JSON Path: `$[0]`</p></li><li>Change per second</li></ul>|
|{#DEVNAME}: Disk write rate|<p>w/s (write operations per second) - the number (after merges) of write requests completed per second for the device.</p>|Dependent item|vfs.dev.write.rate[{#DEVNAME}]<p>**Preprocessing**</p><ul><li><p>JSON Path: `$[4]`</p></li><li>Change per second</li></ul>|
|{#DEVNAME}: Disk read time (rate)|<p>The rate of total read time counter; used in `r_await` calculation.</p>|Dependent item|vfs.dev.read.time.rate[{#DEVNAME}]<p>**Preprocessing**</p><ul><li><p>JSON Path: `$[3]`</p></li><li>Change per second</li><li><p>Custom multiplier: `0.001`</p></li></ul>|
|{#DEVNAME}: Disk write time (rate)|<p>The rate of total write time counter; used in `w_await` calculation.</p>|Dependent item|vfs.dev.write.time.rate[{#DEVNAME}]<p>**Preprocessing**</p><ul><li><p>JSON Path: `$[7]`</p></li><li>Change per second</li><li><p>Custom multiplier: `0.001`</p></li></ul>|
|{#DEVNAME}: Disk read request avg waiting time (r_await)|<p>This formula contains two Boolean expressions that evaluate to 1 or 0 in order to set the calculated metric to zero and to avoid the exception - division by zero.</p>|Calculated|vfs.dev.read.await[{#DEVNAME}]|
|{#DEVNAME}: Disk write request avg waiting time (w_await)|<p>This formula contains two Boolean expressions that evaluate to 1 or 0 in order to set the calculated metric to zero and to avoid the exception - division by zero.</p>|Calculated|vfs.dev.write.await[{#DEVNAME}]|
|{#DEVNAME}: Disk average queue size (avgqu-sz)|<p>The current average disk queue; the number of requests outstanding on the disk while the performance data is being collected.</p>|Dependent item|vfs.dev.queue_size[{#DEVNAME}]<p>**Preprocessing**</p><ul><li><p>JSON Path: `$[10]`</p></li><li>Change per second</li><li><p>Custom multiplier: `0.001`</p></li></ul>|
|{#DEVNAME}: Disk utilization|<p>The percentage of elapsed time during which the selected disk drive was busy while servicing read or write requests.</p>|Dependent item|vfs.dev.util[{#DEVNAME}]<p>**Preprocessing**</p><ul><li><p>JSON Path: `$[9]`</p></li><li>Change per second</li><li><p>Custom multiplier: `0.1`</p></li></ul>|

### Trigger prototypes for Block devices discovery

|Name|Description|Expression|Severity|Dependencies and additional info|
|----|-----------|----------|--------|--------------------------------|
|{#DEVNAME}: Disk read/write request responses are too high|<p>This trigger might indicate the disk `{#DEVNAME}` saturation.</p>|`min(/Linux by Zabbix agent/vfs.dev.read.await[{#DEVNAME}],15m) > {$VFS.DEV.READ.AWAIT.WARN:"{#DEVNAME}"} or min(/Linux by Zabbix agent/vfs.dev.write.await[{#DEVNAME}],15m) > {$VFS.DEV.WRITE.AWAIT.WARN:"{#DEVNAME}"}`|Warning|**Manual close**: Yes|

### LLD rule Network interface discovery

|Name|Description|Type|Key and additional info|
|----|-----------|----|-----------------------|
|Network interface discovery|<p>The discovery of network interfaces.</p>|Zabbix agent|net.if.discovery|

### Item prototypes for Network interface discovery

|Name|Description|Type|Key and additional info|
|----|-----------|----|-----------------------|
|Interface {#IFNAME}: Bits received||Zabbix agent|net.if.in["{#IFNAME}"]<p>**Preprocessing**</p><ul><li>Change per second</li><li><p>Custom multiplier: `8`</p></li></ul>|
|Interface {#IFNAME}: Bits sent||Zabbix agent|net.if.out["{#IFNAME}"]<p>**Preprocessing**</p><ul><li>Change per second</li><li><p>Custom multiplier: `8`</p></li></ul>|
|Interface {#IFNAME}: Outbound packets with errors||Zabbix agent|net.if.out["{#IFNAME}",errors]<p>**Preprocessing**</p><ul><li>Change per second</li></ul>|
|Interface {#IFNAME}: Inbound packets with errors||Zabbix agent|net.if.in["{#IFNAME}",errors]<p>**Preprocessing**</p><ul><li>Change per second</li></ul>|
|Interface {#IFNAME}: Outbound packets discarded||Zabbix agent|net.if.out["{#IFNAME}",dropped]<p>**Preprocessing**</p><ul><li>Change per second</li></ul>|
|Interface {#IFNAME}: Inbound packets discarded||Zabbix agent|net.if.in["{#IFNAME}",dropped]<p>**Preprocessing**</p><ul><li>Change per second</li></ul>|
|Interface {#IFNAME}: Operational status|<p>Reference: https://www.kernel.org/doc/Documentation/networking/operstates.txt</p>|Zabbix agent|vfs.file.contents["/sys/class/net/{#IFNAME}/operstate"]<p>**Preprocessing**</p><ul><li><p>JavaScript: `The text is too long. Please see the template.`</p></li></ul>|
|Interface {#IFNAME}: Interface type|<p>It indicates the interface protocol type as a decimal value.</p><p>See `include/uapi/linux/if_arp.h` for all possible values.</p><p>Reference: https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-class-net</p>|Zabbix agent|vfs.file.contents["/sys/class/net/{#IFNAME}/type"]<p>**Preprocessing**</p><ul><li><p>Discard unchanged with heartbeat: `1d`</p></li></ul>|
|Interface {#IFNAME}: Speed|<p>It indicates the latest or current speed value of the interface. The value is an integer representing the link speed expressed in bits/sec.</p><p>This attribute is only valid for the interfaces that implement the ethtool `get_link_ksettings` method (mostly Ethernet).</p><p></p><p>Reference: https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-class-net</p>|Zabbix agent|vfs.file.contents["/sys/class/net/{#IFNAME}/speed"]<p>**Preprocessing**</p><ul><li><p>Custom multiplier: `1000000`</p></li><li><p>Discard unchanged with heartbeat: `1h`</p></li></ul>|

### Trigger prototypes for Network interface discovery

|Name|Description|Expression|Severity|Dependencies and additional info|
|----|-----------|----------|--------|--------------------------------|
|Interface {#IFNAME}: High bandwidth usage|<p>The utilization of the network interface is close to its estimated maximum bandwidth.</p>|`(avg(/Linux by Zabbix agent/net.if.in["{#IFNAME}"],15m)>({$IF.UTIL.MAX:"{#IFNAME}"}/100)*last(/Linux by Zabbix agent/vfs.file.contents["/sys/class/net/{#IFNAME}/speed"]) or avg(/Linux by Zabbix agent/net.if.out["{#IFNAME}"],15m)>({$IF.UTIL.MAX:"{#IFNAME}"}/100)*last(/Linux by Zabbix agent/vfs.file.contents["/sys/class/net/{#IFNAME}/speed"])) and last(/Linux by Zabbix agent/vfs.file.contents["/sys/class/net/{#IFNAME}/speed"])>0`|Warning|**Manual close**: Yes<br>**Depends on**:<br><ul><li>Interface {#IFNAME}: Link down</li></ul>|
|Interface {#IFNAME}: High error rate|<p>It recovers when it is below 80% of the `{$IF.ERRORS.WARN:"{#IFNAME}"}` threshold.</p>|`min(/Linux by Zabbix agent/net.if.in["{#IFNAME}",errors],5m)>{$IF.ERRORS.WARN:"{#IFNAME}"} or min(/Linux by Zabbix agent/net.if.out["{#IFNAME}",errors],5m)>{$IF.ERRORS.WARN:"{#IFNAME}"}`|Warning|**Manual close**: Yes<br>**Depends on**:<br><ul><li>Interface {#IFNAME}: Link down</li></ul>|
|Interface {#IFNAME}: Link down|<p>This trigger expression works as follows:<br>1. It can be triggered if the operations status is down.<br>2. `{$IFCONTROL:"{#IFNAME}"}=1` - a user can redefine the context macro to "0", marking this interface as not important. No new trigger will be fired if this interface is down.<br>3. `last(/TEMPLATE_NAME/METRIC,#1)<>last(/TEMPLATE_NAME/METRIC,#2)` - the trigger fires only if the operational status was up to (1) sometime before (so, does not fire for "eternal off" interfaces.)<br><br>WARNING: if closed manually - it will not fire again on the next poll, because of .diff.</p>|`{$IFCONTROL:"{#IFNAME}"}=1 and last(/Linux by Zabbix agent/vfs.file.contents["/sys/class/net/{#IFNAME}/operstate"])=2 and (last(/Linux by Zabbix agent/vfs.file.contents["/sys/class/net/{#IFNAME}/operstate"],#1)<>last(/Linux by Zabbix agent/vfs.file.contents["/sys/class/net/{#IFNAME}/operstate"],#2))`|Average|**Manual close**: Yes|
|Interface {#IFNAME}: Ethernet has changed to lower speed than it was before|<p>This Ethernet connection has transitioned down from its known maximum speed. This might be a sign of autonegotiation issues. Acknowledge to close the problem manually.</p>|`change(/Linux by Zabbix agent/vfs.file.contents["/sys/class/net/{#IFNAME}/speed"])<0 and last(/Linux by Zabbix agent/vfs.file.contents["/sys/class/net/{#IFNAME}/speed"])>0 and (last(/Linux by Zabbix agent/vfs.file.contents["/sys/class/net/{#IFNAME}/type"])=6 or last(/Linux by Zabbix agent/vfs.file.contents["/sys/class/net/{#IFNAME}/type"])=1) and (last(/Linux by Zabbix agent/vfs.file.contents["/sys/class/net/{#IFNAME}/operstate"])<>2)`|Info|**Manual close**: Yes<br>**Depends on**:<br><ul><li>Interface {#IFNAME}: Link down</li></ul>|

## Feedback

Please report any issues with the template at [`https://support.zabbix.com`](https://support.zabbix.com)

You can also provide feedback, discuss the template, or ask for help at [`ZABBIX forums`](https://www.zabbix.com/forum/zabbix-suggestions-and-feedback)

