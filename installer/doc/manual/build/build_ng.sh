#!/bin/sh

if [ -z "$languages" ]; then
	# Please add languages only if they build properly.
	# languages="en cs es fr ja nl pt_BR" # ca da de el eu it ru

	# Buildlist of languages to be included on RC2 CD's
	languages="ca cs da de en es eu fi fr it ja nl pt_BR ru tl"
#	languages="ja"
fi

if [ -z "$architectures" ]; then
	architectures="alpha arm hppa i386 ia64 m68k mips mipsel powerpc s390 sparc"
#	architectures="i386"
fi

if [ -z "$destination" ]; then
	destination="/var/www/d-i/"
fi

if [ -z "$formats" ]; then
        formats="new.ps new.pdf html"
        #formats="html"
fi

[ -e "$destination" ] || mkdir -p "$destination"

if [ "$official_build" ]; then
	# Propagate this to children.
	export official_build
fi

#noarchdir="yes"

for lang in $languages; do
    echo "Language: $lang";
    for arch in $architectures; do
	echo "Architecture: $arch"
	if [ -n "$noarchdir" ]; then
		destsuffix="$lang"
	else
		destsuffix="${lang}/${arch}"
	fi
	./buildone_ng.sh "$arch" "$lang" "$formats"
	mkdir -p "$destination/$destsuffix"	
	for format in $formats; do
	    if [ "$format" = html ]; then
		mv ./build.out/html/ "$destination/$destsuffix"
	    else
		mv ./build.out/*.${format} "$destination/$destsuffix"
	    fi
	done
	./clear.sh
    done
done

