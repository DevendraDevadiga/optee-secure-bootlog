# OP-TEE Secure bootlog capture and display support for QEMUv7


1MB of secure memory will be used to store the secure boot log and can be fetch and display from TA.
In Linux, "dmesg" is the command to get the bootlog. Similarly "tmesg" - TEE Message command is provided for secure world.


Command to get normal world bootlog message and display:
-------------------------------------------------------

```ruby
# dmesg
Booting Linux on physical CPU 0x0
Linux version 5.14.0 .....
CPU: ARMv7 Processor [412fc0f1] revision 1 (ARMv7), cr=10c5387d
......

```

Command to get the secure bootlog message and display:
-------------------------------------------------------

```ruby
# tmesg
D/TC:0   boot_log_init:53 Bootlog setup done in core.
D/TC:0   console_init:112 Boot logging initialized
D/TC:0   add_phys_mem:571 VCORE_UNPG_RX_PA type TEE_RAM_RX 0x0e100000 size 0x0007e000
D/TC:0   add_phys_mem:571 VCORE_UNPG_RW_PA type TEE_RAM_RW 0x0e17e000 size 0x00182000
D/TC:0   add_phys_mem:571 TA_RAM_START type TA_RAM 0x0e300000 size 0x00d00000
D/TC:0   add_phys_mem:571 TEE_SHMEM_START type NSEC_SHM 0x7fe00000 size 0x00200000
D/TC:0   add_phys_mem:571 ROUNDDOWN(0x09040000, CORE_MMU_PGDIR_SIZE) type IO_SEC 0x09000000 size 0x00100000
D/TC:0   add_phys_mem:571 ROUNDDOWN(0x0e000000, CORE_MMU_PGDIR_SIZE) type IO_SEC 0x0e000000 size 0x00100000
D/TC:0   add_phys_mem:571 ROUNDDOWN((0x08000000 + 0), CORE_MMU_PGDIR_SIZE) type IO_SEC 0x08000000 size 0x00100000
D/TC:0   add_phys_mem:571 ROUNDDOWN((0x08000000 + 0x10000), CORE_MMU_PGDIR_SIZE) type IO_SEC 0x08000000 size 0x00100000
.....
I/TC: OP-TEE version: 3.15.0-236-g7b06f6ca-dev (gcc version 10.2.1 20201103 (GNU Toolchain for the A-profile Architecture 10.2-2020.11 (arm-10.16))) #1 Thu Dec 30 08:40:36 UTC 2021 arm
.....

```


Command to check the current size of secure bootlog message:
-------------------------------------------------------

```ruby
# tmesg -s
Current bootlog message size is : 6522

```

Command to clear the bootlog message and made the size to 0:
-------------------------------------------------------

```ruby
# 
# tmesg -c (Clear the bootlog and made the size to 0.)
# 
# tmesg 
D/TC:? 0 tee_ta_close_session:512 csess 0x559d6b98 id 1
D/TC:? 0 tee_ta_close_session:531 Destroy session
D/TC:? 0 tee_ta_init_session_with_context:607 Re-open TA 60276949-7ff3-4920-9bce-840c9dcf3098
D/TC:? 0 invoke_command:143 command entry point[2] for "pta.bootlog"

# 

```