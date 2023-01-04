PATH=/usr/ucb:/bin:/usr/bin
if [ ! -r version ]; then echo 0 > version; fi
touch version
echo `cat version` `basename \`pwd\`` `cat ../conf/RELEASE` | \
awk '	{	version = $1 + 1; system = $2; release = $3; }\
END	{	printf "char version[] = \"Sun UNIX 4.2 Release %s (%s) #%d: ", release, system, version > "vers.c";\
		printf "%d\n", version > "version"; }' 
echo `date`'\nCopyright (c) 1986 by Sun Microsystems, Inc.\n";' >> vers.c
