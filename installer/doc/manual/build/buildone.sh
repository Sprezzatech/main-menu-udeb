#!/bin/sh

if [ "$1" = "--help" ]; then
    echo "$0: Generate the Debian Installer Manual in several different formats"
    echo "Usage: $0 [arch] [lang] [format]"
    echo "[format] may consist of multiple formats provided they are quoted (e.g. \"html pdf\")"
    echo "Supported formats: html, ps, pdf, txt"
    exit 0
fi

arch=${1:-i386}
language=${2:-en}
formats=${3:-html}

## Configuration
basedir="$(cd "$(dirname $0)"; pwd)"
manual_path="$(echo $basedir | sed "s:/build$::")"
build_path="$manual_path/build"
cd $build_path

stylesheet_dir="$build_path/stylesheets"
stylesheet_profile="$stylesheet_dir/style-profile.xsl"
if [ ! "$web_build" ]; then
    stylesheet_html="$stylesheet_dir/style-html.xsl"
else
    stylesheet_html="$stylesheet_dir/style-html-web.xsl"
fi
stylesheet_html_single="$stylesheet_dir/style-html-single.xsl"
stylesheet_fo="$stylesheet_dir/style-fo.xsl"
stylesheet_dsssl="$stylesheet_dir/style-print.dsl"

entities_path="$build_path/entities"
source_path="$manual_path/$language"

if [ -z "$destdir" ]; then
    destdir="build.out"
fi

tempdir="build.tmp"
dynamic="${tempdir}/dynamic.ent"

create_profiled () {

    [ -x /usr/bin/xsltproc ] || return 9

    echo "Info: creating temporary profiled .xml file..."

    if [ ! "$official_build" ]; then
        unofficial_build="FIXME;unofficial-build"
    else
        unofficial_build=""
    fi

    # Now we source the profiling information for the selected architecture
    [ -f "arch-options/${arch}" ] || {
        echo "Error: unknown architecture '$arch'"
        return 1
    }
    . arch-options/$arch

    # Join all architecture options into one big variable
    condition="$fdisk;$network;$boot;$smp;$other;$goodies;$unofficial_build;$status"

    # Write dynamic non-profilable entities into the file
    echo "<!-- arch- and lang-specific non-profilable entities -->" > $dynamic
    echo "<!ENTITY langext \".${language}\">" >> $dynamic
    echo "<!ENTITY architecture \"${arch}\">" >> $dynamic
    echo "<!ENTITY kernelversion \"${kernelversion}\">" >> $dynamic
    echo "<!ENTITY altkernelversion \"${altkernelversion}\">" >> $dynamic
    sed "s:##SRCPATH##:$source_path:" templates/docstruct.ent >> $dynamic

    sed "s:##LANG##:$language:g" templates/install.xml.template | \
        sed "s:##TEMPDIR##:$tempdir:g" | \
        sed "s:##ENTPATH##:$entities_path:g" | \
        sed "s:##SRCPATH##:$source_path:" > $tempdir/install.${language}.xml

    # Create the profiled xml file
    /usr/bin/xsltproc \
        --xinclude \
        --stringparam profile.arch "$archspec" \
        --stringparam profile.condition "$condition" \
        --output $tempdir/install.${language}.profiled.xml \
        $stylesheet_profile \
        $tempdir/install.${language}.xml
    RET=$?; [ $RET -ne 0 ] && return $RET

    return 0
}

create_html () {

    echo "Info: creating .html files..."

    /usr/bin/xsltproc \
        --xinclude \
        --stringparam base.dir $destdir/html/ \
        $stylesheet_html \
        $tempdir/install.${language}.profiled.xml
    RET=$?; [ $RET -ne 0 ] && return $RET

    return 0
}

create_text () {

    [ -x /usr/bin/w3m ] || return 9

    echo "Info: creating temporary .html file..."

    /usr/bin/xsltproc \
        --xinclude \
        --output $tempdir/install.${language}.html \
        $stylesheet_html_single \
        $tempdir/install.${language}.profiled.xml
    RET=$?; [ $RET -ne 0 ] && return $RET

    # Replace some unprintable characters
    sed "s:–:-:g        # n-dash
         s:—:--:g       # m-dash
         s:“:\&quot;:g  # different types of quotes
         s:”:\&quot;:g
         s:„:\&quot;:g
         s:…:...:g      # ellipsis
         s:™: (tm):g    # trademark" \
        $tempdir/install.${language}.html >$tempdir/install.${language}.corr.html
    RET=$?; [ $RET -ne 0 ] && return $RET

    echo "Info: creating .txt file..."

    # Set encoding for output file
    case "$language" in
        cs)
            CHARSET=ISO-8859-2 ;;
        el)
            CHARSET=UTF-8 ;;
        ja)
            CHARSET=EUC-JP ;;
        ko)
            CHARSET=EUC-KR ;;
        ru)
            CHARSET=KOI8-R ;;
        *)
            CHARSET=ISO-8859-1 ;;
    esac
    
    /usr/bin/w3m -dump $tempdir/install.${language}.corr.html \
        -o display_charset=$CHARSET \
        >$destdir/install.${language}.txt
    RET=$?; [ $RET -ne 0 ] && return $RET

    # Ugly hack because w3m does not compute width for table borders correctly
    # for oriental character sets.
    # Output is not perfect, but a lot better than when uncorrected
    case "$language" in
        ja)
            mv $destdir/install.${language}.txt $tempdir
            sed "s:��:����:g
                 s:��:����:g
                 s:��:����:g
                 s:��:����:g
                 s:��:����:g
                 s:��:����:g
                 s:��:����:g
                 s:��:����:g
                 s:��:����:g
                 s:��$: ��:g
                 s:��:����:g
                 s:��:�� :g" \
            $tempdir/install.${language}.txt >$destdir/install.${language}.txt
            ;;
        ko)
            mv $destdir/install.${language}.txt $tempdir
            sed "s:��:����:g
                 s:��:����:g
                 s:��:����:g
                 s:��:����:g
                 s:��:����:g
                 s:��:����:g
                 s:��:����:g
                 s:��:����:g
                 s:��:����:g
                 s:��$: ��:g
                 s:��:����:g
                 s:��:�� :g" \
            $tempdir/install.${language}.txt >$destdir/install.${language}.txt
            ;;
    esac
    
    return 0
}

create_dvi () {
    
    [ -x /usr/bin/openjade ] || return 9
    [ -x /usr/bin/jadetex ] || return 9

    # Skip this step if the .dvi file already exists
    [ -f "$tempdir/install.${language}.dvi" ] && return

    echo "Info: creating temporary .tex file..."

    # And use openjade to generate a .tex file
    export SP_ENCODING="utf-8"
    /usr/bin/openjade -t tex \
        -b utf-8 \
        -o $tempdir/install.${language}.tex \
        -d $stylesheet_dsssl \
        -V tex-backend \
        $tempdir/install.${language}.profiled.xml
    RET=$?; [ $RET -ne 0 ] && return $RET

    echo "Info: creating temporary .dvi file..."

    # Next we use jadetext to generate a .dvi file
    # This needs three passes to properly generate the index (page numbering)
    cd $tempdir
    for PASS in 1 2 3 ; do
        /usr/bin/jadetex install.${language}.tex >/dev/null
        RET=$?; [ $RET -ne 0 ] && break
    done
    cd ..
    [ $RET -ne 0 ] && return $RET

    return 0
}

create_pdf() {
    
    [ -x /usr/bin/dvipdf ] || return 9

    create_dvi
    RET=$?; [ $RET -ne 0 ] && return $RET

    echo "Info: creating .pdf file..."

    /usr/bin/dvipdf $tempdir/install.${language}.dvi
    RET=$?; [ $RET -ne 0 ] && return $RET
    mv install.${language}.pdf $destdir/

    return 0
}

create_ps() {
    
    [ -x /usr/bin/dvips ] || return 9

    create_dvi
    RET=$?; [ $RET -ne 0 ] && return $RET

    echo "Info: creating .ps file..."

    /usr/bin/dvips -q $tempdir/install.${language}.dvi
    RET=$?; [ $RET -ne 0 ] && return $RET
    mv install.${language}.ps $destdir/

    return 0
}

## MAINLINE

# Clean old builds
rm -rf $tempdir
rm -rf $destdir

[ -d "$manual_path/$language" ] || {
    echo "Error: unknown language '$language'"
    exit 1
}

mkdir -p $tempdir
mkdir -p $destdir

# Create profiled XML. This is needed for all output formats.
create_profiled
RET=$?; [ $RET -ne 0 ] && exit 1

BUILD_OK=""
BUILD_FAIL=""
for format in $formats ; do
    if [ "$language" = "el" -o "$language" = "ja" -o "$language" = "ko" ] && \
       [ "$format" = "pdf" -o "$format" = "ps" ] ; then
        echo "Warning: pdf and ps formats are currently not supported for Greek, Japanese and Korean"
        BUILD_SKIP="$BUILD_SKIP $format"
        continue
    fi

    case $format in
        html)  create_html;;
        ps)    create_ps;;
        pdf)   create_pdf;;
        txt)   create_text;;
        *)
            echo "Error: format $format unknown or not yet supported!"
            exit 1
            ;;
    esac

    RET=$?
    case $RET in
        0)
            BUILD_OK="$BUILD_OK $format"
            ;;
        9)
            BUILD_FAIL="$BUILD_FAIL $format"
            echo "Error: build of $format failed because of missing build dependencies"
            ;;
        *)
            BUILD_FAIL="$BUILD_FAIL $format"
            echo "Error: build of $format failed with error code $RET"
            ;;
    esac
done

# Clean up
rm -r $tempdir

# Evaluate the overall results
[ -n "$BUILD_SKIP" ] && echo "Info: The following formats were skipped:$BUILD_SKIP"
[ -z "$BUILD_FAIL" ] && exit 0            # Build successful for all formats
echo "Warning: The following formats failed to build:$BUILD_FAIL"
[ -n "$BUILD_OK" ] && exit 2              # Build failed for some formats
exit 1                                    # Build failed for all formats
