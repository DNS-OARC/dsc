Parser=$1
Status=0

if test -z "$Parser"
then
	echo "usage: $0 <parser-executable>"
	exit 255
fi

gauge='/usr/bin/time -l'
limiter='limits -B -v 220m'

testBuild() {
	echo
	echo "testBuild $Parser {"

	uname -a
	export `grep ^CXX Makefile | head -1 | sed 's/ *//g'`
	$CXX --version

	rm $Parser $Parser.o
	sh -x -c "$gauge make $Parser"
	ls -l $Parser
	sh -x -c "strip $Parser"
	ls -l $Parser

	echo "} testBuild $Parser"
}

testSpeed() {
	scale=1
	tagCount=$1
	echo
	echo "testSpeed scale: $scale tagCount: $tagCount {"
	date

	./xmlgen -f $scale -s $tagCount | \
		$gauge $limiter ./$Parser
	Status=$?

	date
	echo "} testSpeed"
}

echo -n "start timestamp:  "
date

testBuild

tagCount=1
while test $tagCount -lt 1000000 -a $Status -eq 0
do
	testSpeed $tagCount
	step=$(($tagCount/10))
	if test $step -le 0
	then
		step=$tagCount
	fi
	tagCount=$(($tagCount+$step))
done

echo -n "finish timestamp: "
date
