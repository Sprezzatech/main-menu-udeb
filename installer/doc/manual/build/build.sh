#!/bin/sh

set -e

[ -r ./po_functions ] || exit 1
. ./po_functions

if [ -z "$languages" ]; then
    # Please add languages only if they build properly.
    # languages="en cs es fr ja nl pt_BR" # ca da de el eu it ru

    # Buildlist of languages to be included on RC3 CD's
    languages="en cs de es fr ja pt_BR ru"
fi

if [ -z "$architectures" ]; then
    architectures="alpha arm hppa i386 ia64 m68k mips mipsel powerpc s390 sparc"
fi

if [ -z "$destination" ]; then
    destination="/tmp/manual"
fi

if [ -z "$formats" ]; then
    #formats="html pdf ps txt"
    formats="html pdf txt"
fi

[ -e "$destination" ] || mkdir -p "$destination"

if [ "$official_build" ]; then
    # Propagate this to children.
    export official_build
fi

# We need to merge the XML files for English and update the POT files
export PO_USEBUILD="1"
update_templates

for lang in $languages; do
    echo "Language: $lang";

    # Update PO files and create XML files
    check_po
    if [ -n "$USES_PO" ] ; then
        generate_xml
    fi
    
    for arch in $architectures; do
        echo "Architecture: $arch"
        if [ -n "$noarchdir" ]; then
                destsuffix="$lang"
        else
                destsuffix="${lang}.${arch}"
        fi
        ./buildone.sh "$arch" "$lang" "$formats"
        mkdir -p "$destination/$destsuffix"
        for format in $formats; do
            if [ "$format" = html ]; then
                mv ./build.out/html/* "$destination/$destsuffix"
            else
                # Do not fail because of missing PDF support for some languages
                mv ./build.out/install.$lang.$format "$destination/$destsuffix" || true
            fi
        done

        ./clear.sh
    done

    # Delete generated XML files
    [ -n "$USES_PO" ] && rm -r ../$lang || true
done

PRESEED="../en/appendix/example-preseed.xml"
if [ -f $PRESEED ] && [ -f preseed.awk ] ; then
    gawk -f preseed.awk $PRESEED >$destination/example-preseed.txt
fi

clear_po
