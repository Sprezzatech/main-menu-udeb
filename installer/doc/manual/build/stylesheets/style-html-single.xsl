<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<!-- Import our base stylesheet -->
<xsl:import
href="file:///usr/share/sgml/docbook/stylesheet/xsl/nwalsh/html/docbook.xsl"/>

<!-- Include our common parameters -->
<xsl:include href="style-common.xsl"/>


<!-- Any html-specific parameters follow -->
<!-- You may find some in /etc/sgml/docbook-xsl/html/param.xsl -->


<!-- Where to put resulting html. Don't forget trailing slash! -->
<xsl:param name="base.dir" select="'./'"/>

<!-- Do we want fancy icons around note, warning, etc.? -->
<xsl:param name="admon.graphics">0</xsl:param>

<!-- Do we want fancy icons instead of Next, Prev, Up, Home? -->
<xsl:param name="navig.graphics">0</xsl:param>

</xsl:stylesheet>
