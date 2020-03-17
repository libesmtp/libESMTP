<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
		xmlns="http://www.w3.org/1999/xhtml"
                version='1.0'>

<xsl:import href="/usr/share/xml/docbook/stylesheet/docbook-xsl/xhtml/docbook.xsl"/>
<xsl:include href="libesmtp-user.xsl"/>

<xsl:param name="html.stylesheet">libesmtp.css</xsl:param>
<xsl:param name="funcsynopsis.style">ansi</xsl:param>
<xsl:param name="function.parens" select="1"/>
<xsl:param name="css.decoration" select="0"/>
<xsl:param name="refentry.separator" select="0"/>
<xsl:param name="refentry.generate.title" select="1"></xsl:param>
<xsl:param name="refentry.generate.name" select="0"></xsl:param>
<xsl:param name="chunker.output.encoding">ISO-8859-1</xsl:param>

</xsl:stylesheet>

