# TODO

## feature
* remove lifo code
* merge feature branch to main branch
* adapt to nemos framework bluetooth
* remove STRUCT_SECTION_* from source code and redesign the definition
  * BT_CONN_CB_DEFINE
  * BT_MEM_POOL_DEFINE_IN_SECT
  * BT_MEM_POOL_DEFINE_IN_SECT_STATIC
  * ~~BT_L2CAP_BR_CHANNEL_DEFINE~~
  * ~~BT_L2CAP_FIXED_CHANNEL_DEFINE~~
  * BT_SCO_CONN_CB_DEFINE
  * BT_MESH_APP_KEY_CB_DEFINE
  * BT_MESH_SUBNET_CB_DEFINE
  * BT_MESH_HB_CB_DEFINE
  * BT_MESH_LPN_CB_DEFINE
  * BT_MESH_FRIEND_CB_DEFINE
  * BT_MESH_BEACON_CB_DEFINE
  * BT_MESH_PROXY_CB_DEFINE
  * BT_IAS_CB_DEFINE
  * BT_GATT_SERVICE_DEFINE
  * Z_INTERNAL_BT_NUS_INST_DEFINE
* remove shim folder
* add init function for each module

## cmd tool
* ~~remove bt_shell_* from source code~~
* ~~add bt_cmd_* interface~~
* ~~modify cmd register to support dynamic registration~~

## Functional verification
* verify BR/EDR
* verify LE
* verify LEA
* verify Mesh

## storage design
* remove settings from source code
* add storage register interface
* add empty storage interface

## build system
* Cmake build system support

## platform support
* linux
  * ~~posix os support~~
  * ~~user channel hci socket support~~
* nuttx
  * ~~nuttx posix os support~~
  * "/dev/ttyHCI0" hci driver support
  * Makefile & Kconfig
* freertos
* windows

## demo
* SDL audio demo
* merge feature branch to main branch

## test
* add unit test for each module
* auto test framework

## Certification
* AutoPTS
