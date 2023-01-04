: installboot.sh	1.1	86/09/25
: Usage: installboot bootfile disk
dd if=$1 of=$2 bs=1b count=15 seek=1 conv=sync
sync
